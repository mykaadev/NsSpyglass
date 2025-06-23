#include "NsSpyglass.h"
#include "Settings/NsSpyglassSettings.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SBox.h"
#include "Layout/Visibility.h"
#include "Widgets/SNsSpyglassGraphWidget.h"
#include "Widgets/SPluginInfoWidget.h"
#include "Widgets/Text/STextBlock.h"

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
            FUIAction(FExecuteAction::CreateLambda([]
            {
                FGlobalTabmanager::Get()->TryInvokeTab(SpyglassTabName);
            })));
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
    TSharedPtr<SNsSpyglassGraphWidget> GraphWidget;
    TSharedPtr<SPluginInfoWidget> InfoWidget;
    TSharedPtr<SVerticalBox> SettingsBox;

    // Helper that binds a slider to a settings value
    auto Slider = [](float& Value, const float Min, const float Max)
    {
        return SNew(SSpinBox<float>)
        .Value_Lambda([&Value]() { return Value; })
        .MaxValue(Max)
        .MinValue(Min)
        .MinSliderValue(Min)
        .MaxSliderValue(Max)
        .OnValueChanged_Lambda([&Value](const float V)
        {
            Value = V;
        });
    };

    // Limits for the slider UI
    const float MaxRepulsion = 50000.f;
    const float MaxCenterForce = 2.f;

    TSharedRef<SDockTab> Tab = SNew(SDockTab)
    .TabRole(ETabRole::NomadTab)
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot().AutoWidth().Padding(4.f)
        [
            SAssignNew(SettingsBox, SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().Padding(FMargin(0.f, 0.f, 0.f, 4.f))
            [
                SNew(SCheckBox)
                .IsChecked(ECheckBoxState::Unchecked)
                .OnCheckStateChanged_Lambda([GraphWidget, SettingsBox, InfoWidget](ECheckBoxState State)
                {
                    const bool bZen = State == ECheckBoxState::Checked;
                    if (GraphWidget.IsValid())
                    {
                        GraphWidget->SetZenMode(bZen);
                    }
                    if (SettingsBox.IsValid())
                    {
                        SettingsBox->SetVisibility(bZen ? EVisibility::Collapsed : EVisibility::Visible);
                    }
                    if (InfoWidget.IsValid())
                    {
                        InfoWidget->SetVisibility(bZen ? EVisibility::Collapsed : EVisibility::Visible);
                    }
                })
                [
                    SNew(STextBlock).Text(FText::FromString("Zen Mode"))
                ]
            ]
            + SVerticalBox::Slot().AutoHeight()
            [
                SNew(STextBlock).Text(FText::FromString("Repulsion"))
            ]
            + SVerticalBox::Slot().AutoHeight()
            [
                Slider(UNsSpyglassSettings::GetSettings()->Repulsion, 1000.f, MaxRepulsion)
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,5,0,0))
            [
                SNew(STextBlock).Text(FText::FromString("Center Force"))
            ]
            + SVerticalBox::Slot().AutoHeight()
            [
                Slider(UNsSpyglassSettings::GetSettings()->CenterForce, 0.f, MaxCenterForce)
            ]
        ]
        + SHorizontalBox::Slot().FillWidth(1.f)
        [
            SAssignNew(GraphWidget, SNsSpyglassGraphWidget)
        ]
        + SHorizontalBox::Slot().AutoWidth().Padding(4.f)
        [
            SAssignNew(InfoWidget, SPluginInfoWidget)
        ]
    ];

    if (GraphWidget.IsValid() && InfoWidget.IsValid())
    {
        GraphWidget->SetOnNodeHovered(SNsSpyglassGraphWidget::FOnNodeHovered::CreateSP(InfoWidget.Get(), &SPluginInfoWidget::SetPlugin));
    }

    return Tab;
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FNsSpyglassModule, NsSpyglass)
