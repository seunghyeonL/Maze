#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "MazeGameInstance.generated.h"

/**
 * Custom GameInstance for Maze project.
 * Handles network failures via GEngine->OnNetworkFailure() delegate.
 * Suppresses engine's default double-travel via empty HandleNetworkError override.
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
	 *  Receives ErrorString and handles Korean detection + error message logic. */
	void OnNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);

	/** Guard flag to prevent recursive error handling within 2 seconds. */
	bool bHandlingFailure = false;

	/** Timer handle for failure guard timeout. */
	FTimerHandle FailureGuardTimerHandle;
};
