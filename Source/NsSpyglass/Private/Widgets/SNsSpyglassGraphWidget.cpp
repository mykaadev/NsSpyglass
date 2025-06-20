#include "Widgets/SNsSpyglassGraphWidget.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Interfaces/IPluginManager.h"
#include "Math/RandomStream.h"
#include "Rendering/DrawElements.h"
#include "Settings/NsSpyglassSettings.h"

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

    FRandomStream Rand(12345);

    const TArray<TSharedRef<IPlugin>>& Plugins = IPluginManager::Get().GetEnabledPlugins();

    TMap<FString, int32> NameToIndex;
    TMap<FString, FLinearColor> CategoryColors;

    // Root node stays in the middle
    FPluginNode RootNode;
    RootNode.Name = TEXT("Root");
    RootNode.Position = FVector2D::ZeroVector;
    RootNode.bFixed = true;
    RootNode.Color = FLinearColor(0.8f, 0.2f, 0.2f, 0.1f);
    RootIndex = Nodes.Add(RootNode);
    NameToIndex.Add(RootNode.Name, RootIndex);

    // Create nodes for plugins
    for (const TSharedRef<IPlugin>& Plugin : Plugins)
    {
        FPluginNode Node;
        const FVector RandVector = Rand.VRand() * 200.f;

        Node.Name = Plugin->GetName();
        Node.bIsEngine = Plugin->GetLoadedFrom() == EPluginLoadedFrom::Engine;
        Node.Position = FVector2D(RandVector.X, RandVector.Y);

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
                    Nodes[i].Links.Add(*DepIdx);
                }
            }
        }

        // Link to root so everything stays connected
        Nodes[i].Links.Add(RootIndex);
        Nodes[RootIndex].Links.Add(i);
    }

    // Cluster nodes that are linked together so they start near each other
    TArray<int32> Component;
    Component.Init(INDEX_NONE, Nodes.Num());
    int32 CompCount = 0;
    for (int32 i = 1; i < Nodes.Num(); ++i)
    {
        if (Component[i] != INDEX_NONE)
        {
            continue;
        }
        TArray<int32> Stack;
        Stack.Add(i);
        Component[i] = CompCount;

        while (Stack.Num() > 0)
        {
            int32 Cur = Stack.Pop();
            for (int32 Link : Nodes[Cur].Links)
            {
                if (Link == RootIndex)
                {
                    continue;
                }
                if (Component[Link] == INDEX_NONE)
                {
                    Component[Link] = CompCount;
                    Stack.Add(Link);
                }
            }
        }
        ++CompCount;
    }

    const float ClusterSpacing = 150.f;
    const float ClusterSpread = 80.f;
    for (int32 c = 0; c < CompCount; ++c)
    {
        float Radius = ClusterSpacing * FMath::Sqrt(static_cast<float>(c));
        float Angle = Rand.FRandRange(0.f, 2.f * PI);
        FVector2D CenterOffset(Radius * FMath::Cos(Angle), Radius * FMath::Sin(Angle));
        for (int32 i = 1; i < Nodes.Num(); ++i)
        {
            if (Component[i] == c)
            {
                FVector RandOffset = Rand.VRand() * ClusterSpread;
                Nodes[i].Position = CenterOffset + FVector2D(RandOffset.X, RandOffset.Y);
            }
        }
    }
}

