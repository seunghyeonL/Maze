#include "Character/BotCharacter.h"
// ABotAIController.h는 Wave 2 완료 후 추가 예정
// 현재는 전방 선언만으로 AIControllerClass 설정 가능

ABotCharacter::ABotCharacter()
{
	// AI 컨트롤러 자동 점유 설정
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// AI는 컨트롤러 Yaw 회전 사용 안 함
	bUseControllerRotationYaw = false;

	// AIControllerClass는 BP에서 설정하거나 BotAIController 완성 후 추가
	// AIControllerClass = ABotAIController::StaticClass(); // Wave 2 후 활성화
}

void ABotCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// 봇은 입력이 필요 없음 — StateTree로 제어
	// Super 호출하지 않음 (MazeCharacter의 AttackAction 바인딩 방지)
}
