// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MazePlayerController.generated.h"

class ULoadingOverlayWidget;
class UCommonModalWidget;

UCLASS()
class MAZE_API AMazePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AMazePlayerController();

	/** 서버 → 클라이언트: 로딩 위젯 숨기기 */
	UFUNCTION(Client, Reliable)
	void ClientHideLoading();

	/** 서버 → 클라이언트: 게임 결과 표시 */
	UFUNCTION(Client, Reliable)
	void ClientShowGameResult(bool bWinner);

protected:
	virtual void BeginPlay() override;

	/** 로딩 오버레이 위젯 클래스 (BP에서 설정) */
	UPROPERTY(EditDefaultsOnly, Category="Maze|UI")
	TSubclassOf<ULoadingOverlayWidget> LoadingOverlayClass;

	/** 결과 모달 위젯 클래스 (BP에서 설정) */
	UPROPERTY(EditDefaultsOnly, Category="Maze|UI")
	TSubclassOf<UCommonModalWidget> ResultModalClass;

private:
	UPROPERTY()
	TObjectPtr<ULoadingOverlayWidget> LoadingOverlay;

	UPROPERTY()
	TObjectPtr<UCommonModalWidget> ResultModal;
};
