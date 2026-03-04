#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "MazeLobbyPlayerState.generated.h"

class AMazeLobbyPlayerState;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnReadyChangedBP, AMazeLobbyPlayerState*, PlayerState, bool, bIsReady);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMazeSizeChangedBP, AMazeLobbyPlayerState*, PlayerState, int32, NewMazeSize);

UCLASS()
class MAZE_API AMazeLobbyPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	AMazeLobbyPlayerState();

	UFUNCTION(BlueprintPure, Category="Lobby")
	bool IsReady() const { return bIsReady; }

	UFUNCTION(BlueprintPure, Category="Lobby")
	int32 GetSelectedMazeSize() const { return SelectedMazeSize; }

	UFUNCTION(BlueprintCallable, Category="Lobby")
	void RequestSetReady(bool bNewReady);

	UFUNCTION(BlueprintCallable, Category="Lobby")
	void RequestSetMazeSize(int32 NewSize);

	UPROPERTY(BlueprintAssignable, Category="Lobby")
	FOnReadyChangedBP OnReadyChanged;

	UPROPERTY(BlueprintAssignable, Category="Lobby")
	FOnMazeSizeChangedBP OnMazeSizeChanged;

protected:
	UPROPERTY(ReplicatedUsing=OnRep_IsReady, BlueprintReadOnly, Category="Lobby")
	bool bIsReady = false;

	UPROPERTY(ReplicatedUsing=OnRep_SelectedMazeSize, BlueprintReadOnly, Category="Lobby")
	int32 SelectedMazeSize = 9;

	UFUNCTION()
	void OnRep_IsReady();

	UFUNCTION(Server, Reliable)
	void ServerSetReady(bool bNewReady);

	UFUNCTION()
	void OnRep_SelectedMazeSize();

	UFUNCTION(Server, Reliable)
	void ServerSetMazeSize(int32 NewSize);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
