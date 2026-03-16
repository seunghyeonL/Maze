#include "TitlePlayerController.h"

#include "../UI/LobbyWidget.h"
#include "../UI/MatchWidget.h"
#include "../UI/TitleWidget.h"
#include "../UI/UIFlowSubsystem.h"

#include "UI/AudioSettingsWidget.h"
#include "Settings/MazeUserSettings.h"
#include "AudioDevice.h"
#include "Sound/SoundMix.h"
#include "Sound/SoundClass.h"
#include "Blueprint/UserWidget.h"

void ATitlePlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
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
