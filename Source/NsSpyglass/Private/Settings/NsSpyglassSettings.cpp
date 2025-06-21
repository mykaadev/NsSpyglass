// Copyright (C) 2025 mykaadev. All rights reserved.

#include "Settings/NsSpyglassSettings.h"

UNsSpyglassSettings::UNsSpyglassSettings()
   : Repulsion(5000.f)
   , CenterForce(0.5f)
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
