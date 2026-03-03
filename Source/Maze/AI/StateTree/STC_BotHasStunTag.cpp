#include "AI/StateTree/STC_BotHasStunTag.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "GAS/MazeGameplayTags.h"
#include "StateTreeExecutionContext.h"

bool FSTC_BotHasStunTag::TestCondition(FStateTreeExecutionContext& Context) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (!InstanceData.Actor)
	{
		return false;
	}

	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(InstanceData.Actor);
	if (!ASI)
	{
		return false;
	}

	UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent();
	if (!ASC)
	{
		return false;
	}

	return ASC->HasMatchingGameplayTag(FMazeGameplayTags::Get().State_Debuff_Stun);
}
