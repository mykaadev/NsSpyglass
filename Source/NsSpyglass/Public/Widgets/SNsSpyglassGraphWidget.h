#pragma once

#include "CoreMinimal.h"
#include "Interfaces/IPluginManager.h"
#include "Widgets/SCompoundWidget.h"

/**
 * Node information used by the force-directed graph
 */
struct FPluginNode
{
    /** Display name of the plugin. */
    FString Name;

    /** Current position relative to the center of the view. */
    FVector2D Position = FVector2D::ZeroVector;

    /** Indices of linked plugins used by the layout. */
    TArray<int32> Links;

    /** Directed dependencies for this plugin. */
    TArray<int32> Dependencies;

    /** Plugins that directly depend on this one. */
    TArray<int32> Dependents;

    /** Whether this plugin comes from the engine. */
    bool bIsEngine = false;

    /** Color assigned to this node's group. */
    FLinearColor Color = FLinearColor(1.f, 1.f, 1.f, 0.1f);

    /** When true, the node will remain stationary during simulation. */
    bool bFixed = false;


    /** Plugin reference used for detailed information. */
    TSharedPtr<IPlugin> Plugin;
};

/** Background star used for the parallax backdrop. */
struct FBackgroundStar
{
    /** Position relative to the center of the screen. */
    FVector2D Position = FVector2D::ZeroVector;

    /** Current opacity of the star. */
    float Alpha = 0.f;

    /** Desired opacity to interpolate toward. */
    float TargetAlpha = 0.f;

    /** Speed of the fade interpolation. */
    float FadeSpeed = 1.f;
};

/**
 * Widget that displays all loaded plugins in a force-directed graph
 */
class SNsSpyglassGraphWidget : public SCompoundWidget
{

// Functions
public:

    /** Constructor */
    SNsSpyglassGraphWidget();

    SLATE_BEGIN_ARGS(SNsSpyglassGraphWidget) {}
    SLATE_END_ARGS()

    /** Build the widget and initialize graph data. */
    void Construct(const FArguments& InArgs);

    /** Delegate fired when the hovered node changes. */
    DECLARE_DELEGATE_OneParam(FOnNodeHovered, TSharedPtr<IPlugin>);

    /** Register a callback for hover events. */
    void SetOnNodeHovered(FOnNodeHovered InDelegate);

    //~ Begin SCompoundWidget Interface
    virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
    virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
    virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
    virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
    virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
    virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
    //~ End SCompoundWidget Interface

    /** Reset pan/zoom to the origin. */
    void RecenterView();

    /** Clear and rebuild all nodes. */
    void RebuildGraph();

private:

    /** Create random background stars. */
    void InitStars(const FVector2D& ViewSize) const;

    /** Populate the node array by scanning loaded plugins. */
    void BuildNodes(const FVector2D& ViewSize) const;

    /** Return the index of the node under the cursor or INDEX_NONE. */
    int32 HitTestNode(const FVector2D& LocalPos, const FVector2D& ViewSize) const;

    /** Perform a single ForceAtlas2 iteration on the node array.*/
    void RunForceAtlas2Step(TArray<FPluginNode>& Nodes, int32 RootIndex, float Repulsion, float Gravity, float DeltaTime);

// Variables
private:

    /** All nodes currently in the graph. */
    mutable TArray<FPluginNode> Nodes;

    /** Panning offset applied to the view. */
    mutable FVector2D ViewOffset;

    /** Last mouse position used for dragging calculations. */
    mutable FVector2D LastMousePos;

    /** Current zoom factor. */
    mutable float ZoomAmount = 1.f;

    /** Whether the user is currently panning the view. */
    mutable bool bIsPanning = false;

    /** Whether a node is currently being dragged. */
    mutable bool bIsDragging = false;

    /** Index of the node currently dragged by the user. */
    mutable int32 DraggedNode = INDEX_NONE;


    /** Index of the node currently hovered by the mouse. */
    mutable int32 HoveredNode = INDEX_NONE;

    /** Index of the root node in the Nodes array. */
    mutable int32 RootIndex = INDEX_NONE;

    /** Delegate for hover updates. */
    FOnNodeHovered OnNodeHovered;

    /** Background stars shown behind the graph. */
    mutable TArray<FBackgroundStar> Stars;

    /** Cached size for generating star positions. */
    mutable FVector2D StarsViewSize = FVector2D::ZeroVector;
};

