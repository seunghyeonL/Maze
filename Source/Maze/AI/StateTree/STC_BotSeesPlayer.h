#pragma once

#include "CoreMinimal.h"
#include "StateTreeConditionBase.h"
#include "Conditions/StateTreeAIConditionBase.h"
#include "STC_BotSeesPlayer.generated.h"

class AAIController;

USTRUCT(DisplayName="Bot Sees Player")
struct MAZE_API FSTC_BotSeesPlayerInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category=Context)
	TObjectPtr<AAIController> AIController = nullptr;

};

USTRUCT(DisplayName="Bot Sees Player")
struct MAZE_API FSTC_BotSeesPlayer : public FStateTreeAIConditionBase
{
	GENERATED_BODY()

	using FInstanceDataType = FSTC_BotSeesPlayerInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};
