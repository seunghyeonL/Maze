#include "Character/MazeCharacter.h"

#include "Abilities/GameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "GAS/MazeGameplayTags.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayAbilitySpec.h"
#include "InputAction.h"
#include "InputActionValue.h"

AMazeCharacter::AMazeCharacter()
{
    AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
    AbilitySystemComponent->SetIsReplicated(true);
    AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

    WeaponMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
    WeaponMeshComponent->SetupAttachment(GetMesh(), WeaponSocketName);
}

UAbilitySystemComponent* AMazeCharacter::GetAbilitySystemComponent() const
{
    return AbilitySystemComponent;
}

void AMazeCharacter::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);

    if (!AbilitySystemComponent)
    {
        return;
    }

    AbilitySystemComponent->InitAbilityActorInfo(this, this);
    GiveDefaultAbilities();
    RegisterStunCallback();
}

void AMazeCharacter::OnRep_PlayerState()
{
    Super::OnRep_PlayerState();

    if (!AbilitySystemComponent)
    {
        return;
    }

    AbilitySystemComponent->InitAbilityActorInfo(this, this);
    RegisterStunCallback();
}

void AMazeCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
    if (EnhancedInput && AttackAction)
    {
        EnhancedInput->BindAction(AttackAction, ETriggerEvent::Started, this, &AMazeCharacter::OnAttackInput);
    }
}

void AMazeCharacter::GiveDefaultAbilities()
{
    if (!HasAuthority() || !AbilitySystemComponent || bAbilitiesGranted)
    {
        return;
    }

    for (const TSubclassOf<UGameplayAbility>& AbilityClass : DefaultAbilities)
    {
        if (!AbilityClass)
        {
            continue;
        }

        AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1, INDEX_NONE, this));
    }

    bAbilitiesGranted = true;
}

void AMazeCharacter::RegisterStunCallback()
{
    if (!AbilitySystemComponent)
    {
        return;
    }

    AbilitySystemComponent
        ->RegisterGameplayTagEvent(FMazeGameplayTags::Get().State_Debuff_Stun, EGameplayTagEventType::NewOrRemoved)
        .AddUObject(this, &AMazeCharacter::OnStunTagChanged);
}

void AMazeCharacter::OnStunTagChanged(const FGameplayTag Tag, int32 NewCount)
{
    if (!GetCharacterMovement())
    {
        return;
    }

    if (NewCount > 0)
    {
        GetCharacterMovement()->DisableMovement();
        return;
    }

    GetCharacterMovement()->SetMovementMode(MOVE_Walking);
}

void AMazeCharacter::OnAttackInput(const FInputActionValue& Value)
{
    if (!AbilitySystemComponent || DefaultAbilities.IsEmpty())
    {
        return;
    }

    AbilitySystemComponent->TryActivateAbilityByClass(DefaultAbilities[0]);
}
