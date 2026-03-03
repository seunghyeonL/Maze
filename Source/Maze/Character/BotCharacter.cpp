#include "Character/BotCharacter.h"
#include "AI/BotAIController.h"


ABotCharacter::ABotCharacter()
{
	// AI 컨트롤러 자동 점유 설정
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// AI는 컨트롤러 Yaw 회전 사용 안 함
	bUseControllerRotationYaw = false;

	// AIControllerClass는 BP에서 설정하거나 BotAIController 완성 후 추가
	AIControllerClass = ABotAIController::StaticClass();
}

void ABotCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// 봇은 입력이 필요 없음 — StateTree로 제어
	// Super 호출하지 않음 (MazeCharacter의 AttackAction 바인딩 방지)
}
