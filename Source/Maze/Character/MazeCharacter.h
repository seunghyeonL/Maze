#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "Interfaces/AttackHitNotifyReceiver.h"
#include "MazeCharacter.generated.h"

class UAbilitySystemComponent;
class UGameplayAbility;
class UInputAction;
class UStaticMeshComponent;
struct FInputActionValue;
struct FGameplayTag;

UCLASS()
class MAZE_API AMazeCharacter : public ACharacter, public IAbilitySystemInterface, public IAttackHitNotifyReceiver
{
    GENERATED_BODY()

public:
    AMazeCharacter();

    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
    
    UFUNCTION(Server, Reliable)
    void Server_RequestAttackHitEvent(int32 NotifyId);
    
    virtual void NotifyAttackHitWindow_Implementation(int32 NotifyId) override;
    
    virtual void ResetAttackNotifySpamGuard_Server_Implementation() override;

protected:
    virtual void PossessedBy(AController* NewController) override;
    virtual void OnRep_PlayerState() override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GAS")
    TObjectPtr<UAbilitySystemComponent> ASC;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon")
    TObjectPtr<UStaticMeshComponent> WeaponMeshComponent;

    UPROPERTY(EditDefaultsOnly, Category="Weapon")
    FName WeaponSocketName = TEXT("HandGrip_R");

    UPROPERTY(EditDefaultsOnly, Category="GAS")
    TArray<TSubclassOf<UGameplayAbility>> DefaultAbilities;

    UPROPERTY(EditDefaultsOnly, Category="Input")
    TObjectPtr<UInputAction> AttackAction;
    
    // 스팸 방지: 같은 NotifyId는 한 번만 처리
    UPROPERTY()
    int32 LastProcessedAttackNotifyId_Server = INDEX_NONE;
    
    void GiveDefaultAbilities();
    void RegisterStunCallback();

    UFUNCTION()
    void OnStunTagChanged(const FGameplayTag Tag, int32 NewCount);

    void OnAttackInput(const FInputActionValue& Value);

    bool bAbilitiesGranted = false;
};
