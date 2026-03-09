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