int32 SNsSpyglassGraphWidget::HitTestNode(const FVector2D& LocalPos, const FVector2D& ViewSize) const
{
    const FVector2D Center = ViewSize * 0.5f;
    for (int32 i = 0; i < Nodes.Num(); ++i)
    {
        float Size = Nodes[i].Name == TEXT("Root") ? 60.f : 40.f;
        FVector2D NodePos = Center + ViewOffset + Nodes[i].Position * ZoomAmount;
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

    const FVector2D Center = AllottedGeometry.GetLocalSize() * 0.5f;

    TSet<int32> Highlight;
    if (HoveredNode != INDEX_NONE)
    {
        TArray<int32> Stack;
        TSet<int32> Visited;
        Highlight.Add(HoveredNode);
        Stack.Add(HoveredNode);
        Visited.Add(HoveredNode);

        while (Stack.Num() > 0)
        {
            int32 Cur = Stack.Pop();

            if (Cur == RootIndex)
            {
                continue; // do not traverse beyond the root
            }

            // Forward links (dependencies)
            for (int32 Link : Nodes[Cur].Links)
            {
                if (!Visited.Contains(Link))
                {
                    Highlight.Add(Link);
                    Visited.Add(Link);
                    Stack.Add(Link);
                }
            }

            // Reverse links (dependents)
            for (int32 i = 0; i < Nodes.Num(); ++i)
            {
                if (Nodes[i].Links.Contains(Cur) && !Visited.Contains(i))
                {
                    Highlight.Add(i);
                    Visited.Add(i);
                    Stack.Add(i);
                }
            }
        }
    }

    // Draw edges with arrowheads pointing to dependencies
    const float ZoomScale = FMath::Clamp(FMath::Sqrt(ZoomAmount), 0.5f, 1.5f);
    for (int32 i = 0; i < Nodes.Num(); ++i)
    {
        const FPluginNode& Node = Nodes[i];
        const FVector2D NodePos = Center + ViewOffset + Node.Position * ZoomAmount;
        const float NodeRadius = ((Node.Name == TEXT("Root")) ? 60.f : 40.f) * ZoomScale * 0.5f;

        for (int32 Link : Node.Links)
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

            bool bConnected = Highlight.Contains(i) && Highlight.Contains(Link);
            FLinearColor Color = FLinearColor::Gray;
            float Thickness = 1.f;
            if (HoveredNode != INDEX_NONE)
            {
                if (bConnected)
                {
                    Thickness = 3.f;
                }
                else
                {
                    Color.A = 0.1f;
                }
            }

            // Line body
            TArray<FVector2D> LinePoints{Start, End};
            FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), LinePoints, ESlateDrawEffect::None, Color, true, Thickness);

            // Arrowhead
            const float ArrowSize = 8.f * ZoomScale;
            const FVector2D Perp(-Dir.Y, Dir.X);
            const FVector2D Tip = End;
            const FVector2D ArrowP1 = Tip - Dir * ArrowSize + Perp * ArrowSize * 0.5f;
            const FVector2D ArrowP2 = Tip - Dir * ArrowSize - Perp * ArrowSize * 0.5f;

            TArray<FVector2D> Arrow1{ArrowP1, Tip};
            TArray<FVector2D> Arrow2{ArrowP2, Tip};
            FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), Arrow1, ESlateDrawEffect::None, Color, true, Thickness);
            FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), Arrow2, ESlateDrawEffect::None, Color, true, Thickness);
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
        if (Highlight.Contains(i))
        {
            BoxColor.A = 0.2f;
        }
        else
        {
            BoxColor.A = 0.1f;
        }

        const bool bOutlined = Highlight.Contains(i);
        const FLinearColor OutlineColor = bOutlined ? FLinearColor(1.f, 0.5f, 0.f) : FLinearColor::Transparent;
        const float OutlineThickness = bOutlined ? 2.f : 0.f;

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
        const float Scale = FMath::Min(1.f, (Size - 8.f) / TextSize.X);
        const FVector2D Offset((Size - TextSize.X * Scale) * 0.5f, (Size - TextSize.Y * Scale) * 0.5f);

        FSlateDrawElement::MakeText(
            OutDrawElements,
            LayerId + 2,
            AllottedGeometry.ToPaintGeometry(TextSize, FSlateLayoutTransform(Scale, DrawPos + Offset)),
            Node.Name,
            Font,
            ESlateDrawEffect::None,
            FLinearColor::White
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
            return FReply::Handled();
        }
        else
        {
            bIsPanning = true;
            LastMousePos = MouseEvent.GetScreenSpacePosition();
            return FReply::Handled();
        }
    }
    else if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
    {
        bIsPanning = true;
        LastMousePos = MouseEvent.GetScreenSpacePosition();
        return FReply::Handled();
    }

    return FReply::Unhandled();
}

