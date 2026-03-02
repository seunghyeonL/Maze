// Fill out your copyright notice in the Description page of Project Settings.

#include "GCNA_ElectricTrail.h"

#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"

bool AGCNA_ElectricTrail::OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	UE_LOG(LogTemp, Warning, TEXT("Trail Cue Active"));
	if (!MyTarget || !TrailSystem)
		return false;

	UE_LOG(LogTemp, Warning, TEXT("Trail Cue Target: %s"), *MyTarget->GetName());
	USkeletalMeshComponent* Mesh = MyTarget->FindComponentByClass<USkeletalMeshComponent>();
	if (!Mesh)
		return false;

	UE_LOG(LogTemp, Warning, TEXT("Trail Cue Mesh: %s"), *Mesh->GetName());
	// 이미 있으면 중복 생성 방지
	if (SpawnedComp && IsValid(SpawnedComp))
		return true;

	UE_LOG(LogTemp, Warning, TEXT("Trail Cue Already Spawn"));
	SpawnedComp = UNiagaraFunctionLibrary::SpawnSystemAttached(
		TrailSystem,
		Mesh,
		AttachSocketName,
		FVector(0.f, 0.f, 100.f),
		FRotator::ZeroRotator,
		EAttachLocation::SnapToTarget,
		/*bAutoDestroy*/ false
	);

	UE_LOG(LogTemp, Warning, TEXT("Trail Cue Spawned: %s"), *SpawnedComp->GetName());
	return SpawnedComp != nullptr;
}

bool AGCNA_ElectricTrail::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	if (SpawnedComp && IsValid(SpawnedComp))
	{
		// 트레일은 보통 "Deactivate"로 부드럽게 끄고, AutoDestroy를 켜거나 DestroyComponent로 정리
		SpawnedComp->Deactivate();
		SpawnedComp->DestroyComponent();
		SpawnedComp = nullptr;
	}
	return true;
}