#include "AI/StateTree/STT_BotCombat.h"
#include "AIController.h"
#include "AI/BotAIController.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "StateTreeExecutionContext.h"

EStateTreeRunStatus FSTT_BotCombat::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	InstanceData.Phase = EBotCombatPhase::Chase;
	InstanceData.DelayTimer = 0.f;

	// Get target player directly from the AI controller
	ABotAIController* BotController = Cast<ABotAIController>(InstanceData.AIController);
	AActor* TargetPlayer = BotController ? BotController->GetPerceivedPlayer() : nullptr;

	// Start chasing the target
	if (InstanceData.AIController && TargetPlayer)
	{
		FAIMoveRequest MoveRequest;
		MoveRequest.SetGoalActor(TargetPlayer);
		MoveRequest.SetAcceptanceRadius(InstanceData.AttackRange);
		InstanceData.AIController->MoveTo(MoveRequest);
	}

	return EStateTreeRunStatus::Running;
}

void FSTT_BotCombat::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (InstanceData.AIController)
	{
		InstanceData.AIController->StopMovement();
	}
	InstanceData.Phase = EBotCombatPhase::Chase;
	InstanceData.DelayTimer = 0.f;
}

EStateTreeRunStatus FSTT_BotCombat::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	// Get target player directly from the AI controller
	ABotAIController* BotController = Cast<ABotAIController>(InstanceData.AIController);
	AActor* TargetPlayer = BotController ? BotController->GetPerceivedPlayer() : nullptr;

	if (!InstanceData.AIController || !InstanceData.Actor || !TargetPlayer)
	{
		return EStateTreeRunStatus::Failed;
	}

	switch (InstanceData.Phase)
	{
	case EBotCombatPhase::Chase:
		{
			// Check if we're close enough to attack
			const float DistToTarget = FVector::Dist(
				InstanceData.Actor->GetActorLocation(),
				TargetPlayer->GetActorLocation()
			);

			if (DistToTarget <= InstanceData.AttackRange)
			{
				// In range — stop and start pre-attack delay
				InstanceData.AIController->StopMovement();
				InstanceData.Phase = EBotCombatPhase::PreAttack;
				InstanceData.DelayTimer = 0.f;
			}
			break;
		}

	case EBotCombatPhase::PreAttack:
		{
			InstanceData.DelayTimer += DeltaTime;
			if (InstanceData.DelayTimer >= InstanceData.PreAttackDelay)
			{
				InstanceData.Phase = EBotCombatPhase::Attacking;
			}
			break;
		}

	case EBotCombatPhase::Attacking:
		{
			// Activate attack ability (GA_MazeAttack set via AttackAbilityClass parameter)
			if (InstanceData.AttackAbilityClass)
			{
				if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(InstanceData.Actor))
				{
					UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent();
					if (ASC)
					{
						ASC->TryActivateAbilityByClass(InstanceData.AttackAbilityClass);
					}
				}
			}

			// Return to chase phase
			InstanceData.Phase = EBotCombatPhase::Chase;
			InstanceData.DelayTimer = 0.f;

			// Re-chase if target moved
			if (TargetPlayer)
			{
				FAIMoveRequest MoveRequest;
				MoveRequest.SetGoalActor(TargetPlayer);
				MoveRequest.SetAcceptanceRadius(InstanceData.AttackRange);
				InstanceData.AIController->MoveTo(MoveRequest);
			}
			break;
		}

	default:
		break;
	}

	return EStateTreeRunStatus::Running;
}