FReply SNsSpyglassGraphWidget::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
    if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bIsDragging)
    {
        if (Nodes.IsValidIndex(DraggedNode))
        {
            Nodes[DraggedNode].bFixed = false;
        }
        bIsDragging = false;
        DraggedNode = INDEX_NONE;
    }

    if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
    {
        bIsPanning = false;
    }

    return FReply::Handled();
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

    HoveredNode = HitTestNode(LocalPos, MyGeometry.GetLocalSize());
    if (HoveredNode != INDEX_NONE)
    {
        SetToolTipText(FText::FromString(Nodes[HoveredNode].Name));
    }
    else
    {
        SetToolTipText(FText());
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

void SNsSpyglassGraphWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
    if (Nodes.Num() == 0)
    {
        return;
    }

    TArray<FVector2D> Forces;
    Forces.Init(FVector2D::ZeroVector, Nodes.Num());

    const UNsSpyglassSettings* const Settings = UNsSpyglassSettings::GetSettings();
    const float Repulsion = Settings->Repulsion;
    const float SpringLength = Settings->SpringLength;
    const float SpringStiffness = Settings->SpringStiffness;
    const float MaxLinkDistance = Settings->MaxLinkDistance;
    const float CenterForce = Settings->CenterForce;

    for (int32 i = 0; i < Nodes.Num(); ++i)
    {
        for (int32 j = i + 1; j < Nodes.Num(); ++j)
        {
            if (Nodes[i].Links.Contains(j) || Nodes[j].Links.Contains(i))
            {
                continue;
            }

            FVector2D Delta = Nodes[j].Position - Nodes[i].Position;
            const float DistSq = FMath::Max(Delta.SizeSquared(), 1.f);
            const FVector2D Dir = Delta / FMath::Sqrt(DistSq);
            const FVector2D Rep = Dir * (Repulsion / DistSq);
            Forces[i] -= Rep;
            Forces[j] += Rep;
        }
    }

    for (int32 i = 0; i < Nodes.Num(); ++i)
    {
        for (const int32 Link : Nodes[i].Links)
        {
            if (Link < 0 || Link >= Nodes.Num() || Link <= i)
            {
                continue;
            }

            const FVector2D Delta = Nodes[Link].Position - Nodes[i].Position;
            const float Dist = FMath::Max(Delta.Size(), 1.f);
            const FVector2D Dir = Delta / Dist;
            const FVector2D Spring = Dir * (Dist - SpringLength) * SpringStiffness;
            Forces[i] += Spring;
            Forces[Link] -= Spring;
        }
    }

    for (int32 i = 0; i < Nodes.Num(); ++i)
    {
        Forces[i] += -Nodes[i].Position * CenterForce;
    }

    for (int32 i = 0; i < Nodes.Num(); ++i)
    {
        if (!Nodes[i].bFixed)
        {
            Nodes[i].Velocity += Forces[i] * InDeltaTime;
            Nodes[i].Velocity *= 0.8f;
            Nodes[i].Position += Nodes[i].Velocity * InDeltaTime;
        }
    }

    // Clamp the distance between linked nodes to avoid them drifting too far apart
    for (int32 i = 0; i < Nodes.Num(); ++i)
    {
        for (const int32 Link : Nodes[i].Links)
        {
            if (Link < 0 || Link >= Nodes.Num() || Link <= i)
            {
                continue;
            }

            const FVector2D Delta = Nodes[Link].Position - Nodes[i].Position;
            const float Dist = Delta.Size();
            if (Dist > MaxLinkDistance)
            {
                const FVector2D Dir = Delta / Dist;
                const FVector2D Adjust = Dir * (Dist - MaxLinkDistance) * 0.5f;
                if (!Nodes[i].bFixed)
                {
                    Nodes[i].Position += Adjust;
                }
                if (!Nodes[Link].bFixed)
                {
                    Nodes[Link].Position -= Adjust;
                }
            }
        }
    }
}

