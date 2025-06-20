#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

struct FPluginNode
{
    FString Name;
    FVector2D Position;
    TArray<FString> Dependencies;
};

class SSpyglassGraphWidget : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SSpyglassGraphWidget) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
        const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
        int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
    void BuildNodes() const;

    mutable TArray<FPluginNode> Nodes;
};

