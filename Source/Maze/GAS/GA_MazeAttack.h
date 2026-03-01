#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_MazeAttack.generated.h"

class UAnimMontage;
class UGameplayEffect;
struct FGameplayEventData;

UCLASS()
class MAZE_API UGA_MazeAttack : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_MazeAttack();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	TObjectPtr<UAnimMontage> AttackMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	TSubclassOf<UGameplayEffect> StunEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	TSubclassOf<UGameplayEffect> InvincibilityEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	float TraceRadius = 75.f;

	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	float TraceForwardOffset = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	float TraceLength = 50.f;

private:
	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageBlendOut();

	UFUNCTION()
	void OnMontageInterrupted();

	UFUNCTION()
	void OnMontageCancelled();

	UFUNCTION()
	void OnAttackHitEvent(FGameplayEventData Payload);

	void EndAbilityCleanly();
};
