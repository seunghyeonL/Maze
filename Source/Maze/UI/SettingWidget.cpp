// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/SettingWidget.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Settings/MazeUserSettings.h"
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
