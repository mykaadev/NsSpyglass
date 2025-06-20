#pragma once

#include "CoreMinimal.h"
#include "UObject/DeveloperSettings.h"
#include "SpyglassSettings.generated.h"

/** Persistent settings for Spyglass layout. */
UCLASS(Config=EditorPerProjectUserSettings, DefaultConfig)
class UNsSpyglassSettings : public UDeveloperSettings
{
    GENERATED_BODY()
public:
    // Force directed layout parameters
    UPROPERTY(EditAnywhere, Config, Category="Layout")
    float Repulsion = 200000.f;

    UPROPERTY(EditAnywhere, Config, Category="Layout")
    float SpringLength = 150.f;

    UPROPERTY(EditAnywhere, Config, Category="Layout")
    float SpringStiffness = 0.2f;

    UPROPERTY(EditAnywhere, Config, Category="Layout")
    float MaxLinkDistance = 300.f;

    static const UNsSpyglassSettings* Get();
};
