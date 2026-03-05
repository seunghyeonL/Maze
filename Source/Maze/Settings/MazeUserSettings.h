// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "MazeUserSettings.generated.h"

/**
 * Maze 프로젝트 사용자 설정 (볼륨 등)
 * Config=GameUserSettings.ini 에 자동 저장됨
 */
UCLASS()
class MAZE_API UMazeUserSettings : public UGameUserSettings
{
	GENERATED_BODY()

public:
	UMazeUserSettings(const FObjectInitializer& ObjectInitializer);

	/** 싱글턴 접근자 */
	static UMazeUserSettings* GetMazeUserSettings();

	// ---- Volume ----

	UFUNCTION(BlueprintCallable, Category="Audio")
	void SetMasterVolume(float InVolume);

	UFUNCTION(BlueprintPure, Category="Audio")
	float GetMasterVolume() const { return MasterVolume; }

	UFUNCTION(BlueprintCallable, Category="Audio")
	void SetBGMVolume(float InVolume);

	UFUNCTION(BlueprintPure, Category="Audio")
	float GetBGMVolume() const { return BGMVolume; }

	UFUNCTION(BlueprintCallable, Category="Audio")
	void SetSFXVolume(float InVolume);

	UFUNCTION(BlueprintPure, Category="Audio")
	float GetSFXVolume() const { return SFXVolume; }

	virtual void SetToDefaults() override;

protected:
	UPROPERTY(config, BlueprintReadOnly, Category="Audio")
	float MasterVolume;

	UPROPERTY(config, BlueprintReadOnly, Category="Audio")
	float BGMVolume;

	UPROPERTY(config, BlueprintReadOnly, Category="Audio")
	float SFXVolume;
};
