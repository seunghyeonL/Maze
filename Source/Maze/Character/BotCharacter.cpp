#include "Character/BotCharacter.h"
#include "AI/BotAIController.h"
#include "GameFramework/CharacterMovementComponent.h"


ABotCharacter::ABotCharacter()
{
	// AI 컨트롤러 자동 점유 설정
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// AI는 컨트롤러 Yaw 회전 사용 안 함
	bUseControllerRotationYaw = false;

	// AI 컨트롤러 클래스 설정 — ABotAIController가 StateTree + Perception을 담당
	AIControllerClass = ABotAIController::StaticClass();
	
	if (auto* MovementComponent = GetCharacterMovement())
	{
		// 이동 방향으로 자동 회전 (SetFocus가 없을 때 Patrol 상태에서 적용)
		MovementComponent->bOrientRotationToMovement = true;
		MovementComponent->RotationRate = FRotator(0.f, 540.f, 0.f);
		
		// RVO config
		MovementComponent->bUseRVOAvoidance = true;
		MovementComponent->AvoidanceConsiderationRadius = AvoidanceRadius;
		MovementComponent->AvoidanceWeight = AvoidanceWeight;
	}
}

void ABotCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// 봇은 입력이 필요 없음 — StateTree로 제어
	// Super 호출하지 않음 (MazeCharacter의 AttackAction 바인딩 방지)
}
