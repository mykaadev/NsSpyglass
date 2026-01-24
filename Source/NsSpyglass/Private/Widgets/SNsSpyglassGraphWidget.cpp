// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "Widgets/SNsSpyglassGraphWidget.h"
#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Interfaces/IPluginManager.h"
#include "Rendering/DrawElements.h"
#include "Settings/NsSpyglassSettings.h"
#include "Styling/CoreStyle.h"

SNsSpyglassGraphWidget::SNsSpyglassGraphWidget()
    : ViewOffset(FVector2D::ZeroVector)
    , LastMousePos(FVector2D::ZeroVector)
{
}

void SNsSpyglassGraphWidget::Construct(const FArguments& InArgs)
{
    RecenterView();
    BuildNodes(FVector2D(960.f, 540.f));

    // Start intro animation (first time only)
    bIntroRunning = true;
    IntroElapsed = 0.f;

    // Default: everything hidden/inactive, will appear over time
    for (FPluginNode& N : Nodes)
    {
        N.bActive = false;
        N.AppearAlpha = 0.f;
        N.Velocity = FVector2D::ZeroVector;
    }

    // Assign delays (nice: high-degree nodes appear first)
    TArray<int32> Order;
    Order.Reserve(Nodes.Num());
    for (int32 i = 0; i < Nodes.Num(); ++i) Order.Add(i);

    Order.Sort([this](int32 A, int32 B)
    {
        return Nodes[A].Links.Num() > Nodes[B].Links.Num();
    });

    for (int32 k = 0; k < Order.Num(); ++k)
    {
        Nodes[Order[k]].AppearDelay = k * IntroStagger;
    }
}

