#include "Settings/SpyglassSettings.h"

const UNsSpyglassSettings* UNsSpyglassSettings::Get()
{
    return GetDefault<UNsSpyglassSettings>();
}
