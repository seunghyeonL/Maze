// Fill out your copyright notice in the Description page of Project Settings.

#include "Settings/MazeUserSettings.h"
#include "Math/UnrealMathUtility.h"

UMazeUserSettings::UMazeUserSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	MasterVolume = 1.0f;
	BGMVolume    = 1.0f;
	SFXVolume    = 1.0f;
}

UMazeUserSettings* UMazeUserSettings::GetMazeUserSettings()
{
	return Cast<UMazeUserSettings>(UGameUserSettings::GetGameUserSettings());
}

void UMazeUserSettings::SetMasterVolume(float InVolume)
{
	MasterVolume = FMath::Clamp(InVolume, 0.f, 1.f);
}

void UMazeUserSettings::SetBGMVolume(float InVolume)
{
	BGMVolume = FMath::Clamp(InVolume, 0.f, 1.f);
}

void UMazeUserSettings::SetSFXVolume(float InVolume)
{
	SFXVolume = FMath::Clamp(InVolume, 0.f, 1.f);
}

void UMazeUserSettings::SetToDefaults()
{
	Super::SetToDefaults();
	MasterVolume = 1.0f;
	BGMVolume    = 1.0f;
	SFXVolume    = 1.0f;
}