void SNsSpyglassGraphWidget::BuildNodes(const FVector2D& ViewSize) const
{
    Nodes.Reset();
    RootIndex = INDEX_NONE;

    const UNsSpyglassSettings* Settings = UNsSpyglassSettings::GetSettings();
    const TArray<TSharedRef<IPlugin>>& Plugins = IPluginManager::Get().GetDiscoveredPlugins();
    TArray<TSharedRef<IPlugin>> FilteredPlugins;
    FilteredPlugins.Reserve(Plugins.Num());

    TMap<FString, int32> NameToIndex;
    TMap<FString, FLinearColor> CategoryColors;

    // Create nodes for plugins
    for (const TSharedRef<IPlugin>& Plugin : Plugins)
    {
        const bool bIsEngine = Plugin->GetLoadedFrom() == EPluginLoadedFrom::Engine;
        const bool bIsEnabled = Plugin->IsEnabled();
        if (bIsEngine && !Settings->bShowEnginePlugins)
        {
            continue;
        }
        if (!bIsEngine && !Settings->bShowProjectPlugins)
        {
            continue;
        }
        if (!bIsEnabled && !Settings->bShowDisabledPlugins)
        {
            continue;
        }

        FPluginNode Node;
        Node.Name = Plugin->GetName();
        Node.Plugin = Plugin;
        Node.bIsEngine = bIsEngine;
        Node.bIsEnabled = bIsEnabled;
        Node.Position = FVector2D::ZeroVector;
        Node.bFixed = false;

        const FPluginDescriptor& Desc = Plugin->GetDescriptor();
        const FString Category = Desc.Category.IsEmpty() ? TEXT("Misc") : Desc.Category;

        FLinearColor* Existing = CategoryColors.Find(Category);
        if (!Existing)
        {
            const int32 Index = CategoryColors.Num();
            const float Hue = FMath::Fmod(static_cast<float>(Index) * 50.f, 360.f);
            FLinearColor NewColor = FLinearColor::MakeFromHSV8(static_cast<uint8>(Hue), 160, 255);
            NewColor.A = 0.1f;
            CategoryColors.Add(Category, NewColor);
            Existing = CategoryColors.Find(Category);
        }
        Node.Color = *Existing;
        Node.Color.A = 0.1f;

        int32 Idx = Nodes.Add(Node);
        NameToIndex.Add(Node.Name, Idx);
        FilteredPlugins.Add(Plugin);
    }

    // Fill in links
    for (int32 i = 0; i < Nodes.Num(); ++i)
    {
        const TSharedRef<IPlugin>& Plugin = FilteredPlugins[i];
        const FPluginDescriptor& Desc = Plugin->GetDescriptor();
        for (const FPluginReferenceDescriptor& Ref : Desc.Plugins)
        {
            if (Ref.bEnabled)
            {
                if (int32* DepIdx = NameToIndex.Find(Ref.Name))
                {
                    Nodes[i].Links.AddUnique(*DepIdx);
                    Nodes[*DepIdx].Links.AddUnique(i); // undirected for layout

                    // Store directed dependency
                    Nodes[i].Dependencies.AddUnique(*DepIdx);
                    Nodes[*DepIdx].Dependents.AddUnique(i);
                }
            }
        }
    }

    int32 MaxImpact = 0;
    for (int32 i = 0; i < Nodes.Num(); ++i)
    {
        TSet<int32> Visited;
        TArray<int32> Stack = Nodes[i].Dependencies;
        while (Stack.Num() > 0)
        {
            const int32 Current = Stack.Pop();
            if (Visited.Contains(Current))
            {
                continue;
            }
            Visited.Add(Current);
            Stack.Append(Nodes[Current].Dependencies);
        }
        MaxImpact = FMath::Max(MaxImpact, Visited.Num());
        Nodes[i].ImpactStrength = static_cast<float>(Visited.Num());
    }

    if (MaxImpact > 0)
    {
        for (FPluginNode& Node : Nodes)
        {
            Node.ImpactStrength /= static_cast<float>(MaxImpact);
        }
    }

    // Arrange nodes in a circle to avoid overlapping at the origin
    if (Nodes.Num() > 1)
    {
        constexpr float Radius = 200.f;
        const float Step = 2.f * PI / static_cast<float>(Nodes.Num());
        for (int32 i = 0; i < Nodes.Num(); ++i)
        {
            const float Angle = Step * static_cast<float>(i - 1);
            Nodes[i].Position = FVector2D(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius);
        }
    }
}

void SNsSpyglassGraphWidget::InitStars(const FVector2D& ViewSize) const
{
    if (Stars.Num() == 0 || !ViewSize.Equals(StarsViewSize))
    {
        StarsViewSize = ViewSize;
        Stars.Empty();
        const int32 NumStars = 250;
        for (int32 i = 0; i < NumStars; ++i)
        {
            FBackgroundStar Star;
            Star.Position.X = FMath::FRandRange(-ViewSize.X, ViewSize.X);
            Star.Position.Y = FMath::FRandRange(-ViewSize.Y, ViewSize.Y);
            Star.Alpha = 0.f;
            Star.TargetAlpha = FMath::FRandRange(0.2f, 1.f);
            Star.FadeSpeed = FMath::FRandRange(0.5f, 1.5f);
            Stars.Add(Star);
        }
    }
}

int32 SNsSpyglassGraphWidget::HitTestNode(const FVector2D& LocalPos, const FVector2D& ViewSize) const
{
    const FVector2D Center = ViewSize * 0.5f;

    for (int32 i = 0; i < Nodes.Num(); ++i)
    {
        if (!Nodes[i].bActive || Nodes[i].AppearAlpha < 0.15f)
        {
            continue;
        }

        const float BaseSize = Nodes[i].Name == TEXT("Root") ? 60.f : 40.f;
        const float Size = BaseSize * ZoomAmount;

        const FVector2D NodePos = Center + ViewOffset + Nodes[i].Position * ZoomAmount;

        if ((LocalPos - NodePos).SizeSquared() <= FMath::Square(Size * 0.5f))
        {
            return i;
        }
    }

    return INDEX_NONE;
}

