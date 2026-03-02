// AttackHitNotifyReceiver.h
#pragma once
#include "UObject/Interface.h"
#include "AttackHitNotifyReceiver.generated.h"

UINTERFACE(BlueprintType, Blueprintable)
class UAttackHitNotifyReceiver : public UInterface
{
	GENERATED_BODY()
};

class IAttackHitNotifyReceiver
{
	GENERATED_BODY()

public:
	// Notify가 호출할 함수(로컬에서 호출됨)
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void NotifyAttackHitWindow(int32 NotifyId);
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void ResetAttackNotifySpamGuard_Server();
};