#include "NsSpyglass.h"
#include "Widgets/SSpyglassGraphWidget.h"
#include "LevelEditor.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

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
    TSharedRef<FSpyglassGraphParams> Params = MakeShared<FSpyglassGraphParams>();

    return SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                [ SNew(STextBlock).Text(FText::FromString("Repulsion")) ]
                + SHorizontalBox::Slot()
                [
                    SNew(SNumericEntryBox<float>)
                    .Value_Lambda([Params] { return Params->Repulsion; })
                    .OnValueChanged_Lambda([Params](float V) { Params->Repulsion = V; })
                ]
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                [ SNew(STextBlock).Text(FText::FromString("Spring Length")) ]
                + SHorizontalBox::Slot()
                [
                    SNew(SNumericEntryBox<float>)
                    .Value_Lambda([Params] { return Params->SpringLength; })
                    .OnValueChanged_Lambda([Params](float V) { Params->SpringLength = V; })
                ]
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                [ SNew(STextBlock).Text(FText::FromString("Spring Stiffness")) ]
                + SHorizontalBox::Slot()
                [
                    SNew(SNumericEntryBox<float>)
                    .Value_Lambda([Params] { return Params->SpringStiffness; })
                    .OnValueChanged_Lambda([Params](float V) { Params->SpringStiffness = V; })
                ]
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                [ SNew(STextBlock).Text(FText::FromString("Max Link Distance")) ]
                + SHorizontalBox::Slot()
                [
                    SNew(SNumericEntryBox<float>)
                    .Value_Lambda([Params] { return Params->MaxLinkDistance; })
                    .OnValueChanged_Lambda([Params](float V) { Params->MaxLinkDistance = V; })
                ]
            ]
            + SVerticalBox::Slot()
            .FillHeight(1.f)
            [
                SNew(SSpyglassGraphWidget).Params(Params)
            ]
        ];
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FNsSpyglassModule, NsSpyglass)
