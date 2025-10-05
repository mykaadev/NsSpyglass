// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "Settings/NsSpyglassSettings.h"

UNsSpyglassSettings::UNsSpyglassSettings()
	: Repulsion(15000.f)
	, CenterForce(0.05f)
	, AttractionScale(1.f)
	, bZenMode(false)
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
