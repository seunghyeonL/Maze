#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "Tasks/StateTreeAITask.h"
#include "STT_BotStun.generated.h"

class AAIController;

USTRUCT(DisplayName="Bot Stun")
struct MAZE_API FSTT_BotStunInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category=Context)
	TObjectPtr<AAIController> AIController = nullptr;
};

USTRUCT(DisplayName="Bot Stun")
struct MAZE_API FSTT_BotStun : public FStateTreeAIActionTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FSTT_BotStunInstanceData;

	FSTT_BotStun() { bShouldCallTick = false; }

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
