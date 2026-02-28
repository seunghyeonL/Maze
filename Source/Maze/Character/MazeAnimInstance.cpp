#include "MazeAnimInstance.h"

#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Pawn.h"
#include "GAS/MazeGameplayTags.h"

void UMazeAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// Get the owning pawn
	APawn* OwnerPawn = TryGetPawnOwner();
	if (!OwnerPawn)
	{
		bIsStunned = false;
		return;
	}

	// Try to get the Ability System Component via IAbilitySystemInterface
	IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(OwnerPawn);
	if (!ASCInterface)
	{
		bIsStunned = false;
		return;
	}

	UAbilitySystemComponent* ASC = ASCInterface->GetAbilitySystemComponent();
	if (!ASC)
	{
		bIsStunned = false;
		return;
	}

	// Check if the stun tag is active
	bIsStunned = ASC->HasMatchingGameplayTag(FMazeGameplayTags::Get().State_Debuff_Stun);
}
