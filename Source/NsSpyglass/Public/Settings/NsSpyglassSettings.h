#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "NsSpyglassSettings.generated.h"

/**
 * Settings that control the force directed layout.
 * Values are persisted per user so tweaks are restored across editor sessions.
 */
UCLASS(Config=EditorPerProjectUserSettings)
class UNsSpyglassSettings : public UDeveloperSettings
{
    GENERATED_BODY()

// Functions
public:

    /** Constructor */
    UNsSpyglassSettings();

    /** Get Spyglass Settings */
    UFUNCTION(BlueprintCallable, Category = "Settings")
    static const UNsSpyglassSettings* GetSettings();

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

    /** Whether to show engine plugins in the graph. */
    UPROPERTY(EditAnywhere, Config, Category="Filters")
    bool bShowEnginePlugins;

    /** Whether to show project plugins in the graph. */
    UPROPERTY(EditAnywhere, Config, Category="Filters")
    bool bShowProjectPlugins;

    /** Whether to show disabled plugins in the graph. */
    UPROPERTY(EditAnywhere, Config, Category="Filters")
    bool bShowDisabledPlugins;

    /** Whether to color nodes by transitive dependency impact. */
    UPROPERTY(EditAnywhere, Config, Category="Filters")
    bool bEnableImpactHeatmap;
};
