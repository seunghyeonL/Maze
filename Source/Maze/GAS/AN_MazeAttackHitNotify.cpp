#include "GAS/AN_MazeAttackHitNotify.h"

#include "Components/SkeletalMeshComponent.h"
#include "Character/Interfaces/AttackHitNotifyReceiver.h"

void UAN_MazeAttackHitNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp)
	{
		return;
	}

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner)
	{
		return;
	}
	
	if (Owner->GetClass()->ImplementsInterface(UAttackHitNotifyReceiver::StaticClass()))
	{
		IAttackHitNotifyReceiver::Execute_NotifyAttackHitWindow(Owner, NotifyId);
	}
}

FString UAN_MazeAttackHitNotify::GetNotifyName_Implementation() const
{
	return TEXT("MazeAttackHit");
}
