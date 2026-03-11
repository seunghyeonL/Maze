// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AudioSettingsWidget.generated.h"

class USlider;
class UTextBlock;
class UButton;
class UCommonModalWidget;

DECLARE_DELEGATE(FOnAudioSettingsCloseRequested);
DECLARE_DELEGATE(FOnAudioVolumeUpdated);
DECLARE_DELEGATE(FOnExitToTitleRequested);

/**
 * 인게임 볼륨 조절 팝업 위젯
 * Master / BGM / SFX 3개 슬라이더 제공
 */
UCLASS()
class MAZE_API UAudioSettingsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Controller에서 바인딩 — 닫기 요청 */
	FOnAudioSettingsCloseRequested OnCloseRequested;

	/** Controller에서 바인딩 — 볼륨 변경 시 오디오 적용 요청 */
	FOnAudioVolumeUpdated OnVolumeUpdated;

	/** Controller에서 바인딩 — 게임 종료 요청 */
	FOnExitToTitleRequested OnExitToTitleRequested;

	/** ExitToTitleButton 표시/숨김 제어 */
	void SetExitToTitleVisible(bool bVisible);

protected:
	virtual void NativeConstruct() override;

	// ---- Bound Widgets ----

	UPROPERTY(meta = (BindWidget))
	USlider* MasterVolumeSlider;

	UPROPERTY(meta = (BindWidget))
	USlider* BGMVolumeSlider;

	UPROPERTY(meta = (BindWidget))
	USlider* SFXVolumeSlider;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* MasterValueText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* BGMValueText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* SFXValueText;

	UPROPERTY(meta = (BindWidget))
	UButton* CloseButton;

	UPROPERTY(meta = (BindWidget))
	UButton* ExitToTitleButton;

	UPROPERTY(meta = (BindWidget))
	UCommonModalWidget* ConfirmExitModal;

private:
	/** 현재 설정값으로 슬라이더/텍스트 초기화 */
	void InitializeSliderValues();

	/** SetValue() 호출 중 OnValueChanged 콜백 방지 가드 */
	bool bInitializing = false;

	// ---- Callbacks ----

	UFUNCTION()
	void OnMasterVolumeChanged(float Value);

	UFUNCTION()
	void OnBGMVolumeChanged(float Value);

	UFUNCTION()
	void OnSFXVolumeChanged(float Value);

	UFUNCTION()
	void OnCloseClicked();

	UFUNCTION()
	void OnExitToTitleClicked();

	UFUNCTION()
	void HandleExitConfirmed();
};
