#pragma once

#include "CoreMinimal.h"
#include "Interfaces/IPluginManager.h"
#include "Widgets/SCompoundWidget.h"

/**
 * Widget that displays information about a plugin.
 */
class SPluginInfoWidget : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SPluginInfoWidget) {}
    SLATE_END_ARGS()

    /** Build the widget. */
    void Construct(const FArguments& InArgs);

    /** Set plugin info to display. Pass nullptr to clear. */
    void SetPlugin(TSharedPtr<IPlugin> InPlugin);

private:
    /** Open the documentation URL. */
    void OnDocsClicked() const;

    /** Current plugin displayed. */
    TSharedPtr<IPlugin> CurrentPlugin;

    /** Widget references for updating. */
    TSharedPtr<class STextBlock> NameText;
    TSharedPtr<class STextBlock> DescriptionText;
    TSharedPtr<class STextBlock> AuthorText;
    TSharedPtr<class SHyperlink> DocsLink;
    TSharedPtr<class SVerticalBox> ModulesBox;
    TSharedPtr<class SVerticalBox> DependenciesBox;

    /** URL to open when the docs hyperlink is clicked. */
    FString DocsURL;
};

