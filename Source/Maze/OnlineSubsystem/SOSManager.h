#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "SOSManager.generated.h"

/**
 * UI에서 다루기 쉬운 “검색 결과 요약”용 구조체
 */
USTRUCT(BlueprintType)
struct FFoundLobbyInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) int32 Index = -1;
	UPROPERTY(BlueprintReadOnly) FString SessionId;
	UPROPERTY(BlueprintReadOnly) FString HostName;
	UPROPERTY(BlueprintReadOnly) int32 PingMs = -1;
	UPROPERTY(BlueprintReadOnly) int32 CurrentPlayers = 0;
	UPROPERTY(BlueprintReadOnly) int32 MaxPlayers = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLobbyCreatedBP, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLobbyDestroyedBP, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLobbiesFoundBP, bool, bSuccess, const TArray<FFoundLobbyInfo>&, Results);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLobbyJoinedBP, bool, bSuccess);

UCLASS()
class MAZE_API USOSManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// ---- BP Events (UMG에서 바인딩) ----
	UPROPERTY(BlueprintAssignable) FOnLobbyCreatedBP OnLobbyCreated;
	UPROPERTY(BlueprintAssignable) FOnLobbiesFoundBP OnLobbiesFound;
	UPROPERTY(BlueprintAssignable) FOnLobbyJoinedBP OnLobbyJoined;
	UPROPERTY(BlueprintAssignable) FOnLobbyDestroyedBP OnLobbyDestroyed;

	// ---- Create / Find / Join / Destroy ----
	UFUNCTION(BlueprintCallable, Category="SOS|Lobby")
	void CreateLobby(int32 MaxPlayers, const FString& LobbyMap, bool bLAN = false);

	UFUNCTION(BlueprintCallable, Category="SOS|Lobby")
	void FindLobbies(int32 MaxResults = 50, bool bLAN = false);

	UFUNCTION(BlueprintCallable, Category="SOS|Lobby")
	void JoinLobbyByIndex(int32 ResultIndex);

	UFUNCTION(BlueprintCallable, Category="SOS|Lobby")
	void DestroyLobby();

	// 마지막 검색 결과를 UI가 필요할 때 다시 가져가고 싶으면
	UFUNCTION(BlueprintPure, Category="SOS|Lobby")
	const TArray<FFoundLobbyInfo>& GetLastFoundLobbies() const { return LastFoundLobbies; }

private:
	IOnlineSessionPtr GetSessionInterface() const;

	void HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void HandleFindSessionsComplete(bool bWasSuccessful);
	void HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful);

private:
	// Steam/Null 공통으로 NAME_GameSession 많이 씀
	static const FName GAME_SESSION_NAME;

	FDelegateHandle CreateHandle;
	FDelegateHandle FindHandle;
	FDelegateHandle JoinHandle;
	FDelegateHandle DestroyHandle;

	TSharedPtr<FOnlineSessionSearch> SessionSearch;

	UPROPERTY() FString PendingLobbyMap = TEXT("/Game/Maps/Lobby"); // 기본값
	UPROPERTY() TArray<FFoundLobbyInfo> LastFoundLobbies;
	UPROPERTY() bool bHosting = false;
};