int32 SNsSpyglassGraphWidget::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
    if (Nodes.Num() == 0)
    {
        BuildNodes(AllottedGeometry.GetLocalSize());
    }

    InitStars(AllottedGeometry.GetLocalSize());

    const FVector2D Center = AllottedGeometry.GetLocalSize() * 0.5f;

    const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("WhiteBrush");
    const FVector2D StarOffset = ViewOffset * 0.1f;
    for (const FBackgroundStar& Star : Stars)
    {
        const FVector2D DrawPos = Center + StarOffset + Star.Position - FVector2D(1.f, 1.f);
        FSlateDrawElement::MakeBox(
            OutDrawElements,
            LayerId,
            AllottedGeometry.ToPaintGeometry(FVector2D(2.f, 2.f), FSlateLayoutTransform(DrawPos)),
            WhiteBrush,
            ESlateDrawEffect::None,
            FLinearColor(1.f, 1.f, 1.f, Star.Alpha * 0.5f)
        );
    }
    ++LayerId;

    TSet<int32> Downstream;
    TSet<int32> Upstream;

    const int32 ActiveNode = (PinnedNode != INDEX_NONE) ? PinnedNode : HoveredNode;
    if (ActiveNode != INDEX_NONE)
    {
        // Downstream dependencies
        TArray<int32> Stack = Nodes[ActiveNode].Dependencies;
        TSet<int32> Visited;
        for (int32 Dep : Stack)
        {
            Downstream.Add(Dep);
            Visited.Add(Dep);
        }
        while (Stack.Num() > 0)
        {
            int32 Cur = Stack.Pop();
            for (int32 Dep : Nodes[Cur].Dependencies)
            {
                if (!Visited.Contains(Dep))
                {
                    Downstream.Add(Dep);
                    Visited.Add(Dep);
                    Stack.Add(Dep);
                }
            }
        }

        // Upstream dependents
        Stack = Nodes[ActiveNode].Dependents;
        Visited.Empty();
        for (int32 Dep : Stack)
        {
            Upstream.Add(Dep);
            Visited.Add(Dep);
        }
        while (Stack.Num() > 0)
        {
            int32 Cur = Stack.Pop();
            for (int32 Dep : Nodes[Cur].Dependents)
            {
                if (!Visited.Contains(Dep))
                {
                    Upstream.Add(Dep);
                    Visited.Add(Dep);
                    Stack.Add(Dep);
                }
            }
        }

        Downstream.Add(ActiveNode);
    }

    TSet<int32> Highlight = Downstream;
    Highlight.Append(Upstream);

    // Draw edges with arrowheads pointing to dependencies. Node and text sizes
    // should follow the current zoom factor so zooming in enlarges them.
    const float ZoomScale = ZoomAmount;
    for (int32 i = 0; i < Nodes.Num(); ++i)
    {
        const FPluginNode& Node = Nodes[i];

        if (!Node.bActive || Node.AppearAlpha <= 0.01f)
        {
            continue;
        }

        const FVector2D NodePos = Center + ViewOffset + Node.Position * ZoomAmount;
        const float NodeRadius = ((Node.Name == TEXT("Root")) ? 60.f : 40.f) * ZoomScale * 0.5f;

        for (int32 Link : Node.Dependencies)
        {
            if (!Nodes.IsValidIndex(Link) || i == Link)
            {
                continue;
            }

            const float EdgeAlpha = FMath::Min(Node.AppearAlpha, Nodes[Link].AppearAlpha);
            if (!Nodes[Link].bActive || EdgeAlpha <= 0.01f)
            {
                continue;
            }

            const FPluginNode& DepNode = Nodes[Link];
            const FVector2D DepPos = Center + ViewOffset + DepNode.Position * ZoomAmount;
            const float DepRadius = ((DepNode.Name == TEXT("Root")) ? 60.f : 40.f) * ZoomScale * 0.5f;

            const FVector2D Delta = DepPos - NodePos;
            const float Dist = FMath::Max(Delta.Size(), 1.f);
            const FVector2D Dir = Delta / Dist;
            const FVector2D Start = NodePos + Dir * NodeRadius;
            const FVector2D End = DepPos - Dir * DepRadius;

            const bool bHighlighted = ActiveNode != INDEX_NONE && Highlight.Contains(i) && Highlight.Contains(Link);

            FLinearColor LineColor = FLinearColor::Gray;
            float Thickness = 1.f;

            if (bHighlighted)
            {
                LineColor = Nodes[i].Color;
                LineColor.A = Upstream.Contains(i) ? 0.3f : 1.f;
                Thickness = Upstream.Contains(i) ? 2.f : 4.f;
            }
            else
            {
                LineColor.A = 0.05f;
            }

            LineColor.A *= EdgeAlpha;
            // Line body
            TArray<FVector2D> LinePoints{Start, End};
            FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), LinePoints, ESlateDrawEffect::None, LineColor, true, Thickness);

            if (bHighlighted)
            {
                // Arrowhead uses the dependency color with upstream arrows dimmer
                FLinearColor ArrowColor = Nodes[Link].Color;
                ArrowColor.A = Upstream.Contains(i) ? 0.3f : 1.f;
                ArrowColor.A *= EdgeAlpha;

                const float ArrowSize = 8.f * ZoomScale;
                const FVector2D Perp(-Dir.Y, Dir.X);
                const FVector2D Tip = End;
                const FVector2D ArrowP1 = Tip - Dir * ArrowSize + Perp * ArrowSize * 0.5f;
                const FVector2D ArrowP2 = Tip - Dir * ArrowSize - Perp * ArrowSize * 0.5f;

                TArray<FVector2D> Arrow1{ArrowP1, Tip};
                TArray<FVector2D> Arrow2{ArrowP2, Tip};
                FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), Arrow1, ESlateDrawEffect::None, ArrowColor, true, Thickness);
                FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), Arrow2, ESlateDrawEffect::None, ArrowColor, true, Thickness);
            }
        }
    }

    // Draw nodes
    for (int32 i = 0; i < Nodes.Num(); ++i)
    {
        const FPluginNode& Node = Nodes[i];

        if (!Node.bActive || Node.AppearAlpha <= 0.01f)
        {
            continue;
        }

        const float BaseSize = 40.f;
        const float Ease = FMath::InterpEaseOut(0.f, 1.f, Node.AppearAlpha, 2.f);
        const float Size = BaseSize * ZoomScale * FMath::Lerp(0.2f, 1.f, Ease);
        FVector2D DrawPos = Center + ViewOffset + Node.Position * ZoomAmount - FVector2D(Size * 0.5f, Size * 0.5f);

        const UNsSpyglassSettings* Settings = UNsSpyglassSettings::GetSettings();
        FLinearColor BoxColor = Node.Color;
        if (Settings->bEnableImpactHeatmap)
        {
            const FLinearColor HeatTarget(1.f, 0.2f, 0.2f, BoxColor.A);
            BoxColor = FLinearColor::LerpUsingHSV(BoxColor, HeatTarget, Node.ImpactStrength);
        }

        if (Node.bIsEngine && Node.Name != TEXT("Root"))
        {
            FLinearColor LerpColor = FLinearColor::LerpUsingHSV(BoxColor, FLinearColor::White, 0.3f);
            LerpColor.A = BoxColor.A;
            BoxColor = LerpColor;
        }
        if (!Node.bIsEnabled)
        {
            BoxColor.A *= 0.4f;
        }
        const float BaseAlpha = Settings->bEnableImpactHeatmap ? 0.18f : 0.05f;
        if (Downstream.Contains(i))
        {
            BoxColor.A = Settings->bEnableImpactHeatmap ? 0.35f : 0.2f;
        }
        else if (Upstream.Contains(i))
        {
            BoxColor.A = Settings->bEnableImpactHeatmap ? 0.22f : 0.1f;
        }
        else
        {
            BoxColor.A = BaseAlpha;
        }

        const bool bOutlined = Highlight.Contains(i);
        FLinearColor OutlineColor = bOutlined ? Node.Color : FLinearColor::Transparent;
        if (bOutlined)
        {
            if (Upstream.Contains(i))
            {
                OutlineColor.A = 0.2f;
            }
            else if (Downstream.Contains(i))
            {
                OutlineColor.A = 1.0f;
            }
        }
        const float OutlineThickness = bOutlined ? 4.f : 0.f;

        FSlateRoundedBoxBrush CircleBrush(FLinearColor::White, Size * 0.5f, OutlineColor, OutlineThickness);
        FSlateDrawElement::MakeBox(
            OutDrawElements,
            LayerId + 1,
            AllottedGeometry.ToPaintGeometry(FVector2D(Size, Size), FSlateLayoutTransform(DrawPos)),
            &CircleBrush,
            ESlateDrawEffect::None,
            BoxColor
        );

        const FSlateFontInfo Font = FCoreStyle::Get().GetFontStyle("NormalFont");
        const TSharedRef<FSlateFontMeasure> Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();

        const bool bSplitName = Node.Name.Len() > 12;
        if (bSplitName)
        {
            FString ShortName;
            for (const TCHAR Ch : Node.Name)
            {
                if (FChar::IsUpper(Ch))
                {
                    ShortName.AppendChar(Ch);
                }
            }
            if (ShortName.IsEmpty())
            {
                ShortName = Node.Name.Left(2).ToUpper();
            }

            const FVector2D ShortSize = Measure->Measure(ShortName, Font);
            const FVector2D FullSize = Measure->Measure(Node.Name, Font);
            const float BaseScale = FMath::Min(1.f, (BaseSize - 8.f) / FMath::Max(ShortSize.X, FullSize.X));
            const float ShortScale = BaseScale * ZoomScale;
            const float FullScale = BaseScale * 0.6f * ZoomScale;
            const float TextAlpha = FMath::Clamp(ZoomAmount, 0.f, 1.f) * Node.AppearAlpha;

            const float TotalHeight = ShortSize.Y * ShortScale + FullSize.Y * FullScale;
            const float StartY = (Size - TotalHeight) * 0.5f;

            FVector2D Offset((Size - ShortSize.X * ShortScale) * 0.5f, StartY);
            FSlateDrawElement::MakeText(
                OutDrawElements,
                LayerId + 2,
                AllottedGeometry.ToPaintGeometry(ShortSize, FSlateLayoutTransform(ShortScale, DrawPos + Offset)),
                ShortName,
                Font,
                ESlateDrawEffect::None,
                FLinearColor(1.f, 1.f, 1.f, TextAlpha)
            );

            Offset.X = (Size - FullSize.X * FullScale) * 0.5f;
            Offset.Y = StartY + ShortSize.Y * ShortScale;
            FSlateDrawElement::MakeText(
                OutDrawElements,
                LayerId + 2,
                AllottedGeometry.ToPaintGeometry(FullSize, FSlateLayoutTransform(FullScale, DrawPos + Offset)),
                Node.Name,
                Font,
                ESlateDrawEffect::None,
                FLinearColor(1.f, 1.f, 1.f, TextAlpha)
            );
        }
        else
        {
            const FVector2D TextSize = Measure->Measure(Node.Name, Font);
            const float BaseScale = FMath::Min(1.f, (BaseSize - 8.f) / TextSize.X);
            const float TextScale = BaseScale * ZoomScale;
            const float TextAlpha = FMath::Clamp(ZoomAmount, 0.f, 1.f) * Node.AppearAlpha;
            const FVector2D Offset((Size - TextSize.X * TextScale) * 0.5f, (Size - TextSize.Y * TextScale) * 0.5f);

            FSlateDrawElement::MakeText(
                OutDrawElements,
                LayerId + 2,
                AllottedGeometry.ToPaintGeometry(TextSize, FSlateLayoutTransform(TextScale, DrawPos + Offset)),
                Node.Name,
                Font,
                ESlateDrawEffect::None,
                FLinearColor(1.f, 1.f, 1.f, TextAlpha)
            );
        }
    }

    return LayerId + 3;
}

