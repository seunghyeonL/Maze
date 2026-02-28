#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "MazeAnimInstance.generated.h"

/**
 * Base AnimInstance for Maze characters.
 * Reads stun state from the owning pawn's Ability System Component.
 */
UCLASS()
class MAZE_API UMazeAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	/** Whether the character is currently stunned. Updated from ASC stun tag each frame. */
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	bool bIsStunned = false;
};
