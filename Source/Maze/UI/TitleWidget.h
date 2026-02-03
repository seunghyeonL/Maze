// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TitleWidget.generated.h"

class UButton;
class UUIFlowSubsystem;

UCLASS()
class MAZE_API UTitleWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

protected:
	UPROPERTY(meta = (BindWidget))
	UButton* GameStartButton;

	UPROPERTY(meta = (BindWidget))
	UButton* ExitButton;

	UPROPERTY()
	UUIFlowSubsystem* UIFlowSubsystem = nullptr;

	UFUNCTION()
	void HandleGameStartClicked();

	UFUNCTION()
	void HandleExitClicked();

private:
	void CacheSubsystems();
};