FReply SNsSpyglassGraphWidget::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
    const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());

    if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        int32 Hit = HitTestNode(LocalPos, MyGeometry.GetLocalSize());
        if (Hit != INDEX_NONE)
        {
            bIsDragging = true;
            DraggedNode = Hit;
            Nodes[Hit].bFixed = true;
            Nodes[Hit].Velocity = FVector2D::ZeroVector;
            LastMousePos = LocalPos;
            DragStartPos = LocalPos;
            bPendingPinClick = true;
            bHasDragged = false;
            return FReply::Handled().CaptureMouse(SharedThis(this));
        }

        bIsPanning = true;
        LastMousePos = MouseEvent.GetScreenSpacePosition();
        return FReply::Handled().CaptureMouse(SharedThis(this));
    }
    else if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
    {
        bIsPanning = true;
        LastMousePos = MouseEvent.GetScreenSpacePosition();
        return FReply::Handled().CaptureMouse(SharedThis(this));
    }

    return FReply::Unhandled();
}

FReply SNsSpyglassGraphWidget::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
    if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
        const int32 Hit = HitTestNode(LocalPos, MyGeometry.GetLocalSize());
        if (bIsDragging && Nodes.IsValidIndex(DraggedNode))
        {
            Nodes[DraggedNode].bFixed = false;
            Nodes[DraggedNode].Velocity = FVector2D::ZeroVector;
        }

        if (bPendingPinClick && !bHasDragged && Hit != INDEX_NONE)
        {
            if (PinnedNode == Hit)
            {
                PinnedNode = INDEX_NONE;
                OnNodePinned.ExecuteIfBound(nullptr);
                if (HoveredNode != INDEX_NONE)
                {
                    OnNodeHovered.ExecuteIfBound(Nodes[HoveredNode].Plugin);
                }
                else
                {
                    OnNodeHovered.ExecuteIfBound(nullptr);
                }
            }
            else
            {
                PinnedNode = Hit;
                OnNodePinned.ExecuteIfBound(Nodes[Hit].Plugin);
            }
        }

        bPendingPinClick = false;
        bHasDragged = false;
        bIsDragging = false;
        DraggedNode = INDEX_NONE;
        bIsPanning = false;
        return FReply::Handled().ReleaseMouseCapture();
    }
    else if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
    {
        bIsPanning = false;
        return FReply::Handled().ReleaseMouseCapture();
    }

    return FReply::Unhandled();
}

