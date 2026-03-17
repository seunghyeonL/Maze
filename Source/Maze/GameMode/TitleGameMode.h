// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "TitleGameMode.generated.h"

class AMazeGameSession;

/**
 * 
 */

// DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLoginFailed, const FText&, Title, const FText&, Message);

UCLASS()
class MAZE_API ATitleGameMode : public AGameMode
{
	GENERATED_BODY()
	
public:
	ATitleGameMode();
	
	virtual void NotifyPendingConnectionLost(const FUniqueNetIdRepl& ConnectionUniqueId) override;
};
