#include "AI/StateTree/STT_BotPatrol.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "StateTreeExecutionContext.h"

EStateTreeRunStatus FSTT_BotPatrol::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	PickNextPatrolTarget(Context);
	return EStateTreeRunStatus::Running;
}

void FSTT_BotPatrol::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (InstanceData.AIController)
	{
		InstanceData.AIController->StopMovement();
	}
	InstanceData.bIsMoving = false;
}

EStateTreeRunStatus FSTT_BotPatrol::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (!InstanceData.AIController || !InstanceData.Actor)
	{
		return EStateTreeRunStatus::Failed;
	}

	// Check if we've arrived at the target
	const EPathFollowingStatus::Type MoveStatus = InstanceData.AIController->GetMoveStatus();
	if (MoveStatus == EPathFollowingStatus::Idle || MoveStatus == EPathFollowingStatus::Waiting)
	{
		// Arrived or not moving — pick next target
		PickNextPatrolTarget(Context);
	}

	return EStateTreeRunStatus::Running;
}

void FSTT_BotPatrol::PickNextPatrolTarget(FStateTreeExecutionContext& Context) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (!InstanceData.AIController || !InstanceData.Actor)
	{
		return;
	}

	UWorld* World = InstanceData.Actor->GetWorld();
	if (!World)
	{
		return;
	}

	const FVector CurrentLocation = InstanceData.Actor->GetActorLocation();

	// 4 cardinal directions, each CellSize distance
	const FVector Directions[4] = {
		FVector(InstanceData.CellSize, 0.f, 0.f),
		FVector(-InstanceData.CellSize, 0.f, 0.f),
		FVector(0.f, InstanceData.CellSize, 0.f),
		FVector(0.f, -InstanceData.CellSize, 0.f),
	};

	TArray<FVector> OpenDirections;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(InstanceData.Actor);

	for (const FVector& Dir : Directions)
	{
		// Raycast half-cell distance to detect walls between cells
		FHitResult HitResult;
		const FVector TraceEnd = CurrentLocation + Dir * 0.5f;
		const bool bHit = World->LineTraceSingleByChannel(
			HitResult,
			CurrentLocation,
			TraceEnd,
			ECC_WorldStatic,
			QueryParams
		);

		if (!bHit)
		{
			OpenDirections.Add(CurrentLocation + Dir);
		}
	}

	if (OpenDirections.IsEmpty())
	{
		// Fallback: pick any direction
		const int32 FallbackIdx = FMath::RandRange(0, 3);
		OpenDirections.Add(CurrentLocation + Directions[FallbackIdx]);
	}

	// Pick a random open direction
	const int32 RandomIndex = FMath::RandRange(0, OpenDirections.Num() - 1);
	InstanceData.CurrentTargetLocation = OpenDirections[RandomIndex];

	// Move to the selected location using NavMesh
	FAIMoveRequest MoveRequest;
	MoveRequest.SetGoalLocation(InstanceData.CurrentTargetLocation);
	MoveRequest.SetAcceptanceRadius(InstanceData.AcceptanceRadius);
	InstanceData.AIController->MoveTo(MoveRequest);
	InstanceData.bIsMoving = true;
}
