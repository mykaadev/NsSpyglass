#include "NsSpyglass.h"
#include "Widgets/SSpyglassGraphWidget.h"
#include "LevelEditor.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"

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
    return SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
        [
            SNew(SSpyglassGraphWidget)
        ];
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FNsSpyglassModule, NsSpyglass)
