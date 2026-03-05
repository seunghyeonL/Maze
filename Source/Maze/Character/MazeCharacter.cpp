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
#include "Net/UnrealNetwork.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"

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

    // 봇 제외: APlayerController에게만 색 배정
    if (Cast<APlayerController>(NewController))
    {
        if (AGameStateBase* GS = GetWorld()->GetGameState())
        {
            PlayerColorIndex = GS->PlayerArray.IndexOfByPredicate(
                [NewController](const APlayerState* PS)
                {
                    return PS && PS->GetOwningController() == NewController;
                });
            if (PlayerColorIndex < 0)
            {
                PlayerColorIndex = GS->PlayerArray.Num() - 1;
            }
            ApplyPlayerColor(); // Listen Server에서는 OnRep이 호출되지 않으므로 직접 호출
        }
    }
}

void AMazeCharacter::OnRep_PlayerState()
{
    Super::OnRep_PlayerState();

    if (!ASC)
    {
        return;
    }

    ASC->InitAbilityActorInfo(this, this);
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

void AMazeCharacter::OnAttackInput(const FInputActionValue& Value)
{
    if (!ASC || DefaultAbilities.IsEmpty())
    {
        return;
    }

    ASC->TryActivateAbilityByClass(DefaultAbilities[0]);
}

void AMazeCharacter::OnRep_PlayerColorIndex()
{
    ApplyPlayerColor();
}

void AMazeCharacter::ApplyPlayerColor()
{
    if (PlayerColorIndex < 0 || PlayerColorIndex >= PlayerColors.Num())
    {
        return;
    }

    USkeletalMeshComponent* MeshComp = GetMesh();
    if (!MeshComp)
    {
        return;
    }

    const FLinearColor& Color = PlayerColors[PlayerColorIndex];
    for (int32 i = 0; i < MeshComp->GetNumMaterials(); ++i)
    {
        UMaterialInstanceDynamic* DynMat = MeshComp->CreateDynamicMaterialInstance(i);
        if (DynMat)
        {
            DynMat->SetVectorParameterValue(ColorParameterName, Color);
        }
    }
}

void AMazeCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AMazeCharacter, PlayerColorIndex);
}
