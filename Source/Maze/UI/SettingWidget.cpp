// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/SettingWidget.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Settings/MazeUserSettings.h"
#include "GameFramework/GameUserSettings.h"
#include "CommonModalWidget.h"

void USettingWidget::NativeConstruct()
{
	Super::NativeConstruct();

	MasterVolumeSlider->OnValueChanged.AddDynamic(this, &USettingWidget::OnMasterVolumeChanged);
	BGMVolumeSlider->OnValueChanged.AddDynamic(this, &USettingWidget::OnBGMVolumeChanged);
	SFXVolumeSlider->OnValueChanged.AddDynamic(this, &USettingWidget::OnSFXVolumeChanged);
	CloseButton->OnClicked.AddDynamic(this, &USettingWidget::OnCloseClicked);

	if (ExitToTitleButton)
	{
		ExitToTitleButton->OnClicked.AddDynamic(this, &USettingWidget::OnExitToTitleClicked);
		ExitToTitleButton->SetVisibility(ESlateVisibility::Collapsed);
	}

	ScreenModeComboBox->OnSelectionChanged.AddDynamic(this, &USettingWidget::OnScreenModeChanged);

	InitializeSliderValues();
}

void USettingWidget::InitializeSliderValues()
{
	UMazeUserSettings* Settings = UMazeUserSettings::GetMazeUserSettings();
	if (!Settings)
	{
		return;
	}

	bInitializing = true;

	MasterVolumeSlider->SetValue(Settings->GetMasterVolume());
	BGMVolumeSlider->SetValue(Settings->GetBGMVolume());
	SFXVolumeSlider->SetValue(Settings->GetSFXVolume());

	MasterValueText->SetText(FText::AsNumber(Settings->GetMasterVolume() * 100));
	BGMValueText->SetText(FText::AsNumber(Settings->GetBGMVolume() * 100));
	SFXValueText->SetText(FText::AsNumber(Settings->GetSFXVolume() * 100));

	// Screen mode ComboBox
	static const FString FullscreenOption = TEXT("전체 화면");
	static const FString WindowedOption = TEXT("창 모드");

	ScreenModeComboBox->ClearOptions();
	ScreenModeComboBox->AddOption(FullscreenOption);
	ScreenModeComboBox->AddOption(WindowedOption);

	const EWindowMode::Type CurrentMode = Settings->GetFullscreenMode();
	ScreenModeComboBox->SetSelectedOption(
		CurrentMode == EWindowMode::Windowed ? WindowedOption : FullscreenOption
	);

	bInitializing = false;
}

void USettingWidget::OnMasterVolumeChanged(float Value)
{
	if (bInitializing)
	{
		return;
	}

	if (UMazeUserSettings* Settings = UMazeUserSettings::GetMazeUserSettings())
	{
		Settings->SetMasterVolume(Value);
		MasterValueText->SetText(FText::AsNumber(Value * 100));
	}

	OnVolumeUpdated.ExecuteIfBound();
}

void USettingWidget::OnBGMVolumeChanged(float Value)
{
	if (bInitializing)
	{
		return;
	}

	if (UMazeUserSettings* Settings = UMazeUserSettings::GetMazeUserSettings())
	{
		Settings->SetBGMVolume(Value);
		BGMValueText->SetText(FText::AsNumber(Value * 100));
	}

	OnVolumeUpdated.ExecuteIfBound();
}

void USettingWidget::OnSFXVolumeChanged(float Value)
{
	if (bInitializing)
	{
		return;
	}

	if (UMazeUserSettings* Settings = UMazeUserSettings::GetMazeUserSettings())
	{
		Settings->SetSFXVolume(Value);
		SFXValueText->SetText(FText::AsNumber(Value * 100));
	}

	OnVolumeUpdated.ExecuteIfBound();
}

void USettingWidget::OnScreenModeChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (bInitializing)
	{
		return;
	}

	UMazeUserSettings* Settings = UMazeUserSettings::GetMazeUserSettings();
	if (!Settings)
	{
		return;
	}

	const EWindowMode::Type Mode = SelectedItem == TEXT("창 모드")
		? EWindowMode::Windowed
		: EWindowMode::Fullscreen;

	Settings->SetFullscreenMode(Mode);

	if (Mode == EWindowMode::Windowed)
	{
		Settings->SetScreenResolution(FIntPoint(1280, 720));
	}

	Settings->ApplySettings(false);
}

void USettingWidget::OnCloseClicked()
{
	OnCloseRequested.ExecuteIfBound();
}

void USettingWidget::SetExitToTitleVisible(bool bVisible)
{
	if (ExitToTitleButton)
	{
		ExitToTitleButton->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void USettingWidget::OnExitToTitleClicked()
{
	if (!ConfirmExitModal || ConfirmExitModal->IsShowing())
	{
		return;
	}

	ConfirmExitModal->OnConfirmed.AddDynamic(this, &USettingWidget::HandleExitConfirmed);
	ConfirmExitModal->ShowConfirm(
		FText::FromString(TEXT("나가기")),
		FText::FromString(TEXT("게임에서 나가시겠습니까?"))
	);
}

void USettingWidget::HandleExitConfirmed()
{
	OnExitToTitleRequested.ExecuteIfBound();
}
