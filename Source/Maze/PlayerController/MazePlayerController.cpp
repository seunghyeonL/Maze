// Fill out your copyright notice in the Description page of Project Settings.

#include "PlayerController/MazePlayerController.h"
#include "UI/AudioSettingsWidget.h"
#include "Settings/MazeUserSettings.h"
#include "AudioDevice.h"
#include "Sound/SoundMix.h"
#include "Sound/SoundClass.h"
#include "UI/UIFlowSubsystem.h"
#include "OnlineSubsystem/SOSManager.h"
#include "Engine/Engine.h"
#include "Settings/MazeLevelSettings.h"
#include "Kismet/GameplayStatics.h"

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

	if (GEngine)
	{
		GEngine->OnNetworkFailure().RemoveAll(this);
		GEngine->OnNetworkFailure().AddUObject(this, &AMazePlayerController::HandleNetworkFailure);
	}
}

void AMazePlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CleanupAudio();

	if (GEngine)
	{
		GEngine->OnNetworkFailure().RemoveAll(this);
	}
	Super::EndPlay(EndPlayReason);
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
		AudioSettingsWidgetInstance->OnExitToTitleRequested.BindUObject(this, &AMazePlayerController::HandleExitToTitle);
		AudioSettingsWidgetInstance->AddToViewport(100);
		AudioSettingsWidgetInstance->SetExitToTitleVisible(true);

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

void AMazePlayerController::CleanupAudio()
{
	if (!MasterSoundMix)
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

	if (MasterSoundClass)
	{
		AudioDevice->ClearSoundMixClassOverride(MasterSoundMix, MasterSoundClass, 0.f);
	}
	if (BGMSoundClass)
	{
		AudioDevice->ClearSoundMixClassOverride(MasterSoundMix, BGMSoundClass, 0.f);
	}
	if (SFXSoundClass)
	{
		AudioDevice->ClearSoundMixClassOverride(MasterSoundMix, SFXSoundClass, 0.f);
	}

	AudioDevice->PopSoundMixModifier(MasterSoundMix);
}

void AMazePlayerController::HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	(void)World;
	(void)NetDriver;

	if (!IsLocalController())
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("MazePC: NetworkFailure - Type: %d, Error: %s"),
		static_cast<int32>(FailureType), *ErrorString);

	// 에러 메시지 설정: TitleLevel에 도착 후 ConsumePendingError()로 표시됨
	if (UUIFlowSubsystem* Flow = GetGameInstance() ? GetGameInstance()->GetSubsystem<UUIFlowSubsystem>() : nullptr)
	{
		const FText ErrorMessage = FText::FromString(TEXT("호스트와의 연결이 끊어졌습니다."));
		Flow->SetPendingError(ErrorMessage);
		Flow->SetScreenMatch();
	}

	// 세션 정리: 이후 세션 생성/참가가 "Session already exists" 에러 없이 작동하도록
	if (USOSManager* SOS = GetGameInstance() ? GetGameInstance()->GetSubsystem<USOSManager>() : nullptr)
	{
		SOS->DestroySession();
	}

	// TitleLevel로 복귀
	const FString TitleLevelPath = GetDefault<UMazeLevelSettings>()->GetTitleLevelPath();
	ClientTravel(TitleLevelPath, TRAVEL_Absolute);
}

void AMazePlayerController::HandleExitToTitle()
{
	// 1. AudioSettings 위젯 정리
	bAudioSettingsOpen = false;
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

	// 2. UIFlowSubsystem: TitleWidget이 표시되도록 Screen 설정
	if (UUIFlowSubsystem* Flow = GetGameInstance() ? GetGameInstance()->GetSubsystem<UUIFlowSubsystem>() : nullptr)
	{
		Flow->SetScreenTitle();
	}

	// 3. 세션 정리
	if (USOSManager* SOS = GetGameInstance() ? GetGameInstance()->GetSubsystem<USOSManager>() : nullptr)
	{
		SOS->DestroySession();
	}

	// 4. TitleLevel로 standalone 복귀
	const FString TitleLevelPath = GetDefault<UMazeLevelSettings>()->GetTitleLevelPath();
	UGameplayStatics::OpenLevel(this, FName(*TitleLevelPath), true);
}
