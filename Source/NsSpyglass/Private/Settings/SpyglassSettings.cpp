#include "Settings/SpyglassSettings.h"

/** Return the default settings object. */
const UNsSpyglassSettings* UNsSpyglassSettings::Get()
{
    return GetDefault<UNsSpyglassSettings>();
}
