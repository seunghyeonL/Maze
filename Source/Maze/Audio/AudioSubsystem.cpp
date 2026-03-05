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

DEFINE_LOG_CATEGORY(LogMazeAudio);

// ============================================================
//  Lifecycle
// ============================================================

void UAudioSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UAudioSubsystem::OnMapLoaded);
	UE_LOG(LogMazeAudio, Log, TEXT("=== AudioSubsystem::Initialize === Delegate registered"));
}

void UAudioSubsystem::Deinitialize()
{
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);
	UE_LOG(LogMazeAudio, Log, TEXT("=== AudioSubsystem::Deinitialize === Delegate unregistered"));

	Super::Deinitialize();
}

// ============================================================
//  Audio Initialization & Volume
// ============================================================

void UAudioSubsystem::InitializeAudio()
{
	UE_LOG(LogMazeAudio, Log, TEXT("=== InitializeAudio === Called"));

	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (!World)
	{
		UE_LOG(LogMazeAudio, Warning, TEXT("InitializeAudio: World is NULL, skipping"));
		return;
	}

	// Defer to next tick: during ServerTravel the AudioDevice's adjuster
	// hierarchy is not fully ready in BeginPlay, causing
	// RecursiveApplyAdjuster crashes in packaged builds.
	World->GetTimerManager().ClearTimer(AudioInitTimerHandle);
	UE_LOG(LogMazeAudio, Log, TEXT("InitializeAudio: Timer scheduled on World=%s (Ptr=0x%p)"), *World->GetMapName(), World);
	World->GetTimerManager().SetTimerForNextTick(
		FTimerDelegate::CreateUObject(this, &UAudioSubsystem::InitializeAudioDeferred));
}

void UAudioSubsystem::InitializeAudioDeferred()
{
	if (UWorld* W = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr)
	{
		UE_LOG(LogMazeAudio, Log, TEXT("=== InitializeAudioDeferred === World=%s (Ptr=0x%p), NetMode=%d, bIsTearingDown=%s"),
			*W->GetMapName(), W, static_cast<int32>(W->GetNetMode()), W->bIsTearingDown ? TEXT("TRUE") : TEXT("FALSE"));
	}
	else
	{
		UE_LOG(LogMazeAudio, Warning, TEXT("=== InitializeAudioDeferred === World is NULL at entry"));
	}

	const UMazeAudioSettings* AudioSettings = GetDefault<UMazeAudioSettings>();
	USoundMix* Mix = AudioSettings->MasterSoundMix.LoadSynchronous();
	UE_LOG(LogMazeAudio, Log, TEXT("InitializeAudioDeferred: Mix loaded=%s"), Mix ? TEXT("OK") : TEXT("NULL"));

	if (!Mix)
	{
		UE_LOG(LogTemp, Warning, TEXT("MazeAudio: MasterSoundMix not configured in Project Settings > Game > Maze Audio"));
		return;
	}

	if (UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr)
	{
		if (World->bIsTearingDown)
		{
			UE_LOG(LogMazeAudio, Warning, TEXT("InitializeAudioDeferred: World is tearing down, aborting"));
			UE_LOG(LogTemp, Warning, TEXT("MazeAudio: Cannot initialize audio during world tear down"));
			return;
		}

		FAudioDeviceHandle AudioDevice = World->GetAudioDevice();
		UE_LOG(LogMazeAudio, Log, TEXT("InitializeAudioDeferred: AudioDevice valid=%s"), AudioDevice.IsValid() ? TEXT("YES") : TEXT("NO"));
		if (AudioDevice.IsValid())
		{
			if (!bAudioInitialized)
			{
				AudioDevice->SetBaseSoundMix(Mix);
				bAudioInitialized = true;
				UE_LOG(LogMazeAudio, Log, TEXT("InitializeAudioDeferred: SetBaseSoundMix called, bAudioInitialized now true"));
			}
		}
	}

	UE_LOG(LogMazeAudio, Log, TEXT("InitializeAudioDeferred: Calling ApplyAudioSettings..."));
	ApplyAudioSettings();
}

