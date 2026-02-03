#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "OnlineSubsystem/SOSManager.h"
#include "LobbySearchResultItem.generated.h"

UCLASS(BlueprintType)
class MAZE_API ULobbySearchResultItem : public UObject
{
	GENERATED_BODY()

public:
	void Init(const FFoundLobbyInfo& InInfo)
	{
		LobbyInfo = InInfo;
	}

	UPROPERTY(BlueprintReadOnly)
	FFoundLobbyInfo LobbyInfo;
};
