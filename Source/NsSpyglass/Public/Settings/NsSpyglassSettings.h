#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "NsSpyglassSettings.generated.h"

/**
 * Settings that control the force directed layout.
 * Values are persisted per user so tweaks are restored across editor sessions.
 */
UCLASS(Config=NsSpyglassSettings, DefaultConfig)
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

protected:
    virtual FName GetContainerName() const override;

// Variables
public:

    /** Strength of the repulsion force between nodes. */
    UPROPERTY(EditAnywhere, Config, Category="Layout")
    float Repulsion;

    /** Strength of the force pulling nodes toward the center. */
    UPROPERTY(EditAnywhere, Config, Category="Layout")
    float CenterForce;

    /** Attraction Scale between nodes */
    UPROPERTY(EditAnywhere, Config, Category="Layout")
    float AttractionScale;

    /** Zen Mode */
    UPROPERTY(EditAnywhere, Config, Category="Layout")
    bool bZenMode;
};
