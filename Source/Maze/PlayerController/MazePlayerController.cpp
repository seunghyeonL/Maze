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
		UE_LOG(LogMazeAudio, Log, TEXT("=== MazePC::BeginPlay === World=%s, NetMode=%d, AudioSub=%s"),
			GetWorld() ? *GetWorld()->GetMapName() : TEXT("NULL"),
			GetWorld() ? static_cast<int32>(GetWorld()->GetNetMode()) : -1,
			AudioSub ? TEXT("VALID") : TEXT("NULL"));
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
