// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/AudioSettingsWidget.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Settings/MazeUserSettings.h"

void UAudioSettingsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	MasterVolumeSlider->OnValueChanged.AddDynamic(this, &UAudioSettingsWidget::OnMasterVolumeChanged);
	BGMVolumeSlider->OnValueChanged.AddDynamic(this, &UAudioSettingsWidget::OnBGMVolumeChanged);
	SFXVolumeSlider->OnValueChanged.AddDynamic(this, &UAudioSettingsWidget::OnSFXVolumeChanged);
	CloseButton->OnClicked.AddDynamic(this, &UAudioSettingsWidget::OnCloseClicked);

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

	MasterValueText->SetText(FText::AsNumber(Settings->GetMasterVolume()));
	BGMValueText->SetText(FText::AsNumber(Settings->GetBGMVolume()));
	SFXValueText->SetText(FText::AsNumber(Settings->GetSFXVolume()));

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
		MasterValueText->SetText(FText::AsNumber(Value));
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
		BGMValueText->SetText(FText::AsNumber(Value));
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
		SFXValueText->SetText(FText::AsNumber(Value));
	}

	OnVolumeUpdated.ExecuteIfBound();
}

void UAudioSettingsWidget::OnCloseClicked()
{
	OnCloseRequested.ExecuteIfBound();
}