FReply SNsSpyglassGraphWidget::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
    const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());

    if (bIsDragging && Nodes.IsValidIndex(DraggedNode))
    {
        if (!bHasDragged && (LocalPos - DragStartPos).SizeSquared() > 9.f)
        {
            bHasDragged = true;
            bPendingPinClick = false;
        }
        const FVector2D Delta = (LocalPos - LastMousePos) / ZoomAmount;
        Nodes[DraggedNode].Position += Delta;
        LastMousePos = LocalPos;
        return FReply::Handled();
    }
    else if (bIsPanning)
    {
        const FVector2D Delta = MouseEvent.GetScreenSpacePosition() - LastMousePos;
        ViewOffset += Delta;
        LastMousePos = MouseEvent.GetScreenSpacePosition();
        return FReply::Handled();
    }

    const int32 NewHover = HitTestNode(LocalPos, MyGeometry.GetLocalSize());
    if (HoveredNode != NewHover)
    {
        HoveredNode = NewHover;
        if (HoveredNode != INDEX_NONE)
        {
            SetToolTipText(FText::FromString(Nodes[HoveredNode].Name));
            if (PinnedNode == INDEX_NONE)
            {
                OnNodeHovered.ExecuteIfBound(Nodes[HoveredNode].Plugin);
            }
        }
        else
        {
            SetToolTipText(FText());
            if (PinnedNode == INDEX_NONE)
            {
                OnNodeHovered.ExecuteIfBound(nullptr);
            }
        }
    }

    return FReply::Unhandled();
}

