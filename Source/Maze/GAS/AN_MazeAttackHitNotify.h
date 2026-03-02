#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AN_MazeAttackHitNotify.generated.h"

/**
 * AnimNotify that sends a GAS GameplayEvent when the attack hit window occurs.
 * This notifies the GAS ability system that a hit should be processed.
 */
UCLASS()
class MAZE_API UAN_MazeAttackHitNotify : public UAnimNotify
{
	GENERATED_BODY()
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attack")
	int32 NotifyId = 0;

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
};
