// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MazePlayerController.generated.h"

class UAudioSettingsWidget;
class USoundMix;
class USoundClass;

UCLASS()
class MAZE_API AMazePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	/** 볼륨 변경 시 오디오 디바이스에 적용 */
	void ApplyAudioSettings();

protected:
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
	/** ESC 키 — 볼륨 설정 팝업 열기/닫기 */
	void ToggleAudioSettings();

	/** 레벨 진입 시 저장된 볼륨 초기화 적용 */
	void InitializeAudio();

	UPROPERTY()
	TObjectPtr<UAudioSettingsWidget> AudioSettingsWidgetInstance;

	bool bAudioSettingsOpen = false;
};
