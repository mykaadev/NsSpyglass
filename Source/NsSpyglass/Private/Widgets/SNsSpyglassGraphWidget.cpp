#include "Widgets/SNsSpyglassGraphWidget.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Interfaces/IPluginManager.h"
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

    const TArray<TSharedRef<IPlugin>>& Plugins = IPluginManager::Get().GetEnabledPlugins();

    TMap<FString, int32> NameToIndex;
    TMap<FString, FLinearColor> CategoryColors;

    // Root node stays in the middle
    FPluginNode RootNode;
    RootNode.Name = TEXT("Root");
    RootNode.Position = FVector2D::ZeroVector;
    RootNode.Color = FLinearColor(0.8f, 0.2f, 0.2f, 0.1f);
    RootIndex = Nodes.Add(RootNode);
    NameToIndex.Add(RootNode.Name, RootIndex);

    // Create nodes for plugins
    for (const TSharedRef<IPlugin>& Plugin : Plugins)
    {
        FPluginNode Node;
        Node.Name = Plugin->GetName();
        Node.Plugin = Plugin;
        Node.bIsEngine = Plugin->GetLoadedFrom() == EPluginLoadedFrom::Engine;
        Node.Position = FVector2D::ZeroVector;

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
                    Nodes[*DepIdx].Links.AddUnique(i); // treat links as undirected
                }
            }
        }

        // Link to the root so everything stays connected
        Nodes[i].Links.AddUnique(RootIndex);
        Nodes[RootIndex].Links.AddUnique(i);
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

            const bool bDirectHover = HoveredNode != INDEX_NONE && (i == HoveredNode || Link == HoveredNode);

            FLinearColor LineColor = FLinearColor::Gray;
            float Thickness = 1.f;

            if (HoveredNode != INDEX_NONE)
            {
                if (bDirectHover)
                {
                    LineColor = Nodes[HoveredNode].Color;
                    LineColor.A = 1.f;
                    Thickness = 3.f;
                }
                else
                {
                    LineColor.A = 0.1f;
                }
            }

            // Line body
            TArray<FVector2D> LinePoints{Start, End};
            FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), LinePoints, ESlateDrawEffect::None, LineColor, true, Thickness);

            // Arrowhead uses color of dependency
            FLinearColor ArrowColor = Nodes[Link].Color;
            if (HoveredNode != INDEX_NONE && !bDirectHover)
            {
                ArrowColor.A = 0.1f;
            }

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
        FLinearColor OutlineColor = bOutlined ? Node.Color : FLinearColor::Transparent;
        if (bOutlined)
        {
            OutlineColor.A = 1.f;
        }
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
    if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton ||
        MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
    {
        bIsPanning = true;
        LastMousePos = MouseEvent.GetScreenSpacePosition();
        return FReply::Handled();
    }

    return FReply::Unhandled();
}

FReply SNsSpyglassGraphWidget::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
    if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton ||
        MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
    {
        bIsPanning = false;
        return FReply::Handled();
    }

    return FReply::Unhandled();
}

FReply SNsSpyglassGraphWidget::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
    const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());

    if (bIsPanning)
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

namespace
{
    /**
     * Perform a single ForceAtlas2 iteration on the node array.
     */
    void RunForceAtlas2Step(TArray<FPluginNode>& Nodes, int32 RootIndex, float Repulsion, float Gravity, float DeltaTime)
    {
        const int32 Num = Nodes.Num();
        if (Num <= 1)
        {
            return;
        }

        TArray<float> Mass;
        Mass.SetNumUninitialized(Num);
        for (int32 i = 0; i < Num; ++i)
        {
            Mass[i] = 1.f + Nodes[i].Links.Num();
        }

        TArray<FVector2D> Displacement;
        Displacement.Init(FVector2D::ZeroVector, Num);

        // Repulsion forces
        for (int32 i = 0; i < Num; ++i)
        {
            for (int32 j = i + 1; j < Num; ++j)
            {
                FVector2D Delta = Nodes[i].Position - Nodes[j].Position;
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
            for (int32 Link : Nodes[i].Links)
            {
                if (!Nodes.IsValidIndex(Link))
                {
                    continue;
                }

                FVector2D Delta = Nodes[i].Position - Nodes[Link].Position;
                float Dist = Delta.Size();
                if (Dist < KINDA_SMALL_NUMBER)
                {
                    Delta = FVector2D(FMath::FRand() - 0.5f, FMath::FRand() - 0.5f);
                    Dist = KINDA_SMALL_NUMBER;
                }
                Delta /= Dist;

                // Attraction is proportional to distance
                Displacement[i] -= Delta * Dist;
                Displacement[Link] += Delta * Dist;
            }
        }

        // Gravity to keep graph centered
        for (int32 i = 0; i < Num; ++i)
        {
            Displacement[i] -= Nodes[i].Position * Gravity * Mass[i];
        }

        const float Step = DeltaTime * 5.f;
        for (int32 i = 0; i < Num; ++i)
        {
            if (i == RootIndex)
            {
                continue; // keep root node anchored
            }

            Nodes[i].Position += (Displacement[i] / Mass[i]) * Step;
        }
    }
} // namespace

void SNsSpyglassGraphWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
    const UNsSpyglassSettings* Settings = UNsSpyglassSettings::GetSettings();
    RunForceAtlas2Step(Nodes, RootIndex, Settings->Repulsion, Settings->CenterForce, InDeltaTime);
}

