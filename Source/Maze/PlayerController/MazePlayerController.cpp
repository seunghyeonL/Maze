// Fill out your copyright notice in the Description page of Project Settings.

#include "PlayerController/MazePlayerController.h"
#include "Audio/AudioSubsystem.h"

void AMazePlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	bShowMouseCursor = false;
	SetInputMode(FInputModeGameOnly());

	if (UAudioSubsystem* AudioSub = GetGameInstance() ? GetGameInstance()->GetSubsystem<UAudioSubsystem>() : nullptr)
	{
		AudioSub->InitializeAudio();
	}
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
	if (UAudioSubsystem* AudioSub = GetGameInstance() ? GetGameInstance()->GetSubsystem<UAudioSubsystem>() : nullptr)
	{
		AudioSub->ToggleAudioSettings(this);
	}
}