FReply SNsSpyglassGraphWidget::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
    const FVector2D LocalMousePos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
    const FVector2D Center = MyGeometry.GetLocalSize() * 0.5f;

    const float OldZoom = ZoomAmount;
    ZoomAmount *= FMath::Pow(1.1f, MouseEvent.GetWheelDelta());
    ZoomAmount = FMath::Clamp(ZoomAmount, 0.2f, 10.f);

    const float ZoomRatio = ZoomAmount / OldZoom;
    ViewOffset = LocalMousePos - Center - (LocalMousePos - Center - ViewOffset) * ZoomRatio;

    return FReply::Handled();
}

void SNsSpyglassGraphWidget::RecenterView()
{
    ViewOffset = FVector2D::ZeroVector;
    ZoomAmount = 0.75f;
}

void SNsSpyglassGraphWidget::RebuildGraph()
{
    BuildNodes(FVector2D(960.f, 540.f));
    RecenterView();
    PinnedNode = INDEX_NONE;
    OnNodePinned.ExecuteIfBound(nullptr);
}

void SNsSpyglassGraphWidget::SetOnNodeHovered(FOnNodeHovered InDelegate)
{
    OnNodeHovered = InDelegate;
}

void SNsSpyglassGraphWidget::SetOnNodePinned(FOnNodePinned InDelegate)
{
    OnNodePinned = InDelegate;
}

