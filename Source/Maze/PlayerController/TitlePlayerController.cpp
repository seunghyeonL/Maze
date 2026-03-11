#include "TitlePlayerController.h"

#include "../UI/LobbyWidget.h"
#include "../UI/MatchWidget.h"
#include "../UI/TitleWidget.h"
#include "../UI/UIFlowSubsystem.h"

#include "OnlineSubsystem/SOSManager.h"

#include "UI/AudioSettingsWidget.h"
#include "Settings/MazeUserSettings.h"
#include "AudioDevice.h"
#include "Sound/SoundMix.h"
#include "Sound/SoundClass.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Settings/MazeLevelSettings.h"

namespace
{
	bool IsMazeLevel(const UWorld* World)
	{
		if (!World)
		{
			return false;
		}

	const FString LevelName = UGameplayStatics::GetCurrentLevelName(World, true);
	return LevelName.Equals(GetDefault<UMazeLevelSettings>()->GetMazeLevelName(), ESearchCase::CaseSensitive);
	}
}

void ATitlePlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	if (GEngine)
	{
		GEngine->OnNetworkFailure().RemoveAll(this);
		GEngine->OnNetworkFailure().AddUObject(this, &ATitlePlayerController::HandleNetworkFailure);
	}

	if (UUIFlowSubsystem* Flow = GetGameInstance() ? GetGameInstance()->GetSubsystem<UUIFlowSubsystem>() : nullptr)
	{
		Flow->OnScreenChanged.RemoveDynamic(this, &ATitlePlayerController::HandleScreenChanged);
		Flow->OnScreenChanged.AddDynamic(this, &ATitlePlayerController::HandleScreenChanged);
	}

	RefreshUI();

	// [ServerTravel crash fix] TitleLevel에서 SoundMix 미사용 — MazePC가 처리
	// InitializeAudio();
}

void ATitlePlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// [ServerTravel crash fix] 초기화하지 않았으므로 정리도 불필요
	// CleanupAudio();

	if (GEngine)
	{
		GEngine->OnNetworkFailure().RemoveAll(this);
	}

	if (UUIFlowSubsystem* Flow = GetGameInstance() ? GetGameInstance()->GetSubsystem<UUIFlowSubsystem>() : nullptr)
	{
		Flow->OnScreenChanged.RemoveDynamic(this, &ATitlePlayerController::HandleScreenChanged);
	}

	ClearActiveWidget();
	Super::EndPlay(EndPlayReason);
}

void ATitlePlayerController::RefreshUI()
{
	if (!IsLocalController())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (IsMazeLevel(World))
	{
		ClearActiveWidget();
		SetupGameInput();
		return;
	}

	UUIFlowSubsystem* Flow = GetGameInstance() ? GetGameInstance()->GetSubsystem<UUIFlowSubsystem>() : nullptr;
	const EUIFlowScreen Screen = Flow ? Flow->GetScreen() : EUIFlowScreen::Title;

	UClass* DesiredClass = nullptr;
	switch (Screen)
	{
	case EUIFlowScreen::Title:
		DesiredClass = TitleWidgetClass;
		break;
	case EUIFlowScreen::Match:
		DesiredClass = MatchWidgetClass;
		break;
	case EUIFlowScreen::Lobby:
		DesiredClass = LobbyWidgetClass;
		break;
	default:
		DesiredClass = TitleWidgetClass;
		break;
	}

	if (!DesiredClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("MazeUI: Missing widget class for screen %d"), static_cast<int32>(Screen));
		return;
	}

	if (ActiveWidget && ActiveWidget->GetClass() == DesiredClass)
	{
		return;
	}

	ClearActiveWidget();

	ActiveWidget = CreateWidget<UUserWidget>(this, DesiredClass);
	if (!ActiveWidget)
	{
		UE_LOG(LogTemp, Warning, TEXT("MazeUI: Failed to create widget for screen %d"), static_cast<int32>(Screen));
		return;
	}

	ActiveWidget->AddToViewport();
	SetupUIInput(ActiveWidget);
}

void ATitlePlayerController::ClearActiveWidget()
{
	if (ActiveWidget)
	{
		ActiveWidget->RemoveFromParent();
		ActiveWidget = nullptr;
	}
}

