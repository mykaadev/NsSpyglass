// Copyright (C) 2025 mykaadev. All rights reserved.

#include "Settings/NsSpyglassSettings.h"

UNsSpyglassSettings::UNsSpyglassSettings()
       : Repulsion(50000.f)
    , SpringLength(80.f)
       , SpringStiffness(0.5f)
       , MaxLinkDistance(500.f)
       , CenterForce(50.f)
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
