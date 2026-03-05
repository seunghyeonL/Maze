// Fill out your copyright notice in the Description page of Project Settings.

#include "TitleWidget.h"

#include "UIFlowSubsystem.h"
#include "PlayerController/TitlePlayerController.h"

#include "Components/Button.h"
#include "Kismet/KismetSystemLibrary.h"

void UTitleWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetIsFocusable(true);

	CacheSubsystems();

	if (GameStartButton)
	{
		GameStartButton->OnClicked.RemoveDynamic(this, &UTitleWidget::HandleGameStartClicked);
		GameStartButton->OnClicked.AddDynamic(this, &UTitleWidget::HandleGameStartClicked);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("MazeUI: GameStartButton missing"));
	}

	if (ExitButton)
	{
		ExitButton->OnClicked.RemoveDynamic(this, &UTitleWidget::HandleExitClicked);
		ExitButton->OnClicked.AddDynamic(this, &UTitleWidget::HandleExitClicked);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("MazeUI: ExitButton missing"));
	}

	if (SettingsButton)
	{
		SettingsButton->OnClicked.RemoveDynamic(this, &UTitleWidget::HandleSettingsClicked);
		SettingsButton->OnClicked.AddDynamic(this, &UTitleWidget::HandleSettingsClicked);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("MazeUI: SettingsButton missing"));
	}

	SetKeyboardFocus();
}

void UTitleWidget::CacheSubsystems()
{
	UIFlowSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UUIFlowSubsystem>() : nullptr;
}

void UTitleWidget::HandleGameStartClicked()
{
	UE_LOG(LogTemp, Log, TEXT("MazeUI: Title GameStart clicked"));
	if (UIFlowSubsystem)
	{
		UIFlowSubsystem->SetScreenMatch();
	}
	RemoveFromParent();
}

void UTitleWidget::HandleExitClicked()
{
	UE_LOG(LogTemp, Log, TEXT("MazeUI: Title Exit clicked"));
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, true);
}

void UTitleWidget::HandleSettingsClicked()
{
	UE_LOG(LogTemp, Log, TEXT("MazeUI: Title Settings clicked"));
	if (ATitlePlayerController* TitlePC = Cast<ATitlePlayerController>(GetOwningPlayer()))
	{
		TitlePC->ToggleAudioSettings();
	}
}
