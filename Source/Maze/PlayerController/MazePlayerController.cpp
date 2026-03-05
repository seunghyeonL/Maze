// Fill out your copyright notice in the Description page of Project Settings.

#include "PlayerController/MazePlayerController.h"
#include "UI/AudioSettingsWidget.h"
#include "Settings/MazeUserSettings.h"
#include "AudioDevice.h"
#include "Sound/SoundMix.h"
#include "Sound/SoundClass.h"

void AMazePlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	bShowMouseCursor = false;
	SetInputMode(FInputModeGameOnly());

	InitializeAudio();
}

void AMazePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent)
	{
		InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AMazePlayerController::ToggleAudioSettings);
	}
}

void AMazePlayerController::ToggleAudioSettings()
{
	if (!AudioSettingsWidgetClass)
	{
		return;
	}

	bAudioSettingsOpen = !bAudioSettingsOpen;

	if (bAudioSettingsOpen)
	{
		AudioSettingsWidgetInstance = CreateWidget<UAudioSettingsWidget>(this, AudioSettingsWidgetClass);
		if (!AudioSettingsWidgetInstance)
		{
			bAudioSettingsOpen = false;
			return;
		}

		AudioSettingsWidgetInstance->OnCloseRequested.BindUObject(this, &AMazePlayerController::ToggleAudioSettings);
		AudioSettingsWidgetInstance->OnVolumeUpdated.BindUObject(this, &AMazePlayerController::ApplyAudioSettings);
		AudioSettingsWidgetInstance->AddToViewport(100);

		bShowMouseCursor = true;
		SetInputMode(FInputModeGameAndUI());
	}
	else
	{
		if (AudioSettingsWidgetInstance)
		{
			AudioSettingsWidgetInstance->RemoveFromParent();
			AudioSettingsWidgetInstance = nullptr;
		}

		bShowMouseCursor = false;
		SetInputMode(FInputModeGameOnly());

		if (UMazeUserSettings* Settings = UMazeUserSettings::GetMazeUserSettings())
		{
			Settings->SaveSettings();
		}
	}
}

void AMazePlayerController::InitializeAudio()
{
	if (!MasterSoundMix)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		if (FAudioDeviceHandle AudioDevice = World->GetAudioDevice())
		{
			AudioDevice->SetBaseSoundMix(MasterSoundMix);
		}
	}

	ApplyAudioSettings();
}

void AMazePlayerController::ApplyAudioSettings()
{
	if (!MasterSoundMix)
	{
		return;
	}

	UMazeUserSettings* Settings = UMazeUserSettings::GetMazeUserSettings();
	if (!Settings)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FAudioDeviceHandle AudioDevice = World->GetAudioDevice();
	if (!AudioDevice.IsValid())
	{
		return;
	}

	const float Master = Settings->GetMasterVolume();
	const float Pitch  = 1.f;
	const float Fade   = 0.1f;

	if (MasterSoundClass)
	{
		AudioDevice->SetSoundMixClassOverride(MasterSoundMix, MasterSoundClass, Master, Pitch, Fade, true);
	}
	if (BGMSoundClass)
	{
		AudioDevice->SetSoundMixClassOverride(MasterSoundMix, BGMSoundClass, Settings->GetBGMVolume() * Master, Pitch, Fade, false);
	}
	if (SFXSoundClass)
	{
		AudioDevice->SetSoundMixClassOverride(MasterSoundMix, SFXSoundClass, Settings->GetSFXVolume() * Master, Pitch, Fade, false);
	}
}
