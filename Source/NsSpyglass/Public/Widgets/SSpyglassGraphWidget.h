#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

/** Node information used by the force-directed graph. */
struct FPluginNode
{
    /** Display name of the plugin. */
    FString Name;

    /** Current position relative to the center of the view. */
    FVector2D Position;

    /** Velocity accumulated by the solver. */
    FVector2D Velocity = FVector2D::ZeroVector;

    /** Indices of linked plugins. */
    TArray<int32> Links;

    /** Whether this plugin comes from the engine. */
    bool bIsEngine = false;

    /** When true, the node will not move during simulation. */
    bool bFixed = false;
};

class UNsSpyglassSettings;


/** Widget that displays all loaded plugins in a force-directed graph. */
class SSpyglassGraphWidget : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SSpyglassGraphWidget) {}
        /** Settings object used for layout parameters. */
        SLATE_ARGUMENT(UNsSpyglassSettings*, Settings)
    SLATE_END_ARGS()

    /** Build the widget and initialize graph data. */
    void Construct(const FArguments& InArgs);

    /** Draw the graph and handle highlighting. */
    virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
        const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
        int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

    /** Apply the force directed layout each frame. */
    virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

    /** Reset pan/zoom to the origin. */
    void RecenterView();
    /** Clear and rebuild all nodes. */
    void RebuildGraph();

private:
    /** Populate the node array by scanning loaded plugins. */
    void BuildNodes(const FVector2D& ViewSize) const;
    /** Return the index of the node under the cursor or INDEX_NONE. */
    int32 HitTestNode(const FVector2D& LocalPos, const FVector2D& ViewSize) const;

    /** Begin dragging nodes or panning the view. */
    virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
    /** Finish dragging or panning. */
    virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
    /** Handle hover highlight and dragging. */
    virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
    /** Zoom the view using the mouse wheel. */
    virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;


    /** All nodes currently in the graph. */
    mutable TArray<FPluginNode> Nodes;
    /** Panning offset applied to the view. */
    mutable FVector2D ViewOffset = FVector2D::ZeroVector;
    /** Last mouse position used for dragging calculations. */
    mutable FVector2D LastMousePos = FVector2D::ZeroVector;
    /** Current zoom factor. */
    mutable float ZoomAmount = 1.f;
    /** Whether the user is currently panning the view. */
    mutable bool bIsPanning = false;
    /** Whether a node is being dragged. */
    mutable bool bIsDragging = false;
    /** Index of the dragged node. */
    mutable int32 DraggedNode = INDEX_NONE;
    /** Index of the node currently hovered by the mouse. */
    mutable int32 HoveredNode = INDEX_NONE;

    /** Layout parameters shared with the editor UI. */
    UNsSpyglassSettings* Settings = nullptr;
};

