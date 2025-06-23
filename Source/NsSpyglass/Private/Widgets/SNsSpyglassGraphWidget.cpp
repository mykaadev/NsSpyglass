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
}

void SNsSpyglassGraphWidget::BuildNodes(const FVector2D& ViewSize) const
{
    Nodes.Reset();

    const TArray<TSharedRef<IPlugin>>& Plugins = IPluginManager::Get().GetEnabledPlugins();

    TMap<FString, int32> NameToIndex;
    TMap<FString, FLinearColor> CategoryColors;

    // Root node stays in the middle
    if (UNsSpyglassSettings::GetSettings()->bZenMode)
    {
        FPluginNode RootNode;
        RootNode.Name = TEXT("Root");
        RootNode.Position = FVector2D::ZeroVector;
        RootNode.Color = FLinearColor(0.8f, 0.2f, 0.2f, 0.1f);
        RootNode.bFixed = true;
        RootIndex = Nodes.Add(RootNode);
        NameToIndex.Add(RootNode.Name, RootIndex);
    }

    // Create nodes for plugins
    for (const TSharedRef<IPlugin>& Plugin : Plugins)
    {
        FPluginNode Node;
        Node.Name = Plugin->GetName();
        Node.Plugin = Plugin;
        Node.bIsEngine = Plugin->GetLoadedFrom() == EPluginLoadedFrom::Engine;
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
    }

    // Fill in links
    for (int32 i = 1; i < Nodes.Num(); ++i)
    {
        const TSharedRef<IPlugin>& Plugin = Plugins[i - 1];
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

        // If the plugin has no dependencies, connect it to the root
        // so the root appears as an upstream dependency.
        if (UNsSpyglassSettings::GetSettings()->bZenMode)
        {
            if (Nodes[i].Dependencies.Num() == 0)
            {
                Nodes[i].Links.AddUnique(RootIndex);
                Nodes[RootIndex].Links.AddUnique(i);
                // The plugin depends on the root, so the arrow points to the root.
                Nodes[i].Dependencies.AddUnique(RootIndex);
                Nodes[RootIndex].Dependents.AddUnique(i);
            }
        }
    }

    // Arrange nodes in a circle to avoid overlapping at the origin
    if (Nodes.Num() > 1)
    {
        const float Radius = 200.f;
        const float Step = 2.f * PI / static_cast<float>(Nodes.Num() - 1);
        for (int32 i = 1; i < Nodes.Num(); ++i)
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

    if (HoveredNode != INDEX_NONE)
    {
        // Downstream dependencies
        TArray<int32> Stack = Nodes[HoveredNode].Dependencies;
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
        Stack = Nodes[HoveredNode].Dependents;
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

        Downstream.Add(HoveredNode);
    }

    TSet<int32> Highlight = Downstream;
    Highlight.Append(Upstream);

    // Draw edges with arrowheads pointing to dependencies. Node and text sizes
    // should follow the current zoom factor so zooming in enlarges them.
    const float ZoomScale = ZoomAmount;
    for (int32 i = 0; i < Nodes.Num(); ++i)
    {
        const FPluginNode& Node = Nodes[i];
        const FVector2D NodePos = Center + ViewOffset + Node.Position * ZoomAmount;
        const float NodeRadius = ((Node.Name == TEXT("Root")) ? 60.f : 40.f) * ZoomScale * 0.5f;

        for (int32 Link : Node.Dependencies)
        {
            if (!Nodes.IsValidIndex(Link) || i == Link)
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

            const bool bHighlighted = HoveredNode != INDEX_NONE && Highlight.Contains(i) && Highlight.Contains(Link);

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

            // Line body
            TArray<FVector2D> LinePoints{Start, End};
            FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), LinePoints, ESlateDrawEffect::None, LineColor, true, Thickness);

            if (bHighlighted)
            {
                // Arrowhead uses the dependency color with upstream arrows dimmer
                FLinearColor ArrowColor = Nodes[Link].Color;
                ArrowColor.A = Upstream.Contains(i) ? 0.3f : 1.f;
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
        const float BaseSize = Node.Name == TEXT("Root") ? 60.f : 40.f;
        const float Size = BaseSize * ZoomScale;
        FVector2D DrawPos = Center + ViewOffset + Node.Position * ZoomAmount - FVector2D(Size * 0.5f, Size * 0.5f);

        FLinearColor BoxColor = Node.Color;
        if (Node.bIsEngine && Node.Name != TEXT("Root"))
        {
            FLinearColor LerpColor = FLinearColor::LerpUsingHSV(BoxColor, FLinearColor::White, 0.3f);
            LerpColor.A = BoxColor.A;
            BoxColor = LerpColor;
        }
        if (Downstream.Contains(i))
        {
            BoxColor.A = 0.2f;
        }
        else if (Upstream.Contains(i))
        {
            BoxColor.A = 0.1f;
        }
        else
        {
            BoxColor.A = 0.05f;
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
        const FVector2D TextSize = FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Node.Name, Font);

        // Scale text with zoom so names remain legible when zoomed in.
        const float BaseScale = FMath::Min(1.f, (BaseSize - 8.f) / TextSize.X);
        const float TextScale = BaseScale * ZoomScale;
        const float TextAlpha = FMath::Clamp(ZoomAmount, 0.f, 1.f);
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
            LastMousePos = LocalPos;
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
        if (bIsDragging && Nodes.IsValidIndex(DraggedNode))
        {
            Nodes[DraggedNode].bFixed = false;
        }
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
            OnNodeHovered.ExecuteIfBound(Nodes[HoveredNode].Plugin);
        }
        else
        {
            SetToolTipText(FText());
            OnNodeHovered.ExecuteIfBound(nullptr);
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
    ZoomAmount = 1.f;
}

void SNsSpyglassGraphWidget::RebuildGraph()
{
    BuildNodes(FVector2D(960.f, 540.f));
    RecenterView();
}

void SNsSpyglassGraphWidget::SetOnNodeHovered(FOnNodeHovered InDelegate)
{
    OnNodeHovered = InDelegate;
}

void SNsSpyglassGraphWidget::RunForceAtlas2Step(TArray<FPluginNode>& InNodes, int32 InRootIndex, float Repulsion, float Gravity, float DeltaTime)
{
    const int32 Num = InNodes.Num();
    if (Num <= 1)
    {
        return;
    }

    TArray<float> Mass;
    Mass.SetNumUninitialized(Num);
    for (int32 i = 0; i < Num; ++i)
    {
        Mass[i] = 1.f + InNodes[i].Links.Num();
    }

    TArray<FVector2D> Displacement;
    Displacement.Init(FVector2D::ZeroVector, Num);

    // Repulsion forces
    for (int32 i = 0; i < Num; ++i)
    {
        for (int32 j = i + 1; j < Num; ++j)
        {
            FVector2D Delta = InNodes[i].Position - InNodes[j].Position;
            float DistSqr = Delta.SizeSquared();
            if (DistSqr < KINDA_SMALL_NUMBER)
            {
                Delta = FVector2D(FMath::FRand() - 0.5f, FMath::FRand() - 0.5f);
                DistSqr = KINDA_SMALL_NUMBER;
            }
            Delta /= FMath::Sqrt(DistSqr);
            const float Force = Repulsion * Mass[i] * Mass[j] / DistSqr;
            Displacement[i] += Delta * Force;
            Displacement[j] -= Delta * Force;
        }
    }

    // Attraction along edges
    for (int32 i = 0; i < Num; ++i)
    {
        for (const int32 Link : InNodes[i].Links)
        {
            if (!InNodes.IsValidIndex(Link))
            {
                continue;
            }

            FVector2D Delta = InNodes[i].Position - InNodes[Link].Position;
            float Dist = Delta.Size();
            if (Dist < KINDA_SMALL_NUMBER)
            {
                Delta = FVector2D(FMath::FRand() - 0.5f, FMath::FRand() - 0.5f);
                Dist = KINDA_SMALL_NUMBER;
            }
            Delta /= Dist;

            if (UNsSpyglassSettings::GetSettings()->bZenMode)
            {
                // Attraction is proportional to distance
                Displacement[i] -= Delta * Dist;
                Displacement[Link] += Delta * Dist;
            }
            else
            {
                const float AttractionScale = UNsSpyglassSettings::GetSettings()->AttractionScale;
                Displacement[i] -= Delta * FMath::Loge(Dist) * AttractionScale;
                Displacement[Link] += Delta * FMath::Loge(Dist) * AttractionScale;
            }
        }
    }

    // Gravity to keep graph centered
    for (int32 i = 0; i < Num; ++i)
    {
        Displacement[i] -= InNodes[i].Position * Gravity * Mass[i];
    }

    const float Step = DeltaTime * 5.f;
    for (int32 i = 0; i < Num; ++i)
    {
        if (i == InRootIndex || InNodes[i].bFixed)
        {
            continue; // keep anchored InNodes stationary
        }

        InNodes[i].Position += (Displacement[i] / Mass[i]) * Step;
    }
}

void SNsSpyglassGraphWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
    const UNsSpyglassSettings* Settings = UNsSpyglassSettings::GetSettings();
    const float Delta = FMath::Min(InDeltaTime, 0.05f);
    RunForceAtlas2Step(Nodes, RootIndex, Settings->Repulsion, Settings->CenterForce, Delta);

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

