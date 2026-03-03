#include "Character/BotCharacter.h"
#include "AI/BotAIController.h"


ABotCharacter::ABotCharacter()
{
	// AI 컨트롤러 자동 점유 설정
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// AI는 컨트롤러 Yaw 회전 사용 안 함
	bUseControllerRotationYaw = false;

	// AI 컨트롤러 클래스 설정 — ABotAIController가 StateTree + Perception을 담당
	AIControllerClass = ABotAIController::StaticClass();
}

void ABotCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// 봇은 입력이 필요 없음 — StateTree로 제어
	// Super 호출하지 않음 (MazeCharacter의 AttackAction 바인딩 방지)
}
