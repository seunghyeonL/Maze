#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "GameplayTagContainer.h"
#include "GenericTeamAgentInterface.h"
#include "Perception/AIPerceptionTypes.h"
#include "BotAIController.generated.h"

class UStateTreeAIComponent;
class UAIPerceptionComponent;
class UAISenseConfig_Sight;

UCLASS()
class MAZE_API ABotAIController : public AAIController
{
    GENERATED_BODY()

public:
    ABotAIController();

    // IGenericTeamAgentInterface
    virtual FGenericTeamId GetGenericTeamId() const override { return FGenericTeamId(1); }

    // Get the currently perceived player (for StateTree tasks)
    UFUNCTION(BlueprintCallable)
    AActor* GetPerceivedPlayer() const;

protected:
    virtual void OnPossess(APawn* InPawn) override;
    virtual void OnUnPossess() override;

private:
    UPROPERTY(VisibleAnywhere, Category="AI")
    TObjectPtr<UStateTreeAIComponent> StateTreeComponent;

    UPROPERTY(VisibleAnywhere, Category="AI")
    TObjectPtr<UAIPerceptionComponent> BotPerceptionComponent;

    UPROPERTY()
    TObjectPtr<AActor> PerceivedPlayer;

    UFUNCTION()
    void OnStunTagChanged(const FGameplayTag Tag, int32 Count);

    UFUNCTION()
    void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);
};
