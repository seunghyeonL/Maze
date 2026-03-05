#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Sound/SoundMix.h"
#include "Sound/SoundClass.h"
#include "MazeAudioSettings.generated.h"

// Forward declaration
class UAudioSettingsWidget;

/**
 * Centralized audio asset settings for the Maze project.
 * Editable from Project Settings → Game → Maze Audio.
 */
UCLASS(config=Game, defaultconfig, meta=(DisplayName="Maze Audio"))
class MAZE_API UMazeAudioSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UMazeAudioSettings();

    virtual FName GetCategoryName() const override { return FName(TEXT("Game")); }

    // ---- Audio Assets ----
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category="Audio")
    TSoftObjectPtr<USoundMix> MasterSoundMix;

    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category="Audio")
    TSoftObjectPtr<USoundClass> MasterSoundClass;

    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category="Audio")
    TSoftObjectPtr<USoundClass> BGMSoundClass;

    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category="Audio")
    TSoftObjectPtr<USoundClass> SFXSoundClass;

    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category="Audio")
    TSoftClassPtr<UAudioSettingsWidget> AudioSettingsWidgetClass;
};
