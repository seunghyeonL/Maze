#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "MazeLobbyPlayerState.generated.h"

class AMazeLobbyPlayerState;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnReadyChangedBP, AMazeLobbyPlayerState*, PlayerState, bool, bIsReady);

UCLASS()
class MAZE_API AMazeLobbyPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	AMazeLobbyPlayerState();

	UFUNCTION(BlueprintPure, Category="Lobby")
	bool IsReady() const { return bIsReady; }



	UFUNCTION(BlueprintCallable, Category="Lobby")
	void RequestSetReady(bool bNewReady);



	UPROPERTY(BlueprintAssignable, Category="Lobby")
	FOnReadyChangedBP OnReadyChanged;



protected:
	UPROPERTY(ReplicatedUsing=OnRep_IsReady, BlueprintReadOnly, Category="Lobby")
	bool bIsReady = false;



	UFUNCTION()
	void OnRep_IsReady();

	UFUNCTION(Server, Reliable)
	void ServerSetReady(bool bNewReady);





	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
