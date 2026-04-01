#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "MazeGameInstance.generated.h"

/**
 * Custom GameInstance for Maze project.
 * Handles network failures via GEngine->OnNetworkFailure() delegate (the official C++ extension point).
 * Performs session/UI cleanup; travel is delegated to the engine's ?closed path.
 */
UCLASS()
class MAZE_API UMazeGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
	virtual void Shutdown() override;

private:
	/** Delegate callback for GEngine->OnNetworkFailure().
	 *  Performs session/UI cleanup. Travel is NOT performed here —
	 *  the engine's CallHandleDisconnectForFailure handles it via ?closed → GameDefaultMap (TitleLevel). */
	void OnNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);

	/** Guard flag to prevent duplicate error handling within 2 seconds. */
	bool bHandlingFailure = false;

	/** Timer handle for failure guard timeout. */
	FTimerHandle FailureGuardTimerHandle;
};
