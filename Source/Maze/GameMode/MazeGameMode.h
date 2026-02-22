// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "MazeGameMode.generated.h"

class AMazeGameState;
class AMazeTargetPoint;

UCLASS()
class MAZE_API AMazeGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	AMazeGameMode();

	/** MazeGoalActor에서 호출 - 서버 전용, 첫 번째 도달자만 처리 */
	void OnGoalReached(APlayerController* Winner);

protected:
	virtual void HandleSeamlessTravelPlayer(AController*& C) override;
	virtual void PreLogin(const FString& Options, const FString& Address,
		const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;

	// ---- Maze Configuration ----
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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Maze")
	float ReturnToLobbyDelay = 3.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Maze")
	int32 MinExpectedPlayers = 2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Maze")
	float CountdownDuration = 10.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Maze")
	float ArrivalTimeoutDuration = 30.f;

private:
	void TryStartGameFlow();
	void GenerateAndSpawnMaze();
	void TeleportPlayers();
	void OnArrivalTimeout();
	void ReturnToLobby();
	int32 GetExpectedPlayerCount() const;

	UPROPERTY()
	TArray<TObjectPtr<AController>> ArrivedPlayers;

	UPROPERTY()
	TArray<TObjectPtr<AMazeTargetPoint>> MazeTargetPoints;

	bool bGameFlowStarted = false;
	bool bMatchEnded = false;

	FTimerHandle CountdownTimerHandle;
	FTimerHandle ArrivalTimeoutHandle;
	FTimerHandle ReturnTimerHandle;
};