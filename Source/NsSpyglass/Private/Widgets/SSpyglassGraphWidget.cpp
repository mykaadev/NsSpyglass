#include "Widgets/SSpyglassGraphWidget.h"
#include "Interfaces/IPluginManager.h"
#include "Rendering/DrawElements.h"
#include "Math/RandomStream.h"

void SSpyglassGraphWidget::Construct(const FArguments& InArgs)
{
    RecenterView();
    BuildNodes(FVector2D(800.f, 600.f));
}

void SSpyglassGraphWidget::BuildNodes(const FVector2D& ViewSize) const
{
    Nodes.Reset();

    FRandomStream Rand(12345);

    const TArray<TSharedRef<IPlugin>>& Plugins = IPluginManager::Get().GetEnabledPlugins();

    TMap<FString, int32> NameToIndex;

    // Root node stays in the middle
    FPluginNode RootNode;
    RootNode.Name = TEXT("Root");
    RootNode.Position = FVector2D::ZeroVector;
    RootNode.bFixed = true;
    int32 RootIndex = Nodes.Add(RootNode);
    NameToIndex.Add(RootNode.Name, RootIndex);

    // Create nodes for plugins
    for (const TSharedRef<IPlugin>& Plugin : Plugins)
    {
        FPluginNode Node;
        const FVector RandVector = Rand.VRand() * 200.f;

        Node.Name = Plugin->GetName();
        Node.bIsEngine = Plugin->GetLoadedFrom() == EPluginLoadedFrom::Engine;
        Node.Position = FVector2D(RandVector.X, RandVector.Y);
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
}

int32 SSpyglassGraphWidget::HitTestNode(const FVector2D& LocalPos, const FVector2D& ViewSize) const
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

int32 SSpyglassGraphWidget::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
    const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
    int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
    if (Nodes.Num() == 0)
    {
        BuildNodes(AllottedGeometry.GetLocalSize());
    }

    const FVector2D Center = AllottedGeometry.GetLocalSize() * 0.5f;

    // Draw edges
    for (int32 i = 0; i < Nodes.Num(); ++i)
    {
        const FPluginNode& Node = Nodes[i];
        const FVector2D NodePos = Center + ViewOffset + Node.Position * ZoomAmount;
        for (int32 Link : Node.Links)
        {
            if (i < Link && Nodes.IsValidIndex(Link))
            {
                const FVector2D DepPos = Center + ViewOffset + Nodes[Link].Position * ZoomAmount;
                TArray<FVector2D> LinePoints;
                LinePoints.Add(NodePos);
                LinePoints.Add(DepPos);
                FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), LinePoints, ESlateDrawEffect::None, FLinearColor::Gray);
            }
        }
    }

    // Draw nodes
    for (int32 i = 0; i < Nodes.Num(); ++i)
    {
        const FPluginNode& Node = Nodes[i];
        float Size = Node.Name == TEXT("Root") ? 60.f : 40.f;
        FVector2D DrawPos = Center + ViewOffset + Node.Position * ZoomAmount - FVector2D(Size * 0.5f, Size * 0.5f);

        FLinearColor BoxColor = Node.bIsEngine ? FLinearColor(0.1f, 0.6f, 0.1f) : FLinearColor(0.1f, 0.4f, 0.8f);
        if (Node.Name == TEXT("Root"))
        {
            BoxColor = FLinearColor(0.8f, 0.2f, 0.2f);
        }
        if (i == HoveredNode)
        {
            BoxColor = FLinearColor::Yellow;
        }

        FSlateDrawElement::MakeBox(
            OutDrawElements,
            LayerId + 1,
            AllottedGeometry.ToPaintGeometry(FVector2D(Size, Size), FSlateLayoutTransform(DrawPos)),
            FCoreStyle::Get().GetBrush("WhiteBrush"),
            ESlateDrawEffect::None,
            BoxColor
        );

        FSlateDrawElement::MakeText(
            OutDrawElements,
            LayerId + 2,
            AllottedGeometry.ToPaintGeometry(FVector2D(Size - 10.f, 20.f), FSlateLayoutTransform(DrawPos + FVector2D(5.f, Size * 0.5f - 10.f))),
            Node.Name,
            FCoreStyle::Get().GetFontStyle("NormalFont"),
            ESlateDrawEffect::None,
            FLinearColor::White
        );
    }

    return LayerId + 3;
}

FReply SSpyglassGraphWidget::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
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

FReply SSpyglassGraphWidget::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
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

FReply SSpyglassGraphWidget::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
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

FReply SSpyglassGraphWidget::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
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

void SSpyglassGraphWidget::RecenterView()
{
    ViewOffset = FVector2D::ZeroVector;
    ZoomAmount = 1.f;
}

void SSpyglassGraphWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
    if (Nodes.Num() == 0)
    {
        return;
    }

    TArray<FVector2D> Forces;
    Forces.Init(FVector2D::ZeroVector, Nodes.Num());

    const float Repulsion = 200000.f;
    const float SpringLength = 150.f;
    const float SpringStiffness = 0.2f;
    const float MaxLinkDistance = 300.f;

    for (int32 i = 0; i < Nodes.Num(); ++i)
    {
        for (int32 j = i + 1; j < Nodes.Num(); ++j)
        {
            FVector2D Delta = Nodes[j].Position - Nodes[i].Position;
            float DistSq = FMath::Max(Delta.SizeSquared(), 1.f);
            FVector2D Dir = Delta / FMath::Sqrt(DistSq);
            FVector2D Rep = Dir * (Repulsion / DistSq);
            Forces[i] -= Rep;
            Forces[j] += Rep;
        }
    }

    for (int32 i = 0; i < Nodes.Num(); ++i)
    {
        for (int32 Link : Nodes[i].Links)
        {
            if (Link < 0 || Link >= Nodes.Num() || Link <= i)
            {
                continue;
            }
            FVector2D Delta = Nodes[Link].Position - Nodes[i].Position;
            float Dist = FMath::Max(Delta.Size(), 1.f);
            FVector2D Dir = Delta / Dist;
            FVector2D Spring = Dir * (Dist - SpringLength) * SpringStiffness;
            Forces[i] += Spring;
            Forces[Link] -= Spring;
        }
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
        for (int32 Link : Nodes[i].Links)
        {
            if (Link < 0 || Link >= Nodes.Num() || Link <= i)
            {
                continue;
            }

            FVector2D Delta = Nodes[Link].Position - Nodes[i].Position;
            float Dist = Delta.Size();
            if (Dist > MaxLinkDistance)
            {
                FVector2D Dir = Delta / Dist;
                FVector2D Adjust = Dir * (Dist - MaxLinkDistance) * 0.5f;
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

