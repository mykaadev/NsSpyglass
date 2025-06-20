#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Containers/Set.h"

struct FPluginNode
{
    FString Name;
    FVector2D Position;
    FVector2D Velocity = FVector2D::ZeroVector;
    TArray<int32> Links;
    bool bIsEngine = false;
    bool bFixed = false;
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

    virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
    void BuildNodes(const FVector2D& ViewSize) const;
    int32 HitTestNode(const FVector2D& LocalPos, const FVector2D& ViewSize) const;

    virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
    virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
    virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
    virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

    void RecenterView();
    void UpdateHighlightedNodes(int32 StartIndex) const;

    mutable TArray<FPluginNode> Nodes;
    mutable FVector2D ViewOffset = FVector2D::ZeroVector;
    mutable FVector2D LastMousePos = FVector2D::ZeroVector;
    mutable float ZoomAmount = 1.f;
    mutable bool bIsPanning = false;
    mutable bool bIsDragging = false;
    mutable int32 DraggedNode = INDEX_NONE;
    mutable int32 HoveredNode = INDEX_NONE;
    mutable TSet<int32> HighlightedNodes;
};

