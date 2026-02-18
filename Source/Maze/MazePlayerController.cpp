// Fill out your copyright notice in the Description page of Project Settings.

#include "MazePlayerController.h"

#include "UI/LoadingOverlayWidget.h"
#include "UI/CommonModalWidget.h"
#include "Blueprint/UserWidget.h"

AMazePlayerController::AMazePlayerController()
{
}

void AMazePlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = false;

	if (LoadingOverlayClass)
	{
		LoadingOverlay = CreateWidget<ULoadingOverlayWidget>(this, LoadingOverlayClass);
		if (LoadingOverlay)
		{
			LoadingOverlay->AddToViewport(100);
			LoadingOverlay->Show(FText::FromString(TEXT("미로 생성 중...")));
		}
	}
}

void AMazePlayerController::ClientHideLoading_Implementation()
{
	if (LoadingOverlay)
	{
		LoadingOverlay->Hide();
	}
}

void AMazePlayerController::ClientShowGameResult_Implementation(bool bWinner)
{
	if (!ResultModalClass)
	{
		return;
	}

	if (!ResultModal)
	{
		ResultModal = CreateWidget<UCommonModalWidget>(this, ResultModalClass);
		if (ResultModal)
		{
			ResultModal->AddToViewport(200);
		}
	}

	if (ResultModal)
	{
		if (bWinner)
		{
			ResultModal->ShowAlert(
				FText::FromString(TEXT("승리!")),
				FText::FromString(TEXT("축하합니다! 미로를 탈출했습니다!"))
			);
		}
		else
		{
			ResultModal->ShowAlert(
				FText::FromString(TEXT("아쉽네요!")),
				FText::FromString(TEXT("다른 플레이어가 먼저 도착했습니다."))
			);
		}
	}
}
