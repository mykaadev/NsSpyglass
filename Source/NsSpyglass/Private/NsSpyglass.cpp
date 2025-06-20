#include "NsSpyglass.h"
#include "Widgets/SSpyglassGraphWidget.h"
#include "LevelEditor.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Settings/SpyglassSettings.h"

#define LOCTEXT_NAMESPACE "FNsSpyglassModule"

static const FName SpyglassTabName("SpyglassTab");

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

void FNsSpyglassModule::ShutdownModule()
{
    UToolMenus::UnregisterOwner(this);
    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(SpyglassTabName);
}

TSharedRef<SDockTab> FNsSpyglassModule::OnSpawnPluginTab(const FSpawnTabArgs& Args)
{
    UNsSpyglassSettings* Settings = const_cast<UNsSpyglassSettings*>(UNsSpyglassSettings::Get());

    TSharedPtr<SSpyglassGraphWidget> GraphWidget;

    auto Slider = [](float& Value, float Max, UNsSpyglassSettings* InSettings)
    {
        return SNew(SSlider)
            .Value(Value / Max)
            .OnValueChanged_Lambda([InSettings, Max, &Value](float V)
            {
                Value = V * Max;
                InSettings->SaveConfig();
            });
    };

    const float MaxRepulsion = 500000.f;
    const float MaxSpringLength = 300.f;
    const float MaxStiffness = 1.f;
    const float MaxLinkDist = 600.f;

    return SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth().Padding(4.f)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight()
                [ SNew(STextBlock).Text(FText::FromString("Repulsion")) ]
                + SVerticalBox::Slot().AutoHeight()
                [ Slider(Settings->Repulsion, MaxRepulsion, Settings) ]
                + SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,5,0,0))
                [ SNew(STextBlock).Text(FText::FromString("Spring Length")) ]
                + SVerticalBox::Slot().AutoHeight()
                [ Slider(Settings->SpringLength, MaxSpringLength, Settings) ]
                + SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,5,0,0))
                [ SNew(STextBlock).Text(FText::FromString("Spring Stiffness")) ]
                + SVerticalBox::Slot().AutoHeight()
                [ Slider(Settings->SpringStiffness, MaxStiffness, Settings) ]
                + SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,5,0,0))
                [ SNew(STextBlock).Text(FText::FromString("Max Link Distance")) ]
                + SVerticalBox::Slot().AutoHeight()
                [ Slider(Settings->MaxLinkDistance, MaxLinkDist, Settings) ]
                + SVerticalBox::Slot().AutoHeight().Padding(FMargin(0,10,0,0))
                [ SNew(SButton)
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
                SAssignNew(GraphWidget, SSpyglassGraphWidget).Settings(Settings)
            ]
        ];
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FNsSpyglassModule, NsSpyglass)
