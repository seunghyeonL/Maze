#include "GAS/GA_MazeAttack.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayTag.h"
#include "Character/Interfaces/AttackHitNotifyReceiver.h"
#include "GAS/MazeGameplayTags.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"

UGA_MazeAttack::UGA_MazeAttack()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	SetAssetTags(FGameplayTagContainer({FMazeGameplayTags::Get().Ability_Attack}));
	// AbilityTags.AddTag(FMazeGameplayTags::Get().Ability_Attack); - Deprecated
	ActivationBlockedTags.AddTag(FMazeGameplayTags::Get().State_Debuff_Stun);
}

void UGA_MazeAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	// RPC spam guard
	// if (AActor* Avatar = GetAvatarActorFromActorInfo(); Avatar && Avatar->HasAuthority())
	// {
	// 	if (Avatar->GetClass()->ImplementsInterface(UAttackHitNotifyReceiver::StaticClass()))
	// 	{
	// 		IAttackHitNotifyReceiver::Execute_ResetAttackNotifySpamGuard_Server(Avatar);
	// 	}
	// }

	if (!AttackMontage)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, AttackMontage, 1.f, NAME_None, true);
	MontageTask->OnCompleted.AddDynamic(this, &UGA_MazeAttack::OnMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &UGA_MazeAttack::OnMontageBlendOut);
	MontageTask->OnInterrupted.AddDynamic(this, &UGA_MazeAttack::OnMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &UGA_MazeAttack::OnMontageCancelled);
	MontageTask->ReadyForActivation();

	UAbilityTask_WaitGameplayEvent* EventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, FMazeGameplayTags::Get().Event_Montage_AttackHit, nullptr, true, false);
	EventTask->EventReceived.AddDynamic(this, &UGA_MazeAttack::OnAttackHitEvent);
	EventTask->ReadyForActivation();

	UAbilityTask_WaitGameplayTagAdded* StunWatchTask = UAbilityTask_WaitGameplayTagAdded::WaitGameplayTagAdd(
		this, FMazeGameplayTags::Get().State_Debuff_Stun);
	StunWatchTask->Added.AddDynamic(this, &UGA_MazeAttack::OnStunTagAdded);
	StunWatchTask->ReadyForActivation();
}

void UGA_MazeAttack::OnMontageCompleted()
{
	EndAbilityCleanly();
}

void UGA_MazeAttack::OnMontageBlendOut()
{
	EndAbilityCleanly();
}

void UGA_MazeAttack::OnMontageInterrupted()
{
	EndAbilityCleanly();
}

void UGA_MazeAttack::OnMontageCancelled()
{
	EndAbilityCleanly();
}

void UGA_MazeAttack::OnAttackHitEvent(FGameplayEventData Payload)
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar || !Avatar->HasAuthority())
	{
		return;
	}

	UWorld* World = Avatar->GetWorld();
	if (!World)
	{
		return;
	}
	
	const FVector Start = Avatar->GetActorLocation() + Avatar->GetActorForwardVector() * TraceForwardOffset;
	const FVector End = Start + Avatar->GetActorForwardVector() * TraceLength;
	
	// debug line
	// DrawDebugSphere(GetWorld(), Start, TraceRadius, 16, FColor::Green, false, 1.0f);
	// DrawDebugSphere(GetWorld(), End,   TraceRadius, 16, FColor::Red,   false, 1.0f);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Avatar);

	TArray<FHitResult> HitResults;
	World->SweepMultiByChannel(
		HitResults,
		Start,
		End,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(TraceRadius),
		QueryParams);
	
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!SourceASC)
	{
		return;
	}
	
	bool bHitSuccess = false;
	for (const FHitResult& Hit : HitResults)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor || HitActor == Avatar)
		{
			continue;
		}

		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor);
		if (!TargetASC)
		{
			continue;
		}
		
		bHitSuccess = true;
		
		/* GE 면역설정으로 대체 */
		// if (TargetASC->HasMatchingGameplayTag(FMazeGameplayTags::Get().State_Invincible))
		// {
		// 	UE_LOG(LogTemp, Log, TEXT("GA_MazeAttack: Hit %s but they are invincible, skipping"), *HitActor->GetName());
		// 	continue;
		// }

		if (StunEffectClass)
		{
			FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
			Context.AddInstigator(Avatar, Avatar);
			FGameplayEffectSpecHandle StunSpec = SourceASC->MakeOutgoingSpec(StunEffectClass, 1.f, Context);
			if (StunSpec.IsValid() && StunSpec.Data.IsValid())
			{
				SourceASC->ApplyGameplayEffectSpecToTarget(*StunSpec.Data.Get(), TargetASC);
				UE_LOG(LogTemp, Log, TEXT("GA_MazeAttack: Applied stun to %s"), *HitActor->GetName());
			}
		}

		if (InvincibilityEffectClass)
		{
			FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
			Context.AddInstigator(Avatar, Avatar);
			FGameplayEffectSpecHandle InvincSpec = SourceASC->MakeOutgoingSpec(InvincibilityEffectClass, 1.f, Context);
			if (InvincSpec.IsValid() && InvincSpec.Data.IsValid())
			{
				SourceASC->ApplyGameplayEffectSpecToTarget(*InvincSpec.Data.Get(), TargetASC);
			}
		}
	}
	
	// if (bHitSuccess && Avatar->GetClass()->ImplementsInterface(UAttackHitNotifyReceiver::StaticClass()))
	// {
	// 	IAttackHitNotifyReceiver::Execute_PlayHitSound(Avatar);
	// }
	
	if (bHitSuccess && HitSoundEffectClass)
	{
		FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
		Context.AddInstigator(Avatar, Avatar);
		FGameplayEffectSpecHandle HitSpec = SourceASC->MakeOutgoingSpec(HitSoundEffectClass, 1.f, Context);
		
		if (HitSpec.IsValid() && HitSpec.Data.IsValid())
		{
			SourceASC->ApplyGameplayEffectSpecToSelf(*HitSpec.Data.Get());
		}
	}
}

void UGA_MazeAttack::OnStunTagAdded()
{
	EndAbilityCleanly();
}

void UGA_MazeAttack::EndAbilityCleanly()
{
	if (!IsActive())
	{
		return;
	}
	
	const FGameplayAbilitySpecHandle Handle = GetCurrentAbilitySpecHandle();
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	const FGameplayAbilityActivationInfo ActivationInfo = GetCurrentActivationInfo();
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
