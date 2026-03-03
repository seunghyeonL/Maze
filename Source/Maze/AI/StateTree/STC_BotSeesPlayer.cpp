#include "AI/StateTree/STC_BotSeesPlayer.h"
#include "AI/BotAIController.h"
#include "StateTreeExecutionContext.h"

bool FSTC_BotSeesPlayer::TestCondition(FStateTreeExecutionContext& Context) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	ABotAIController* BotController = Cast<ABotAIController>(InstanceData.AIController);
	if (!BotController)
	{
		return false;
	}

	AActor* Player = BotController->GetPerceivedPlayer();
	if (!Player)
	{
		return false;
	}

	return true;
}
