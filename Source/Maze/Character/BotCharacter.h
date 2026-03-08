#pragma once

#include "CoreMinimal.h"
#include "Character/MazeCharacter.h"
#include "BotCharacter.generated.h"

// ABotAIController 전방 선언 (Wave 2에서 생성 예정)
class ABotAIController;

UCLASS()
class MAZE_API ABotCharacter : public AMazeCharacter
{
	GENERATED_BODY()

public:
	ABotCharacter();

protected:
	// 입력 바인딩 차단 — 봇은 StateTree로 제어
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	
	UPROPERTY(EditAnywhere, Category="RVO")
	float AvoidanceRadius = 200.0f;
    
	UPROPERTY(EditAnywhere, Category="RVO")
	float AvoidanceWeight = 0.5f;
};
