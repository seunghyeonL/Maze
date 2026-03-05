// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MazePlayerController.generated.h"

UCLASS()
class MAZE_API AMazePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

private:
	/** ESC 키 — 볼륨 설정 팝업 열기/닫기 */
	void ToggleAudioSettings();
};
