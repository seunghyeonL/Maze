#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
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

    // --- Player Color ---
    UPROPERTY(EditDefaultsOnly, Category="PlayerColor")
    FName ColorParameterName = TEXT("Paint Tint");

    UPROPERTY(EditDefaultsOnly, Category="PlayerColor")
    TArray<FLinearColor> PlayerColors = {
        FLinearColor(1.0f,   0.686f, 0.690f, 1.f),  // #ffafb0
        FLinearColor(0.933f, 0.718f, 0.392f, 1.f),  // #eeb764
        FLinearColor(0.992f, 0.980f, 0.529f, 1.f),  // #fdfa87
        FLinearColor(0.686f, 1.0f,   0.729f, 1.f),  // #afffba
    };

    UPROPERTY(ReplicatedUsing=OnRep_PlayerColorIndex)
    int32 PlayerColorIndex = INDEX_NONE;

    UFUNCTION()
    void OnRep_PlayerColorIndex();

    void ApplyPlayerColor();
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement")
    FGameplayTagContainer BlockMovementTags;

    void OnAttackInput(const FInputActionValue& Value);
    
    bool bAbilitiesGranted = false;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
