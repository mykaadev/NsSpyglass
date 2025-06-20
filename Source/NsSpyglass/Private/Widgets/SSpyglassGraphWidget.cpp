#include "Widgets/SSpyglassGraphWidget.h"
#include "Interfaces/IPluginManager.h"
#include "Rendering/DrawElements.h"

void SSpyglassGraphWidget::Construct(const FArguments& InArgs)
{
    RecenterView();
}

void SSpyglassGraphWidget::BuildNodes(const FVector2D& ViewSize) const
{
    Nodes.Reset();
    const TArray<TSharedRef<IPlugin>>& Plugins = IPluginManager::Get().GetEnabledPlugins();

    const float Radius = FMath::Min(ViewSize.X, ViewSize.Y) * 0.45f;
    const FVector2D Center = ViewSize * 0.5f;
    int32 Index = 0;
    for (const TSharedRef<IPlugin>& Plugin : Plugins)
    {
        FPluginNode Node;
        Node.Name = Plugin->GetName();
        const FPluginDescriptor& Desc = Plugin->GetDescriptor();

        for (const FPluginReferenceDescriptor& Ref : Desc.Plugins)
        {
            if (Ref.bEnabled)
            {
                Node.Dependencies.Add(Ref.Name);
            }
        }

        const float Angle = (2.f * PI) * Index / Plugins.Num();
        Node.Position = Center + FVector2D(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius);
        Nodes.Add(Node);
        ++Index;
    }
}

int32 SSpyglassGraphWidget::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
    const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
    int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
    BuildNodes(AllottedGeometry.GetLocalSize());

    const FVector2D Center = AllottedGeometry.GetLocalSize() * 0.5f;

    // Draw edges
    for (const FPluginNode& Node : Nodes)
    {
        const FVector2D NodePos = Center + ViewOffset + (Node.Position - Center) * ZoomAmount;
        for (const FString& DepName : Node.Dependencies)
        {
            const FPluginNode* DepNode = Nodes.FindByPredicate([&](const FPluginNode& N){ return N.Name == DepName; });
            if (DepNode)
            {
                const FVector2D DepPos = Center + ViewOffset + (DepNode->Position - Center) * ZoomAmount;
                TArray<FVector2D> LinePoints;
                LinePoints.Add(NodePos);
                LinePoints.Add(DepPos);
                FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), LinePoints, ESlateDrawEffect::None, FLinearColor::Gray);
            }
        }
    }

    // Draw nodes
    for (const FPluginNode& Node : Nodes)
    {
        FVector2D DrawPos = Center + ViewOffset + (Node.Position - Center) * ZoomAmount - FVector2D(25.f, 25.f);
        FSlateDrawElement::MakeBox(
            OutDrawElements,
            LayerId + 1,
            AllottedGeometry.ToPaintGeometry(FVector2D(50.f, 50.f), FSlateLayoutTransform(DrawPos)),
            FCoreStyle::Get().GetBrush("WhiteBrush"),
            ESlateDrawEffect::None,
            FLinearColor::Blue
        );

        FSlateDrawElement::MakeText(
            OutDrawElements,
            LayerId + 2,
            AllottedGeometry.ToPaintGeometry(FVector2D(40.f,20.f), FSlateLayoutTransform(DrawPos + FVector2D(5.f, 15.f))),
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
    if (MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton) || MouseEvent.IsMouseButtonDown(EKeys::RightMouseButton))
    {
        bIsPanning = true;
        LastMousePos = MouseEvent.GetScreenSpacePosition();
        return FReply::Handled();
    }
    return FReply::Unhandled();
}

FReply SSpyglassGraphWidget::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
    bIsPanning = false;
    return FReply::Handled();
}

FReply SSpyglassGraphWidget::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
    if (bIsPanning)
    {
        const FVector2D Delta = MouseEvent.GetScreenSpacePosition() - LastMousePos;
        ViewOffset += Delta;
        LastMousePos = MouseEvent.GetScreenSpacePosition();
        return FReply::Handled();
    }
    return FReply::Unhandled();
}

FReply SSpyglassGraphWidget::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
    ZoomAmount *= FMath::Pow(1.1f, MouseEvent.GetWheelDelta());
    ZoomAmount = FMath::Clamp(ZoomAmount, 0.2f, 5.f);
    return FReply::Handled();
}

void SSpyglassGraphWidget::RecenterView()
{
    ViewOffset = FVector2D::ZeroVector;
    ZoomAmount = 1.f;
}

