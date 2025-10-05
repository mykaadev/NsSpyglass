#include "NsSpyglass.h"
#include "Settings/NsSpyglassSettings.h"
#include "Styling/SlateTypes.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSpinBox.h"
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

    // Spin boxes that expose the runtime settings. The widgets are stored so
    // their values can be updated when Zen mode toggles.
    TSharedPtr<SSpinBox<float>> RepulsionSpinBox;
    TSharedPtr<SSpinBox<float>> CenterForceSpinBox;
    TSharedPtr<SSpinBox<float>> AttractionSpinBox;

    // Limits for the slider UI
    const float MaxRepulsion = 50000.f;
    const float MaxCenterForce = 2.f;
    const float MaxAttractionScale = 2.f;

    TSharedRef<SDockTab> Tab = SNew(SDockTab)
    .TabRole(ETabRole::NomadTab)
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot().FillWidth(1.f)
        [
            SAssignNew(GraphWidget, SNsSpyglassGraphWidget)
        ]
        + SHorizontalBox::Slot().AutoWidth().Padding(4.f)
        [
            SAssignNew(InfoWidget, SPluginInfoWidget)
        ]
        + SHorizontalBox::Slot().AutoWidth().Padding(4.f)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight()
            [
                SNew(STextBlock).Text(FText::FromString("Repulsion"))
            ]
            + SVerticalBox::Slot().AutoHeight()
            [
                SAssignNew(RepulsionSpinBox, SSpinBox<float>)
                .Value(UNsSpyglassSettings::GetSettings()->Repulsion)
                .MaxValue(MaxRepulsion)
                .MinValue(10000.f)
                .MinSliderValue(10000.f)
                .MaxSliderValue(MaxRepulsion)
                .OnValueChanged_Lambda([](const float V)
                {
                    UNsSpyglassSettings* NsSpyglassSettings = GetMutableDefault<UNsSpyglassSettings>();
                    check(NsSpyglassSettings);
                        
                    NsSpyglassSettings->Repulsion = V;
                    NsSpyglassSettings->SaveConfig();
                })
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,5,0,0))
            [
                SNew(STextBlock).Text(FText::FromString("Center Force"))
            ]
            + SVerticalBox::Slot().AutoHeight()
            [
                SAssignNew(CenterForceSpinBox, SSpinBox<float>)
                .Value(UNsSpyglassSettings::GetSettings()->CenterForce)
                .MaxValue(MaxCenterForce)
                .MinValue(0.f)
                .MinSliderValue(0.f)
                .MaxSliderValue(MaxCenterForce)
                .OnValueChanged_Lambda([](const float V)
                {
                    UNsSpyglassSettings* NsSpyglassSettings = GetMutableDefault<UNsSpyglassSettings>();
                    check(NsSpyglassSettings);
                        
                    NsSpyglassSettings->CenterForce = V;
                    NsSpyglassSettings->SaveConfig();
                })
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,5,0,0))
            [
                SNew(STextBlock).Text(FText::FromString("AttractionScale"))
            ]
            + SVerticalBox::Slot().AutoHeight()
            [
                SAssignNew(AttractionSpinBox, SSpinBox<float>)
                .Value(UNsSpyglassSettings::GetSettings()->AttractionScale)
                .MaxValue(MaxAttractionScale)
                .MinValue(0.1f)
                .MinSliderValue(0.1f)
                .MaxSliderValue(MaxAttractionScale)
                .OnValueChanged_Lambda([](const float V)
                {
                    UNsSpyglassSettings* NsSpyglassSettings = GetMutableDefault<UNsSpyglassSettings>();
                    check(NsSpyglassSettings);
                        
                    NsSpyglassSettings->AttractionScale = V;
                    NsSpyglassSettings->SaveConfig();
                })
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,5,0,0))
            [
                SNew(STextBlock).Text(FText::FromString("Zen Mode"))
            ]
            + SVerticalBox::Slot().AutoHeight()
            [
                SNew(SCheckBox)
                .IsChecked_Lambda([&] ()
                {
                    return UNsSpyglassSettings::GetSettings()->bZenMode ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
                })
                .OnCheckStateChanged_Lambda([GraphWidget,
                                           RepulsionSpinBox,
                                           CenterForceSpinBox,
                                           AttractionSpinBox](ECheckBoxState InCheckBoxState)
                {
                    UNsSpyglassSettings* NsSpyglassSettings = GetMutableDefault<UNsSpyglassSettings>();
                    check(NsSpyglassSettings);
                        
                    const bool bPreviousState = NsSpyglassSettings->bZenMode;
                    if (InCheckBoxState == ECheckBoxState::Checked)
                    {
                        NsSpyglassSettings->Repulsion = 35000.f;
                        NsSpyglassSettings->CenterForce = 0.2f;
                        NsSpyglassSettings->AttractionScale = 1.0f;
                    }
                    else
                    {
                        NsSpyglassSettings->Repulsion = 15000.f;
                        NsSpyglassSettings->CenterForce = 0.05f;
                        NsSpyglassSettings->AttractionScale = 1.f;
                    }

                    NsSpyglassSettings->bZenMode = InCheckBoxState == ECheckBoxState::Checked;
                    NsSpyglassSettings->SaveConfig();

                    if (RepulsionSpinBox.IsValid())
                    {
                        RepulsionSpinBox->SetValue(NsSpyglassSettings->Repulsion);
                    }
                    if (CenterForceSpinBox.IsValid())
                    {
                        CenterForceSpinBox->SetValue(NsSpyglassSettings->CenterForce);
                    }
                    if (AttractionSpinBox.IsValid())
                    {
                        AttractionSpinBox->SetValue(NsSpyglassSettings->AttractionScale);
                    }

                    if (GraphWidget.IsValid() && bPreviousState != NsSpyglassSettings->bZenMode)
                    {
                        GraphWidget->RebuildGraph();
                    }
                })
            ]
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
