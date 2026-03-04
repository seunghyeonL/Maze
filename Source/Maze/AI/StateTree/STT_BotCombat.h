#pragma once

#include "CoreMinimal.h"
#include "Tasks/StateTreeAITask.h"
#include "Templates/SubclassOf.h"
#include "STT_BotCombat.generated.h"

class UGameplayAbility;
class AAIController;

UENUM()
enum class EBotCombatPhase : uint8
{
	Chase,
	PreAttack,
	Attacking,
	AttackCooldown,
};

USTRUCT(DisplayName="Bot Combat")
struct MAZE_API FSTT_BotCombatInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category=Context)
	TObjectPtr<AAIController> AIController = nullptr;

	UPROPERTY(EditAnywhere, Category=Context)
	TObjectPtr<AActor> Actor = nullptr;

	/** The attack ability class to activate (set to GA_MazeAttack in BP) */
	UPROPERTY(EditAnywhere, Category=Parameter)
	TSubclassOf<UGameplayAbility> AttackAbilityClass = nullptr;

	UPROPERTY(EditAnywhere, Category=Parameter)
	float AttackRange = 200.f;

	UPROPERTY(EditAnywhere, Category=Parameter)
	float PreAttackDelay = 0.1f;

	UPROPERTY(EditAnywhere, Category=Parameter)
	float AttackCooldownDuration = 1.0f;
	
	UPROPERTY(EditAnywhere, Category=Parameter)
	float DesiredSpeed = 600.f;

	UPROPERTY()
	float DelayTimer = 0.f;

	UPROPERTY()
	EBotCombatPhase Phase = EBotCombatPhase::Chase;
};

USTRUCT(meta=(DisplayName="Bot Combat", Category="AI|Action"))
struct MAZE_API FSTT_BotCombat : public FStateTreeAIActionTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FSTT_BotCombatInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};
