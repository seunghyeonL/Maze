// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"
#include "GCNA_Trail.generated.h"

/**
 * 
 */

class UNiagaraSystem;
class UNiagaraComponent;

UCLASS()
class MAZE_API AGCNA_Trail : public AGameplayCueNotify_Actor
{
	GENERATED_BODY()
	
public:
	// Cue 시작(또는 OnActive)에서 스폰
	virtual bool OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;

	// Cue 종료에서 정리
	virtual bool OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category="Trail")
	TObjectPtr<UNiagaraSystem> TrailSystem;

	UPROPERTY(EditDefaultsOnly, Category="Trail")
	FName AttachSocketName = TEXT("HandGrip_R"); // 무기 소켓

private:
	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> SpawnedComp;
};
