#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameSession.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "MazeGameSession.generated.h"

/**
 * MazeGameSession
 * 
 * 접속 허가, 매치 상태 관리, 세션 메타데이터를 통합 관리합니다.
 * - InitOptions: NumPublicConnections → MaxPlayers 동기화 (+1 for Listen Server host)
 * - ApproveLogin: 게임 시작 여부 확인 (정원 체크는 Super::AtCapacity에 위임)
 * - SetExpectedPlayers / GetExpectedPlayerCount: 세션 메타데이터 관리
 */
UCLASS()
class MAZE_API AMazeGameSession : public AGameSession
{
	GENERATED_BODY()

public:
	// ===== Login Approval =====

	virtual void InitOptions(const FString& Options) override;

	/** ApproveLogin — bMatchStarted 체크. 정원 체크는 Super::AtCapacity에 위임 */
	virtual FString ApproveLogin(const FString& Options) override;

	// ===== Match State Management =====

	UFUNCTION(BlueprintCallable, Category = "Maze|Session")
	void SetMatchStarted(bool bStarted);

	virtual void HandleMatchHasStarted() override;
	virtual void HandleMatchHasEnded() override;

	// ===== Session Metadata =====

	UFUNCTION(BlueprintCallable, Category = "Maze|Session")
	void SetExpectedPlayers(int32 Count);

	UFUNCTION(BlueprintCallable, Category = "Maze|Session")
	int32 GetExpectedPlayerCount() const;

	// ===== Getters =====

	UFUNCTION(BlueprintCallable, Category = "Maze|Session")
	bool IsMatchStarted() const { return bMatchStarted; }

private:
	bool bMatchStarted = false;
};