void SNsSpyglassGraphWidget::RunForceAtlas2Step(TArray<FPluginNode>& InNodes, int32 InRootIndex, float Repulsion, float Gravity, float DeltaTime)
{
    const int32 Num = InNodes.Num();
    if (Num <= 1) return;

    const float MinDist = 25.f;

    // Feel tuning
    const float Damping  = 0.90f;
    const float MaxSpeed = 1000.f;
    const float SimSpeed = bIsDragging ? 60.f : 40.f;

    const float EdgeStrength = UNsSpyglassSettings::GetSettings()->AttractionScale;
    const float RestLength = 140.f;   // edges only pull when stretched

    TArray<float> Mass;
    Mass.SetNumUninitialized(Num);
    for (int32 i = 0; i < Num; ++i)
    {
        Mass[i] = 1.f + InNodes[i].Links.Num();
    }

    TArray<FVector2D> Force;
    Force.Init(FVector2D::ZeroVector, Num);

    // --- Repulsion (pairwise) ---
    for (int32 i = 0; i < Num; ++i)
    {
        if (!InNodes[i].bActive) continue;

        for (int32 j = i + 1; j < Num; ++j)
        {
            if (!InNodes[j].bActive) continue;

            FVector2D Delta = InNodes[i].Position - InNodes[j].Position;

            float DistSqr = Delta.SizeSquared();
            DistSqr = FMath::Max(DistSqr, MinDist * MinDist);

            const float Dist = FMath::Sqrt(DistSqr);
            const FVector2D Dir = Delta / Dist;

            const float F = Repulsion * Mass[i] * Mass[j] / DistSqr;

            Force[i] += Dir * F;
            Force[j] -= Dir * F;
        }
    }

    // --- Attraction (edges) ---
    for (int32 i = 0; i < Num; ++i)
    {
        if (!InNodes[i].bActive) continue;

        for (int32 Link : InNodes[i].Links)
        {
            if (!InNodes.IsValidIndex(Link)) continue;
            if (!InNodes[Link].bActive) continue;

            // critical: process each undirected edge once
            if (Link <= i) continue;

            FVector2D Delta = InNodes[i].Position - InNodes[Link].Position;

            float Dist = Delta.Size();
            Dist = FMath::Max(Dist, MinDist);

            const FVector2D Dir = Delta / Dist;

            // only pull when stretched past rest length
            const float Stretch = FMath::Max(0.f, Dist - RestLength);
            const float F = Stretch * EdgeStrength;

            Force[i] -= Dir * F;
            Force[Link] += Dir * F;
        }
    }

    // --- Gravity (toward origin) ---
    for (int32 i = 0; i < Num; ++i)
    {
        if (!InNodes[i].bActive) continue;
        Force[i] -= InNodes[i].Position * Gravity * Mass[i];
    }

    // --- Integrate with damping ---
    for (int32 i = 0; i < Num; ++i)
    {
        if (!InNodes[i].bActive) continue;
        if (i == InRootIndex || InNodes[i].bFixed) continue;

        const FVector2D Accel = Force[i] / Mass[i];

        InNodes[i].Velocity += Accel * DeltaTime * SimSpeed;
        InNodes[i].Velocity *= Damping;
        InNodes[i].Velocity = ClampToMaxSize2D(InNodes[i].Velocity, MaxSpeed);

        InNodes[i].Position += InNodes[i].Velocity * DeltaTime;
    }
}

