#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TitlePlayerController.generated.h"

class UAudioSettingsWidget;
class USoundMix;
class USoundClass;

class UTitleWidget;
class UMatchWidget;
class ULobbyWidget;
class UUserWidget;

UCLASS()
class MAZE_API ATitlePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void ToggleAudioSettings();
	void ApplyAudioSettings();

protected:
	UPROPERTY(EditDefaultsOnly, Category="UI")
	TSubclassOf<UTitleWidget> TitleWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="UI")
	TSubclassOf<UMatchWidget> MatchWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="UI")
	TSubclassOf<ULobbyWidget> LobbyWidgetClass;

	UPROPERTY()
	UUserWidget* ActiveWidget = nullptr;

	// ---- Audio Settings UI ----
	UPROPERTY(EditDefaultsOnly, Category="Audio|UI")
	TSubclassOf<UAudioSettingsWidget> AudioSettingsWidgetClass;

	// ---- SoundMix / SoundClass (에디터에서 할당) ----
	UPROPERTY(EditDefaultsOnly, Category="Audio")
	TObjectPtr<USoundMix> MasterSoundMix;

	UPROPERTY(EditDefaultsOnly, Category="Audio")
	TObjectPtr<USoundClass> MasterSoundClass;

	UPROPERTY(EditDefaultsOnly, Category="Audio")
	TObjectPtr<USoundClass> BGMSoundClass;

	UPROPERTY(EditDefaultsOnly, Category="Audio")
	TObjectPtr<USoundClass> SFXSoundClass;

private:
	void RefreshUI();
	void ClearActiveWidget();
	void SetupUIInput(UUserWidget* Widget);
	void SetupGameInput();
	void HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);
	UFUNCTION() void HandleScreenChanged(EUIFlowScreen NewScreen);

	void InitializeAudio();

	UPROPERTY()
	TObjectPtr<UAudioSettingsWidget> AudioSettingsWidgetInstance;

	bool bAudioSettingsOpen = false;
	bool bSavedShowMouseCursor = false;
};
