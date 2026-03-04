#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "MazeLobbyGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLobbyMazeSizeChanged, int32, NewMazeSize);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLobbyGameStarted);

UCLASS()
class MAZE_API AMazeLobbyGameState : public AGameState
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category="Lobby")
	int32 GetSelectedMazeSize() const { return SelectedMazeSize; }

	UFUNCTION(BlueprintPure, Category="Lobby")
	bool IsGameStarted() const { return bGameStarted; }

	void SetSelectedMazeSize(int32 NewSize);
	void SetGameStarted(bool bStarted);

	UPROPERTY(BlueprintAssignable, Category="Lobby")
	FOnLobbyMazeSizeChanged OnMazeSizeChanged;

	UPROPERTY(BlueprintAssignable, Category="Lobby")
	FOnLobbyGameStarted OnGameStarted;

protected:
	UPROPERTY(ReplicatedUsing=OnRep_SelectedMazeSize, BlueprintReadOnly, Category="Lobby")
	int32 SelectedMazeSize = 9;

	UPROPERTY(ReplicatedUsing=OnRep_bGameStarted, BlueprintReadOnly, Category="Lobby")
	bool bGameStarted = false;

	UFUNCTION()
	void OnRep_SelectedMazeSize();

	UFUNCTION()
	void OnRep_bGameStarted();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
