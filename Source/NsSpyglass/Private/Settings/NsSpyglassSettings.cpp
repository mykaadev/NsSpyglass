// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "Settings/NsSpyglassSettings.h"

UNsSpyglassSettings::UNsSpyglassSettings()
	: Repulsion(15000.f)
	, CenterForce(0.05f)
	, AttractionScale(1.f)
	, bZenMode(false)
{}

UNsSpyglassSettings* UNsSpyglassSettings::GetSettings()
{
   static UNsSpyglassSettings* Instance;
   if(Instance != nullptr)
   {
       return Instance;
   }

   for (const TObjectIterator<UNsSpyglassSettings> SettingsIt(RF_NoFlags); SettingsIt;)
   {
       Instance = *SettingsIt;
       break;
   }
   return Instance;
}