FVector2D SNsSpyglassGraphWidget::ClampToMaxSize2D(const FVector2D& V, float MaxSize)
{
    const float MaxSqr = MaxSize * MaxSize;
    const float Sqr = V.SizeSquared();

    if (Sqr <= MaxSqr || Sqr <= KINDA_SMALL_NUMBER)
    {
        return V;
    }

    return V * (MaxSize / FMath::Sqrt(Sqr));
}

void SNsSpyglassGraphWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
    const UNsSpyglassSettings* Settings = UNsSpyglassSettings::GetSettings();
    const float Delta = FMath::Min(InDeltaTime, 0.05f);

    if (bIntroRunning)
    {
        IntroElapsed += Delta;

        int32 FullyVisible = 0;

        for (FPluginNode& N : Nodes)
        {
            const float T = (IntroElapsed - N.AppearDelay) / FMath::Max(0.001f, IntroFade);
            const float NewAlpha = FMath::Clamp(T, 0.f, 1.f);

            // Activate node when it starts appearing
            if (!N.bActive && NewAlpha > 0.f)
            {
                N.bActive = true;
                N.Velocity = FVector2D::ZeroVector;
            }

            N.AppearAlpha = NewAlpha;

            if (N.AppearAlpha >= 1.f)
            {
                ++FullyVisible;
            }
        }

        if (FullyVisible == Nodes.Num())
        {
            bIntroRunning = false;
        }
    }

    RunForceAtlas2Step(Nodes, RootIndex, Settings->Repulsion * 100.f, Settings->CenterForce, Delta);

    for (FBackgroundStar& Star : Stars)
    {
        Star.Alpha = FMath::FInterpTo(Star.Alpha, Star.TargetAlpha, Delta, Star.FadeSpeed);
        if (FMath::IsNearlyEqual(Star.Alpha, Star.TargetAlpha, 0.01f))
        {
            if (Star.TargetAlpha > 0.f)
            {
                Star.TargetAlpha = 0.f;
            }
            else
            {
                Star.Position.X = FMath::FRandRange(-StarsViewSize.X, StarsViewSize.X);
                Star.Position.Y = FMath::FRandRange(-StarsViewSize.Y, StarsViewSize.Y);
                Star.TargetAlpha = FMath::FRandRange(0.2f, 1.f);
                Star.FadeSpeed = FMath::FRandRange(0.5f, 1.5f);
            }
        }
    }
}
