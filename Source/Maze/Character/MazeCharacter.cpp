#include "Character/MazeCharacter.h"

#include "Abilities/GameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "GAS/MazeGameplayTags.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayAbilitySpec.h"
#include "InputAction.h"
#include "AbilitySystemBlueprintLibrary.h"

AMazeCharacter::AMazeCharacter()
{
    ASC = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
    ASC->SetIsReplicated(true);
    ASC->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

    WeaponMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
    WeaponMeshComponent->SetupAttachment(GetMesh(), WeaponSocketName);
}

UAbilitySystemComponent* AMazeCharacter::GetAbilitySystemComponent() const
{
    return ASC;
}

void AMazeCharacter::Server_RequestAttackHitEvent_Implementation(int32 NotifyId)
{
    // ✅ 서버 스팸 방지: 같은 NotifyId는 무시
    if (NotifyId == LastProcessedAttackNotifyId_Server)
    {
        return;
    }
    LastProcessedAttackNotifyId_Server = NotifyId;
    
    // 서버에서만 실행되는 RPC 구현체
    FGameplayEventData Payload;
    Payload.Instigator = this;
    Payload.Target = nullptr; // 필요하면 채우기

    UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
        this,
        FMazeGameplayTags::Get().Event_Montage_AttackHit,
        Payload
    );
}

void AMazeCharacter::NotifyAttackHitWindow_Implementation(int32 NotifyId)
{
    // 여기서 호출되는 건 보통 클라(로컬) 또는 리슨서버 로컬
    // 원격 클라는 Server RPC로 서버에게 "히트 타이밍"을 알린다.
    Server_RequestAttackHitEvent(NotifyId);
}

void AMazeCharacter::ResetAttackNotifySpamGuard_Server_Implementation()
{
    LastProcessedAttackNotifyId_Server = INDEX_NONE;
}

void AMazeCharacter::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);

    if (!ASC)
    {
        return;
    }

    ASC->InitAbilityActorInfo(this, this);
    GiveDefaultAbilities();
    RegisterStunCallback();
}

void AMazeCharacter::OnRep_PlayerState()
{
    Super::OnRep_PlayerState();

    if (!ASC)
    {
        return;
    }

    ASC->InitAbilityActorInfo(this, this);
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
    if (!HasAuthority() || !ASC || bAbilitiesGranted)
    {
        return;
    }

    for (const TSubclassOf<UGameplayAbility>& AbilityClass : DefaultAbilities)
    {
        if (!AbilityClass)
        {
            continue;
        }

        ASC->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1, INDEX_NONE, this));
    }

    bAbilitiesGranted = true;
}

void AMazeCharacter::RegisterStunCallback()
{
    if (!ASC)
    {
        return;
    }

    ASC
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
    if (!ASC || DefaultAbilities.IsEmpty())
    {
        return;
    }

    ASC->TryActivateAbilityByClass(DefaultAbilities[0]);
}
