#include "NsSpyglass.h"
#include "Widgets/SNsSpyglassGraphWidget.h"
#include "LevelEditor.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Settings/NsSpyglassSettings.h"
#include "Widgets/Input/SSpinBox.h"

#define LOCTEXT_NAMESPACE "FNsSpyglassModule"

// Identifier for the plugin's main tab
static const FName SpyglassTabName("SpyglassTab");

/** Register the plugin tab and add the menu entry. */
void FNsSpyglassModule::StartupModule()
{
    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(SpyglassTabName,
        FOnSpawnTab::CreateRaw(this, &FNsSpyglassModule::OnSpawnPluginTab))
        .SetDisplayName(LOCTEXT("SpyglassTabTitle", "Spyglass"))
        .SetMenuType(ETabSpawnerMenuType::Hidden);

    UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateLambda([]
    {
        UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
        FToolMenuSection& Section = Menu->AddSection("SpyglassMenu", LOCTEXT("SpyglassMenu", "Spyglass"));
        Section.AddMenuEntry("OpenSpyglass",
            LOCTEXT("OpenSpyglass", "Plugin Dependency Viewer"),
            LOCTEXT("OpenSpyglassTooltip", "Open Spyglass viewer"),
            FSlateIcon(),
            FUIAction(FExecuteAction::CreateLambda([] { FGlobalTabmanager::Get()->TryInvokeTab(SpyglassTabName); })));    
    }));
}

/** Cleanup registered UI on shutdown. */
void FNsSpyglassModule::ShutdownModule()
{
    UToolMenus::UnregisterOwner(this);
    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(SpyglassTabName);
}

/** Create the main plugin tab with the graph widget and parameter controls. */
TSharedRef<SDockTab> FNsSpyglassModule::OnSpawnPluginTab(const FSpawnTabArgs& Args)
{
    UNsSpyglassSettings* const Settings = UNsSpyglassSettings::GetSettings();

    TSharedPtr<SNsSpyglassGraphWidget> GraphWidget;

    // Helper that binds a slider to a settings value
    auto Slider = [](float& Value, const float Min, const float Max, UNsSpyglassSettings* InSettings)
    {
        return SNew(SSpinBox<float>)
        .Value(Value / Max)
        .MaxValue(Max)
        .MinValue(Min)
        .MinSliderValue(Min)
        .MaxSliderValue(Max)
        .OnValueChanged_Lambda([InSettings, Max, &Value](float V)
        {
            Value = V;
            InSettings->SaveConfig();
        });
    };

    // Limits for the slider UI
    const float MaxRepulsion = 50000.f;
    const float MaxSpringLength = 500.f;
    const float MaxStiffness = 1.f;
    const float MaxLinkDist = 1000.f;

    return SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth().Padding(4.f)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight()
                [
                    SNew(STextBlock).Text(FText::FromString("Repulsion"))
                ]
                + SVerticalBox::Slot().AutoHeight()
                [
                    Slider(Settings->Repulsion, 1.f, MaxRepulsion, Settings)
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,5,0,0))
                [
                    SNew(STextBlock).Text(FText::FromString("Spring Length"))
                ]
                + SVerticalBox::Slot().AutoHeight()
                [
                    Slider(Settings->SpringLength, 1.f, MaxSpringLength, Settings)
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,5,0,0))
                [
                    SNew(STextBlock).Text(FText::FromString("Spring Stiffness"))
                ]
                + SVerticalBox::Slot().AutoHeight()
                [
                    Slider(Settings->SpringStiffness, 0.f, MaxStiffness, Settings)
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,5,0,0))
                [
                    SNew(STextBlock).Text(FText::FromString("Max Link Distance"))
                ]
                + SVerticalBox::Slot().AutoHeight()
                [
                    Slider(Settings->MaxLinkDistance, 1.f, MaxLinkDist, Settings)
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,10,0,0))
                [
                    SNew(SButton)
                    .Text(FText::FromString("Reset"))
                    .OnClicked_Lambda([&GraphWidget]()
                    {
                        if (GraphWidget.IsValid())
                        {
                            GraphWidget->RebuildGraph();
                        }
                        return FReply::Handled();
                    })
                ]
            ]
            + SHorizontalBox::Slot().FillWidth(1.f)
            [
                SAssignNew(GraphWidget, SNsSpyglassGraphWidget).Settings(Settings)
            ]
        ];
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FNsSpyglassModule, NsSpyglass)