void UAudioSubsystem::ApplyAudioSettings()
{
	UE_LOG(LogMazeAudio, Log, TEXT("=== ApplyAudioSettings === Called"));

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

	UE_LOG(LogMazeAudio, Log, TEXT("ApplyAudioSettings: Assets - Mix=%s, MasterClass=%s, BGMClass=%s, SFXClass=%s"),
		Mix ? TEXT("OK") : TEXT("NULL"),
		MasterClass ? TEXT("OK") : TEXT("NULL"),
		BGMClass ? TEXT("OK") : TEXT("NULL"),
		SFXClass ? TEXT("OK") : TEXT("NULL"));

	UMazeUserSettings* Settings = UMazeUserSettings::GetMazeUserSettings();
	if (!Settings)
	{
		UE_LOG(LogTemp, Warning, TEXT("MazeAudio: MazeUserSettings not available"));
		return;
	}

	UWorld* World = GetGameInstance()->GetWorld();
	if (!World)
	{
		UE_LOG(LogMazeAudio, Warning, TEXT("ApplyAudioSettings: World is NULL, aborting"));
		return;
	}
	if (World->bIsTearingDown)
	{
		UE_LOG(LogMazeAudio, Warning, TEXT("ApplyAudioSettings: World='%s' is tearing down, aborting SetSoundMixClassOverride"), *World->GetMapName());
		return;
	}

	FAudioDeviceHandle AudioDevice = World->GetAudioDevice();
	UE_LOG(LogMazeAudio, Log, TEXT("ApplyAudioSettings: World=%s, NetMode=%d, bIsTearingDown=%s, AudioDevice=%s"),
		*World->GetMapName(), static_cast<int32>(World->GetNetMode()),
		World->bIsTearingDown ? TEXT("TRUE") : TEXT("FALSE"),
		AudioDevice.IsValid() ? TEXT("VALID") : TEXT("INVALID"));

	if (!AudioDevice.IsValid())
	{
		return;
	}

	const float Master = FMath::Max(Settings->GetMasterVolume(), KINDA_SMALL_NUMBER);
	UE_LOG(LogMazeAudio, Log, TEXT("ApplyAudioSettings: Applying overrides - Master=%.4f, BGM=%.4f, SFX=%.4f"),
		Master,
		FMath::Max(Settings->GetBGMVolume() * Master, KINDA_SMALL_NUMBER),
		FMath::Max(Settings->GetSFXVolume() * Master, KINDA_SMALL_NUMBER));

	AudioDevice->SetSoundMixClassOverride(Mix, MasterClass, Master, 1.f, 0.1f, true);
	AudioDevice->SetSoundMixClassOverride(Mix, BGMClass,    FMath::Max(Settings->GetBGMVolume() * Master, KINDA_SMALL_NUMBER), 1.f, 0.1f, false);
	AudioDevice->SetSoundMixClassOverride(Mix, SFXClass,    FMath::Max(Settings->GetSFXVolume() * Master, KINDA_SMALL_NUMBER), 1.f, 0.1f, false);

	UE_LOG(LogMazeAudio, Log, TEXT("=== ApplyAudioSettings COMPLETE ==="));
}

// ============================================================
//  Widget Toggle
// ============================================================

void UAudioSubsystem::ToggleAudioSettings(APlayerController* PC)
{
	UE_LOG(LogMazeAudio, Log, TEXT("ToggleAudioSettings: widgetExists=%s"), IsValid(AudioSettingsWidgetInstance) ? TEXT("YES") : TEXT("NO"));
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

	UE_LOG(LogMazeAudio, Log, TEXT("OpenAudioSettings: WidgetClass=%s, Created=%s"),
		WidgetClass ? *WidgetClass->GetName() : TEXT("NULL"),
		IsValid(AudioSettingsWidgetInstance) ? TEXT("YES") : TEXT("NO - check later"));

	if (!WidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("MazeAudio: AudioSettingsWidgetClass not configured in Project Settings > Game > Maze Audio"));
		return;
	}

	AudioSettingsWidgetInstance = CreateWidget<UAudioSettingsWidget>(PC, WidgetClass);
	UE_LOG(LogMazeAudio, Log, TEXT("OpenAudioSettings: Widget created=%s"), IsValid(AudioSettingsWidgetInstance) ? TEXT("YES") : TEXT("NO"));
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
	UE_LOG(LogMazeAudio, Log, TEXT("CloseAudioSettings: Saving and cleaning up"));
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
	UE_LOG(LogMazeAudio, Log, TEXT("=== OnMapLoaded === LoadedWorld=%s (Ptr=0x%p), bIsTearingDown=%s"),
		LoadedWorld ? *LoadedWorld->GetMapName() : TEXT("NULL"),
		LoadedWorld,
		(LoadedWorld && LoadedWorld->bIsTearingDown) ? TEXT("TRUE") : TEXT("FALSE"));

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

	UE_LOG(LogMazeAudio, Log, TEXT("OnMapLoaded: Widget cleanup done, bAudioInitialized reset to false"));

	// Reset so SetBaseSoundMix runs again on the new world's BeginPlay.
	// Do NOT call InitializeAudio() here: PostLoadMapWithWorld fires before
	// actors BeginPlay, so the AudioDevice adjuster hierarchy may not be
	// ready yet (causes RecursiveApplyAdjuster crash in packaged builds).
	// Both TitlePlayerController and MazePlayerController already call
	// InitializeAudio() in their BeginPlay — the safe point.
	bAudioInitialized = false;
}
