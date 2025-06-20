#pragma once

#include "CoreMinimal.h"
#include "UObject/DeveloperSettings.h"
#include "SpyglassSettings.generated.h"

/** Persistent settings for Spyglass layout. */
UCLASS(Config=EditorPerProjectUserSettings, DefaultConfig)
/**
 * Settings that control the force directed layout. Values are persisted
 * per user so tweaks are restored across editor sessions.
 */
class UNsSpyglassSettings : public UDeveloperSettings
{
    GENERATED_BODY()
public:
    /** Strength of the repulsion force between nodes. */
    UPROPERTY(EditAnywhere, Config, Category="Layout")
    float Repulsion = 200000.f;

    /** Desired distance when nodes are linked together. */
    UPROPERTY(EditAnywhere, Config, Category="Layout")
    float SpringLength = 150.f;

    /** Stiffness used by the spring connections. */
    UPROPERTY(EditAnywhere, Config, Category="Layout")
    float SpringStiffness = 0.2f;

    /** Maximum allowed distance between linked nodes. */
    UPROPERTY(EditAnywhere, Config, Category="Layout")
    float MaxLinkDistance = 300.f;

    static const UNsSpyglassSettings* Get();
};
