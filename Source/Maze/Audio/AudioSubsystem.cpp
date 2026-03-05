// Fill out your copyright notice in the Description page of Project Settings.

#include "Audio/AudioSubsystem.h"
#include "AudioDevice.h"
#include "Sound/SoundMix.h"
#include "Sound/SoundClass.h"
#include "Settings/MazeAudioSettings.h"
#include "Settings/MazeUserSettings.h"
#include "UI/AudioSettingsWidget.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"

// ============================================================
//  Lifecycle
// ============================================================

void UAudioSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UAudioSubsystem::OnMapLoaded);
}

void UAudioSubsystem::Deinitialize()
{
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);

	Super::Deinitialize();
}

// ============================================================
//  Audio Initialization & Volume
// ============================================================

void UAudioSubsystem::InitializeAudio()
{
	const UMazeAudioSettings* AudioSettings = GetDefault<UMazeAudioSettings>();
	USoundMix* Mix = AudioSettings->MasterSoundMix.LoadSynchronous();

	if (!Mix)
	{
		UE_LOG(LogTemp, Warning, TEXT("MazeAudio: MasterSoundMix not configured in Project Settings > Game > Maze Audio"));
		return;
	}

	if (UWorld* World = GetGameInstance()->GetWorld())
	{
		FAudioDeviceHandle AudioDevice = World->GetAudioDevice();
		if (AudioDevice.IsValid())
		{
			if (!bAudioInitialized)
			{
				AudioDevice->SetBaseSoundMix(Mix);
				bAudioInitialized = true;
			}
		}
	}

	ApplyAudioSettings();
}

void UAudioSubsystem::ApplyAudioSettings()
{
	const UMazeAudioSettings* AudioSettings = GetDefault<UMazeAudioSettings>();

	USoundMix*   Mix         = AudioSettings->MasterSoundMix.LoadSynchronous();
	USoundClass* MasterClass = AudioSettings->MasterSoundClass.LoadSynchronous();
	USoundClass* BGMClass    = AudioSettings->BGMSoundClass.LoadSynchronous();
	USoundClass* SFXClass    = AudioSettings->SFXSoundClass.LoadSynchronous();

	if (!Mix)
	{
		UE_LOG(LogTemp, Warning, TEXT("MazeAudio: MasterSoundMix not configured in Project Settings > Game > Maze Audio"));
		return;
	}
	if (!MasterClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("MazeAudio: MasterSoundClass not configured in Project Settings > Game > Maze Audio"));
		return;
	}
	if (!BGMClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("MazeAudio: BGMSoundClass not configured in Project Settings > Game > Maze Audio"));
		return;
	}
	if (!SFXClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("MazeAudio: SFXSoundClass not configured in Project Settings > Game > Maze Audio"));
		return;
	}

	UMazeUserSettings* Settings = UMazeUserSettings::GetMazeUserSettings();
	if (!Settings)
	{
		UE_LOG(LogTemp, Warning, TEXT("MazeAudio: MazeUserSettings not available"));
		return;
	}

	UWorld* World = GetGameInstance()->GetWorld();
	if (!World)
	{
		return;
	}

	FAudioDeviceHandle AudioDevice = World->GetAudioDevice();
	if (!AudioDevice.IsValid())
	{
		return;
	}

	const float Master = FMath::Max(Settings->GetMasterVolume(), KINDA_SMALL_NUMBER);

	AudioDevice->SetSoundMixClassOverride(Mix, MasterClass, Master, 1.f, 0.1f, true);
	AudioDevice->SetSoundMixClassOverride(Mix, BGMClass,    FMath::Max(Settings->GetBGMVolume() * Master, KINDA_SMALL_NUMBER), 1.f, 0.1f, false);
	AudioDevice->SetSoundMixClassOverride(Mix, SFXClass,    FMath::Max(Settings->GetSFXVolume() * Master, KINDA_SMALL_NUMBER), 1.f, 0.1f, false);
}

// ============================================================
//  Widget Toggle
// ============================================================

void UAudioSubsystem::ToggleAudioSettings(APlayerController* PC)
{
	if (IsValid(AudioSettingsWidgetInstance))
	{
		CloseAudioSettings();
	}
	else
	{
		OpenAudioSettings(PC);
	}
}

void UAudioSubsystem::OpenAudioSettings(APlayerController* PC)
{
	if (!IsValid(PC))
	{
		return;
	}

	ActivePC = PC;
	bSavedShowMouseCursor = PC->bShowMouseCursor;

	const UMazeAudioSettings* AudioSettings = GetDefault<UMazeAudioSettings>();
	TSubclassOf<UAudioSettingsWidget> WidgetClass = AudioSettings->AudioSettingsWidgetClass.LoadSynchronous();

	if (!WidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("MazeAudio: AudioSettingsWidgetClass not configured in Project Settings > Game > Maze Audio"));
		return;
	}

	AudioSettingsWidgetInstance = CreateWidget<UAudioSettingsWidget>(PC, WidgetClass);
	if (!IsValid(AudioSettingsWidgetInstance))
	{
		return;
	}

	AudioSettingsWidgetInstance->OnCloseRequested.BindUObject(this, &UAudioSubsystem::CloseAudioSettings);
	AudioSettingsWidgetInstance->OnVolumeUpdated.BindUObject(this, &UAudioSubsystem::ApplyAudioSettings);
	AudioSettingsWidgetInstance->AddToViewport(100);

	PC->bShowMouseCursor = true;
	PC->SetInputMode(FInputModeGameAndUI().SetWidgetToFocus(AudioSettingsWidgetInstance->TakeWidget()));
}

void UAudioSubsystem::CloseAudioSettings()
{
	if (IsValid(AudioSettingsWidgetInstance))
	{
		AudioSettingsWidgetInstance->RemoveFromParent();
		AudioSettingsWidgetInstance = nullptr;
	}

	if (ActivePC.IsValid())
	{
		ActivePC->bShowMouseCursor = bSavedShowMouseCursor;

		if (!bSavedShowMouseCursor)
		{
			ActivePC->SetInputMode(FInputModeGameOnly());
		}
		else
		{
			ActivePC->SetInputMode(FInputModeGameAndUI());
		}
	}

	if (UMazeUserSettings* S = UMazeUserSettings::GetMazeUserSettings())
	{
		S->SaveSettings();
	}

	ActivePC.Reset();
}

// ============================================================
//  Map Load Callback
// ============================================================

void UAudioSubsystem::OnMapLoaded(UWorld* LoadedWorld)
{
	if (IsValid(AudioSettingsWidgetInstance))
	{
		AudioSettingsWidgetInstance->RemoveFromParent();
		AudioSettingsWidgetInstance = nullptr;
	}

	if (UMazeUserSettings* S = UMazeUserSettings::GetMazeUserSettings())
	{
		S->SaveSettings();
	}

	ActivePC.Reset();

	// Reset so SetBaseSoundMix runs again on the new world.
	bAudioInitialized = false;

	InitializeAudio();
}
