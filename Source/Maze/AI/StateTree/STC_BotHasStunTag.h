#pragma once

#include "CoreMinimal.h"
#include "StateTreeConditionBase.h"
#include "Conditions/StateTreeAIConditionBase.h"
#include "STC_BotHasStunTag.generated.h"

USTRUCT(DisplayName="Bot Has Stun Tag")
struct MAZE_API FSTC_BotHasStunTagInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category=Context)
	TObjectPtr<AActor> Actor = nullptr;
};

USTRUCT(DisplayName="Bot Has Stun Tag")
struct MAZE_API FSTC_BotHasStunTag : public FStateTreeAIConditionBase
{
	GENERATED_BODY()

	using FInstanceDataType = FSTC_BotHasStunTagInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};
