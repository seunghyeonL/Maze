#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "SOSManager.generated.h"

/**
 * UI에서 다루기 쉬운 "검색 결과 요약"용 구조체
 */
USTRUCT(BlueprintType)
struct FFoundSessionInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) int32 Index = -1;
	UPROPERTY(BlueprintReadOnly) FString SessionId;
	UPROPERTY(BlueprintReadOnly) FString HostName;
	UPROPERTY(BlueprintReadOnly) int32 PingMs = -1;
	UPROPERTY(BlueprintReadOnly) int32 CurrentPlayers = 0;
	UPROPERTY(BlueprintReadOnly) int32 MaxPlayers = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionCreatedBP, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionDestroyedBP, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSessionsFoundBP, bool, bSuccess, const TArray<FFoundSessionInfo>&, Results);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionJoinedBP, bool, bSuccess);

UCLASS()
class MAZE_API USOSManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// ---- BP Events (UMG에서 바인딩) ----
	UPROPERTY(BlueprintAssignable) mutable FOnSessionCreatedBP OnSessionCreated;
	UPROPERTY(BlueprintAssignable) mutable FOnSessionsFoundBP OnSessionsFound;
	UPROPERTY(BlueprintAssignable) mutable FOnSessionJoinedBP OnSessionJoined;
	UPROPERTY(BlueprintAssignable) mutable FOnSessionDestroyedBP OnSessionDestroyed;

	// ---- Create / Find / Join / Destroy ----
	UFUNCTION(BlueprintCallable, Category="SOS|Session")
	void CreateSession(int32 MaxPlayers, const FString& SessionMap, bool bLAN = false);

	UFUNCTION(BlueprintCallable, Category="SOS|Session")
	void FindSessions(int32 MaxResults = 50, bool bLAN = false);

	UFUNCTION(BlueprintCallable, Category="SOS|Session")
	void JoinSessionByIndex(int32 ResultIndex);

	UFUNCTION(BlueprintCallable, Category="SOS|Session")
	void DestroySession();

	/** GameStart 시점의 실제 접속 인원을 세션 설정에 저장 */
	UFUNCTION(BlueprintCallable, Category="SOS|Session")
	void SetExpectedPlayers(int32 Count);

	// 마지막 검색 결과를 UI가 필요할 때 다시 가져가고 싶으면
	UFUNCTION(BlueprintPure, Category="SOS|Session")
	const TArray<FFoundSessionInfo>& GetLastFoundSessions() const { return LastFoundSessions; }

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

	UPROPERTY() FString PendingSessionMap;
	UPROPERTY() TArray<FFoundSessionInfo> LastFoundSessions;
	UPROPERTY() bool bHosting = false;

	// If a stale GameSession exists (common after disconnect), we destroy first then retry.
	UPROPERTY() bool bPendingCreateAfterDestroy = false;
	UPROPERTY() int32 PendingCreateMaxPlayers = 0;
	UPROPERTY() bool bPendingCreateLAN = false;

	UPROPERTY() bool bPendingJoinAfterDestroy = false;
	UPROPERTY() int32 PendingJoinIndex = -1;
};
