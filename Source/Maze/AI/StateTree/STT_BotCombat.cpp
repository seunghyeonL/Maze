#include "AI/StateTree/STT_BotCombat.h"
#include "AIController.h"
#include "AI/BotAIController.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "StateTreeExecutionContext.h"
#include "GameFramework/CharacterMovementComponent.h"

EStateTreeRunStatus FSTT_BotCombat::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	InstanceData.Phase = EBotCombatPhase::Chase;
	InstanceData.DelayTimer = 0.f;
	
	if (auto* MovementComponent = InstanceData.Actor->FindComponentByClass<UCharacterMovementComponent>())
	{
		MovementComponent->MaxWalkSpeed = InstanceData.DesiredSpeed;
	}

	// Get target player directly from the AI controller
	ABotAIController* BotController = Cast<ABotAIController>(InstanceData.AIController);
	AActor* TargetPlayer = BotController ? BotController->GetPerceivedPlayer() : nullptr;

	// Start chasing the target
	if (InstanceData.AIController && TargetPlayer)
	{
		InstanceData.AIController->SetFocus(TargetPlayer);
		FAIMoveRequest MoveRequest;
		MoveRequest.SetGoalActor(TargetPlayer);
		MoveRequest.SetAcceptanceRadius(InstanceData.AttackRange * 0.7f);
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
		InstanceData.AIController->ClearFocus(EAIFocusPriority::Gameplay);
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
			else
			{
				// MoveTo가 AcceptanceRadius에서 '도착' 판정 후 멈출 수 있음 → 재이동
				const EPathFollowingStatus::Type MoveStatus = InstanceData.AIController->GetMoveStatus();
				if (MoveStatus != EPathFollowingStatus::Moving)
				{
					FAIMoveRequest MoveRequest;
					MoveRequest.SetGoalActor(TargetPlayer);
					MoveRequest.SetAcceptanceRadius(InstanceData.AttackRange * 0.7f);
					InstanceData.AIController->MoveTo(MoveRequest);
				}
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
			// Looking Target
			FVector PlayerDir = (TargetPlayer->GetActorLocation() - InstanceData.Actor->GetActorLocation()).GetSafeNormal();
			if (PlayerDir != FVector::ZeroVector)
			{
				InstanceData.Actor->SetActorRotation(FRotationMatrix::MakeFromX(PlayerDir).Rotator());
			}
			
			// Activate attack ability (GA_MazeAttack set via AttackAbilityClass parameter)
			if (InstanceData.AttackAbilityClass)
			{
				if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(InstanceData.Actor))
				{
					UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent();
					if (ASC)
					{
						const bool bActivated = ASC->TryActivateAbilityByClass(InstanceData.AttackAbilityClass);
						if (!bActivated)
						{
							UE_LOG(LogTemp, Warning, TEXT("STT_BotCombat: Failed to activate attack ability"));
						}
					}
				}
			}

			// 쿨다운으로 전환 (즉시 Chase 복귀 방지 — 몽타주 인터럽트 방지)
			InstanceData.Phase = EBotCombatPhase::AttackCooldown;
			InstanceData.DelayTimer = 0.f;
			break;
		}

	case EBotCombatPhase::AttackCooldown:
		{
			InstanceData.DelayTimer += DeltaTime;
			if (InstanceData.DelayTimer >= InstanceData.AttackCooldownDuration)
			{
				// 쿨다운 완료 → Chase 복귀 + 재이동
				InstanceData.Phase = EBotCombatPhase::Chase;
				InstanceData.DelayTimer = 0.f;
				if (TargetPlayer)
				{
					FAIMoveRequest MoveRequest;
					MoveRequest.SetGoalActor(TargetPlayer);
					MoveRequest.SetAcceptanceRadius(InstanceData.AttackRange * 0.7f);
					InstanceData.AIController->MoveTo(MoveRequest);
				}
			}
			break;
		}

	default:
		break;
	}

	return EStateTreeRunStatus::Running;
}