void ATitlePlayerController::SetupUIInput(UUserWidget* Widget)
{
	bShowMouseCursor = true;

	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	if (Widget)
	{
		InputMode.SetWidgetToFocus(Widget->TakeWidget());
	}
	SetInputMode(InputMode);

	if (Widget)
	{
		Widget->SetKeyboardFocus();
	}
}

void ATitlePlayerController::SetupGameInput()
{
	bShowMouseCursor = false;
	SetInputMode(FInputModeGameOnly());
}

void ATitlePlayerController::HandleScreenChanged(EUIFlowScreen NewScreen)
{
	RefreshUI();
}

void ATitlePlayerController::HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	(void)World;
	(void)NetDriver;

	UE_LOG(LogTemp, Warning, TEXT("MazeUI: NetworkFailure - Type: %d, Error: %s"), 
		static_cast<int32>(FailureType), *ErrorString);

	UUIFlowSubsystem* Flow = GetGameInstance() ? GetGameInstance()->GetSubsystem<UUIFlowSubsystem>() : nullptr;
	if (Flow)
	{
		// PreLogin 거부 메시지나 기타 에러를 pending error로 저장
		// Hangul Syllables 감지 (U+AC00~U+D7AF): 한국어 PreLogin 거부 메시지 보존
		bool bHasKorean = false;
		for (const TCHAR Ch : ErrorString)
		{
			if (Ch >= 0xAC00 && Ch <= 0xD7AF)
			{
				bHasKorean = true;
				break;
			}
		}

		FText ErrorMessage;
		if (ErrorString.Contains(TEXT("full")) || ErrorString.Contains(TEXT("Server is full")))
		{
			ErrorMessage = FText::FromString(TEXT("방이 가득 찼습니다."));
		}
		else if (bHasKorean)
		{
			// 한국어 PreLogin 메시지 (예: "게임이 이미 시작됐습니다.") → 그대로 표시
			ErrorMessage = FText::FromString(ErrorString);
		}
		else
		{
			// 영문 엔진 메시지 또는 빈 문자열 → 한국어 기본 메시지
			ErrorMessage = FText::FromString(TEXT("서버와의 연결이 끊어졌습니다."));
		}

		Flow->SetPendingError(ErrorMessage);
		ClearActiveWidget();  // MatchWidget 로드 오버레이 stuck 방지: 강제 재생성 보장
		Flow->SetScreenMatch();
	}

	// Ensure local OnlineSession state is cleared, otherwise Join/Create can fail with
	// "Session (GameSession) already exists" after a disconnect.
	if (USOSManager* SOS = GetGameInstance() ? GetGameInstance()->GetSubsystem<USOSManager>() : nullptr)
	{
		SOS->DestroySession();
	}

	RefreshUI();
}

void ATitlePlayerController::InitializeAudio()
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

void ATitlePlayerController::CleanupAudio()
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

void ATitlePlayerController::ApplyAudioSettings()
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

void ATitlePlayerController::ToggleAudioSettings()
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

		AudioSettingsWidgetInstance->OnCloseRequested.BindUObject(this, &ATitlePlayerController::ToggleAudioSettings);
		// [ServerTravel crash fix] TitleLevel에서 SoundMix 조정 안 함 — 값은 MazeUserSettings에 자동 저장됨
		// AudioSettingsWidgetInstance->OnVolumeUpdated.BindUObject(this, &ATitlePlayerController::ApplyAudioSettings);
		AudioSettingsWidgetInstance->AddToViewport(100);
		
		// Title은 이미 UI 모드이므로 위젯에 포커스만 설정
		SetInputMode(FInputModeGameAndUI().SetWidgetToFocus(AudioSettingsWidgetInstance->TakeWidget()));
	}
	else
	{
		if (AudioSettingsWidgetInstance)
		{
			AudioSettingsWidgetInstance->RemoveFromParent();
			AudioSettingsWidgetInstance = nullptr;
		}

		// ⚠️ Title 화면 UI 모드 복원 — MazePC처럼 GameOnly로 전환하면 안 된다!
		// ActiveWidget(TitleWidget)에 대한 UI 입력 모드로 복원
		SetupUIInput(ActiveWidget);

		if (UMazeUserSettings* S = UMazeUserSettings::GetMazeUserSettings())
		{
			S->SaveSettings();
		}
	}
}
