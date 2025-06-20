#pragma once

#include "Modules/ModuleManager.h"

class FNsSpyglassModule : public IModuleInterface
{
public:
    /** IModuleInterface implementation */
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    TSharedRef<class SDockTab> OnSpawnPluginTab(const class FSpawnTabArgs& Args);
};

