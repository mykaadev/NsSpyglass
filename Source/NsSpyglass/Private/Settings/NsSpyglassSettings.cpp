// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "Settings/NsSpyglassSettings.h"

UNsSpyglassSettings::UNsSpyglassSettings()
    : Repulsion(1000.f)
    , CenterForce(0.05f)
    , AttractionScale(1.f)
    , bShowEnginePlugins(true)
    , bShowProjectPlugins(true)
    , bShowDisabledPlugins(false)
    , bEnableImpactHeatmap(true)
{
    CategoryName = FName(TEXTVIEW("Plugins"));
}

const UNsSpyglassSettings* UNsSpyglassSettings::GetSettings()
{
   return GetDefault<UNsSpyglassSettings>();
}

FName UNsSpyglassSettings::GetContainerName() const
{
    return FName{TEXTVIEW("Editor")};
}
