#include "AI/StateTree/STT_BotStun.h"
#include "AIController.h"
#include "StateTreeExecutionContext.h"

EStateTreeRunStatus FSTT_BotStun::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (InstanceData.AIController)
	{
		InstanceData.AIController->StopMovement();
	}

	return EStateTreeRunStatus::Running;
}

void FSTT_BotStun::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// No cleanup needed — movement will resume when Patrol task starts
}
