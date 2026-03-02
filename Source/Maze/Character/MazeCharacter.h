#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "MazeCharacter.generated.h"

class UAbilitySystemComponent;
class UGameplayAbility;
class UInputAction;
class UStaticMeshComponent;
struct FInputActionValue;
struct FGameplayTag;

UCLASS()
class MAZE_API AMazeCharacter : public ACharacter, public IAbilitySystemInterface
{
    GENERATED_BODY()

public:
    AMazeCharacter();

    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

protected:
    virtual void PossessedBy(AController* NewController) override;
    virtual void OnRep_PlayerState() override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GAS")
    TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon")
    TObjectPtr<UStaticMeshComponent> WeaponMeshComponent;

    UPROPERTY(EditDefaultsOnly, Category="Weapon")
    FName WeaponSocketName = TEXT("HandGrip_R");

    UPROPERTY(EditDefaultsOnly, Category="GAS")
    TArray<TSubclassOf<UGameplayAbility>> DefaultAbilities;

    UPROPERTY(EditDefaultsOnly, Category="Input")
    TObjectPtr<UInputAction> AttackAction;

private:
    void GiveDefaultAbilities();
    void RegisterStunCallback();

    UFUNCTION()
    void OnStunTagChanged(const FGameplayTag Tag, int32 NewCount);

    void OnAttackInput(const FInputActionValue& Value);

    bool bAbilitiesGranted = false;
};
