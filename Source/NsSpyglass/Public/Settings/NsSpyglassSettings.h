#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "NsSpyglassSettings.generated.h"

/**
 * Settings that control the force directed layout.
 * Values are persisted per user so tweaks are restored across editor sessions.
 */
UCLASS(Config=EditorPerProjectUserSettings, DefaultConfig)
class UNsSpyglassSettings : public UDeveloperSettings
{
    GENERATED_BODY()

// Functions
public:

    /** Constructor */
    UNsSpyglassSettings();

    /** Get Spyglass Settings */
    UFUNCTION(BlueprintCallable, Category = "Settings")
    static UNsSpyglassSettings* GetSettings();

// Variables
public:

    /** Strength of the repulsion force between nodes. */
    UPROPERTY(EditAnywhere, Config, Category="Layout")
    float Repulsion;

    /** Desired distance when nodes are linked together. */
    UPROPERTY(EditAnywhere, Config, Category="Layout")
    float SpringLength;

    /** Stiffness used by the spring connections. */
    UPROPERTY(EditAnywhere, Config, Category="Layout")
    float SpringStiffness;

    /** Maximum allowed distance between linked nodes. */
    UPROPERTY(EditAnywhere, Config, Category="Layout")
    float MaxLinkDistance;

};
