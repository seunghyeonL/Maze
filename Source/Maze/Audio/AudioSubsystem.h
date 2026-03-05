// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AudioSubsystem.generated.h"

class UAudioSettingsWidget;
class APlayerController;

/**
 * Centralizes all audio logic: SoundMix initialization, volume application,
 * and AudioSettings widget lifecycle management.
 */
UCLASS()
class MAZE_API UAudioSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Set BaseSoundMix on the current world's AudioDevice and apply volumes. */
	void InitializeAudio();

	/** Apply current user volume settings to the active AudioDevice. */
	void ApplyAudioSettings();

	/** Toggle the AudioSettings popup. Opens if closed; closes if open. */
	void ToggleAudioSettings(APlayerController* PC);

private:
	/** Create and show the AudioSettings widget, capturing mouse. */
	void OpenAudioSettings(APlayerController* PC);

	/** Destroy the AudioSettings widget and restore input state. */
	void CloseAudioSettings();

	/** Called after every map load — cleans up widget and re-initializes audio. */
	UFUNCTION()
	void OnMapLoaded(UWorld* LoadedWorld);

	// ---- Members ----

	TWeakObjectPtr<APlayerController> ActivePC;

	UPROPERTY()
	TObjectPtr<UAudioSettingsWidget> AudioSettingsWidgetInstance;

	bool bSavedShowMouseCursor = false;
	bool bAudioInitialized = false;
};
