// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/AudioSettingsWidget.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Settings/MazeUserSettings.h"
#include "CommonModalWidget.h"

void UAudioSettingsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	MasterVolumeSlider->OnValueChanged.AddDynamic(this, &UAudioSettingsWidget::OnMasterVolumeChanged);
	BGMVolumeSlider->OnValueChanged.AddDynamic(this, &UAudioSettingsWidget::OnBGMVolumeChanged);
	SFXVolumeSlider->OnValueChanged.AddDynamic(this, &UAudioSettingsWidget::OnSFXVolumeChanged);
	CloseButton->OnClicked.AddDynamic(this, &UAudioSettingsWidget::OnCloseClicked);

	if (ExitToTitleButton)
	{
		ExitToTitleButton->OnClicked.AddDynamic(this, &UAudioSettingsWidget::OnExitToTitleClicked);
		ExitToTitleButton->SetVisibility(ESlateVisibility::Collapsed);
	}

	InitializeSliderValues();
}

void UAudioSettingsWidget::InitializeSliderValues()
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

	bInitializing = false;
}

void UAudioSettingsWidget::OnMasterVolumeChanged(float Value)
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

void UAudioSettingsWidget::OnBGMVolumeChanged(float Value)
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

void UAudioSettingsWidget::OnSFXVolumeChanged(float Value)
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

void UAudioSettingsWidget::OnCloseClicked()
{
	OnCloseRequested.ExecuteIfBound();
}

void UAudioSettingsWidget::SetExitToTitleVisible(bool bVisible)
{
	if (ExitToTitleButton)
	{
		ExitToTitleButton->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UAudioSettingsWidget::OnExitToTitleClicked()
{
	if (!ConfirmExitModal || ConfirmExitModal->IsShowing())
	{
		return;
	}

	ConfirmExitModal->OnConfirmed.AddDynamic(this, &UAudioSettingsWidget::HandleExitConfirmed);
	ConfirmExitModal->ShowConfirm(
		FText::FromString(TEXT("나가기")),
		FText::FromString(TEXT("게임에서 나가시겠습니까?"))
	);
}

void UAudioSettingsWidget::HandleExitConfirmed()
{
	OnExitToTitleRequested.ExecuteIfBound();
}
