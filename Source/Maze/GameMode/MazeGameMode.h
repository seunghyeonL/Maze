// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "MazeGameMode.generated.h"

class AMazePlayerController;

UCLASS()
class MAZE_API AMazeGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	AMazeGameMode();

	/** MazeGoalActor에서 호출 - 서버 전용, 첫 번째 도달자만 처리 */
	void OnGoalReached(APlayerController* Winner);

protected:
	virtual void BeginPlay() override;
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

	// ---- Maze Configuration (BP에서 설정) ----
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Maze")
	int32 MazeWidth = 7;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Maze")
	int32 MazeHeight = 7;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Maze")
	float CellSize = 500.f;

	UPROPERTY(EditDefaultsOnly, Category="Maze")
	TSubclassOf<AActor> WallClass;

	UPROPERTY(EditDefaultsOnly, Category="Maze")
	TSubclassOf<AActor> GoalActorClass;

	/** 결과 표시 후 로비 복귀까지 대기 시간 (초) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Maze")
	float ReturnToLobbyDelay = 3.0f;

private:
	void TryStartMatch();
	void GenerateAndSpawnMaze();
	void SpawnAllPlayers();
	void NotifyGameResult(APlayerController* Winner);
	void ReturnToLobby();
	int32 GetExpectedPlayerCount() const;

	UPROPERTY()
	TArray<TObjectPtr<APlayerController>> WaitingPlayers;

	bool bMazeGenerated = false;
	bool bMatchEnded = false;
	FTimerHandle ReturnTimerHandle;
};