// Fill out your copyright notice in the Description page of Project Settings.

#include "GCNA_Trail.h"

#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"

bool AGCNA_Trail::OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	if (!MyTarget || !TrailSystem)
		return false;

	USkeletalMeshComponent* Mesh = MyTarget->FindComponentByClass<USkeletalMeshComponent>();
	if (!Mesh)
		return false;

	// 이미 있으면 중복 생성 방지
	if (SpawnedComp && IsValid(SpawnedComp))
		return true;

	SpawnedComp = UNiagaraFunctionLibrary::SpawnSystemAttached(
		TrailSystem,
		Mesh,
		AttachSocketName,
		FVector(0.f, 0.f, 100.f),
		FRotator::ZeroRotator,
		EAttachLocation::SnapToTarget,
		/*bAutoDestroy*/ false
	);

	return SpawnedComp != nullptr;
}

bool AGCNA_Trail::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
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