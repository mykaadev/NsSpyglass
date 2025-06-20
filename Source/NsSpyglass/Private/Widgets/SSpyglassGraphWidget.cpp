#include "Widgets/SSpyglassGraphWidget.h"
#include "Interfaces/IPluginManager.h"
#include "Rendering/DrawElements.h"

void SSpyglassGraphWidget::Construct(const FArguments& InArgs)
{
    BuildNodes();
}

void SSpyglassGraphWidget::BuildNodes() const
{
    Nodes.Reset();
    const TArray<TSharedRef<IPlugin>>& Plugins = IPluginManager::Get().GetEnabledPlugins();

    const float Radius = 300.f;
    const FVector2D Center(400.f, 400.f);
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
    BuildNodes();

    const FVector2D Offset = AllottedGeometry.GetLocalSize() * 0.5f;

    // Draw edges
    for (const FPluginNode& Node : Nodes)
    {
        for (const FString& DepName : Node.Dependencies)
        {
            const FPluginNode* DepNode = Nodes.FindByPredicate([&](const FPluginNode& N){ return N.Name == DepName; });
            if (DepNode)
            {
                TArray<FVector2D> LinePoints;
                LinePoints.Add(Node.Position - Offset);
                LinePoints.Add(DepNode->Position - Offset);
                FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), LinePoints, ESlateDrawEffect::None, FLinearColor::Gray);
            }
        }
    }

    // Draw nodes
    for (const FPluginNode& Node : Nodes)
    {
        FVector2D DrawPos = Node.Position - Offset - FVector2D(25.f, 25.f);
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

