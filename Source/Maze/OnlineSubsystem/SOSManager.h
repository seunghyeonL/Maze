#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "SOSManager.generated.h"

UENUM(BlueprintType)
enum class ESOSState : uint8
{
	Idle       UMETA(DisplayName="Idle"),
	Creating   UMETA(DisplayName="Creating"),
	Finding    UMETA(DisplayName="Finding"),
	Joining    UMETA(DisplayName="Joining"),
	Destroying UMETA(DisplayName="Destroying"),
};

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
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSessionJoinedBP, bool, bSuccess, int32, ResultCode);

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

	UFUNCTION(BlueprintCallable, Category="SOS|Session")
	void ForceReset();

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

	ESOSState State = ESOSState::Idle;

	TSharedPtr<FOnlineSessionSearch> SessionSearch;

	UPROPERTY() FString PendingSessionMap;
	UPROPERTY() TArray<FFoundSessionInfo> LastFoundSessions;

	// If a stale GameSession exists (common after disconnect), we destroy first then retry.
	UPROPERTY() bool bPendingCreateAfterDestroy = false;
	UPROPERTY() int32 PendingCreateMaxPlayers = 0;
	UPROPERTY() bool bPendingCreateLAN = false;

	UPROPERTY() bool bPendingJoinAfterDestroy = false;
	UPROPERTY() FString PendingJoinSessionId;
};
