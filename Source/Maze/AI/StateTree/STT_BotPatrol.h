#pragma once

#include "CoreMinimal.h"
#include "Tasks/StateTreeAITask.h"
#include "STT_BotPatrol.generated.h"

class AAIController;

USTRUCT(DisplayName="Bot Patrol")
struct MAZE_API FSTT_BotPatrolInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category=Context)
	TObjectPtr<AAIController> AIController = nullptr;

	UPROPERTY(EditAnywhere, Category=Context)
	TObjectPtr<AActor> Actor = nullptr;

	UPROPERTY(EditAnywhere, Category=Parameter)
	float CellSize = 400.f;

	UPROPERTY(EditAnywhere, Category=Parameter)
	float AcceptanceRadius = 50.f;

	UPROPERTY()
	FVector CurrentTargetLocation = FVector::ZeroVector;

	UPROPERTY()
	bool bIsMoving = false;
};

USTRUCT(meta=(DisplayName="Bot Patrol", Category="AI|Action"))
struct MAZE_API FSTT_BotPatrol : public FStateTreeAIActionTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FSTT_BotPatrolInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

private:
	void PickNextPatrolTarget(FStateTreeExecutionContext& Context) const;
};
