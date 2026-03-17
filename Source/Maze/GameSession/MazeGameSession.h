#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameSession.h"
#include "Online/CoreOnline.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "MazeGameSession.generated.h"

/**
 * MazeGameSession
 * 
 * 접속 허가, 매치 상태 관리, 세션 메타데이터를 통합 관리합니다.
 * - ApproveLogin: 게임 시작 여부, 서버 풀 상태 확인
 * - RegisterPlayer: 접속 완료 시 PendingJoinCount 감소
 * - DecPendingJoin: 접속 실패 시 PendingJoinCount 감소 (GameMode::NotifyPendingConnectionLost 포워딩)
 * - SetExpectedPlayers / GetExpectedPlayerCount: 세션 메타데이터 관리
 */
UCLASS()
class MAZE_API AMazeGameSession : public AGameSession
{
	GENERATED_BODY()

public:
	// ===== Login Approval =====
	
	/**
	 * ApproveLogin
	 * 
	 * 클라이언트 접속 승인 여부를 결정합니다.
	 * 1. 부모 클래스 검증 (기본 에러 체크)
	 * 2. 게임 시작 여부 확인
	 * 3. 서버 풀 상태 확인 (PendingJoinCount + 현재 플레이어 >= MaxPlayers)
	 * 4. PendingJoinCount 증가
	 * 
	 * @return 빈 문자열 = 승인, 비어있지 않은 문자열 = 거부 사유
	 */
	virtual void InitOptions(const FString& Options) override;

	virtual FString ApproveLogin(const FString& Options) override;

	/**
	 * RegisterPlayer
	 * 
	 * 클라이언트 접속이 완료되었을 때 호출됩니다.
	 * PendingJoinCount를 감소시킵니다.
	 */
	virtual void RegisterPlayer(APlayerController* NewPlayer, const FUniqueNetIdRepl& UniqueId, bool bWasFromInvite) override;

	/**
	 * DecPendingJoin
	 * 
	 * 접속 실패 시 PendingJoinCount를 감소시킵니다.
	 * GameMode::NotifyPendingConnectionLost에서 호출됩니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Maze|Session")
	void DecPendingJoin();

	// ===== Match State Management =====

	/**
	 * SetMatchStarted
	 * 
	 * 게임 시작 여부를 설정합니다.
	 * true로 설정되면 이후 접속 요청은 거부됩니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Maze|Session")
	void SetMatchStarted(bool bStarted);

	/**
	 * HandleMatchHasStarted
	 * 
	 * 매치 시작 시 호출됩니다.
	 */
	virtual void HandleMatchHasStarted() override;

	/**
	 * HandleMatchHasEnded
	 * 
	 * 매치 종료 시 호출됩니다.
	 */
	virtual void HandleMatchHasEnded() override;

	// ===== Session Metadata =====

	/**
	 * SetExpectedPlayers
	 * 
	 * 세션 메타데이터에 예상 플레이어 수를 저장합니다.
	 * OnlineSubsystem의 SessionSettings에 "ExpectedPlayers" 키로 저장됩니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Maze|Session")
	void SetExpectedPlayers(int32 Count);

	/**
	 * GetExpectedPlayerCount
	 * 
	 * 세션 메타데이터에서 예상 플레이어 수를 읽습니다.
	 * 메타데이터가 없으면 1을 반환합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Maze|Session")
	int32 GetExpectedPlayerCount() const;

	// ===== Getters =====

	UFUNCTION(BlueprintCallable, Category = "Maze|Session")
	bool IsMatchStarted() const { return bMatchStarted; }

	UFUNCTION(BlueprintCallable, Category = "Maze|Session")
	int32 GetPendingJoinCount() const { return PendingJoinCount; }

private:
	/** 게임 시작 여부 */
	bool bMatchStarted = false;

	/** 접속 중인 플레이어 수 (ApproveLogin에서 증가, RegisterPlayer/DecPendingJoin에서 감소) */
	int32 PendingJoinCount = 0;
};
