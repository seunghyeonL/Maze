#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LobbyPlayerEntryItem.generated.h"

UCLASS(BlueprintType)
class MAZE_API ULobbyPlayerEntryItem : public UObject
{
	GENERATED_BODY()

public:
	void Init(const FString& InPlayerName, bool bInReady)
	{
		PlayerName = InPlayerName;
		bIsReady = bInReady;
	}

	UPROPERTY(BlueprintReadOnly)
	FString PlayerName;

	UPROPERTY(BlueprintReadOnly)
	bool bIsReady = false;
};
