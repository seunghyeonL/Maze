#include "AI/BotAIController.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Character/BotCharacter.h"
#include "Character/MazeCharacter.h"
#include "Components/StateTreeAIComponent.h"
#include "GAS/MazeGameplayTags.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISenseConfig_Sight.h"
#include "StateTreeEvents.h"

ABotAIController::ABotAIController()
{
    // StateTree component — StateTree asset assigned in BP
    StateTreeComponent = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeComponent"));

    // Perception component
    BotPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComponent"));

    // Sight config
    UAISenseConfig_Sight* SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
    SightConfig->SightRadius = 1500.f;
    SightConfig->LoseSightRadius = 1800.f;
    SightConfig->PeripheralVisionAngleDegrees = 60.f;
    SightConfig->SetMaxAge(5.f);
    SightConfig->DetectionByAffiliation.bDetectEnemies = true;
    SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
    SightConfig->DetectionByAffiliation.bDetectFriendlies = false;

    BotPerceptionComponent->ConfigureSense(*SightConfig);
    BotPerceptionComponent->SetDominantSense(UAISense_Sight::StaticClass());

    // Bind perception callback
    BotPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &ABotAIController::OnTargetPerceptionUpdated);
}

void ABotAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    if (!HasAuthority())
    {
        return;
    }

    // Start StateTree logic manually (auto-start doesn't work reliably)
    if (StateTreeComponent)
    {
        StateTreeComponent->StartLogic();
    }

    // Register GAS stun tag event for GAS→StateTree bridge
    if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(InPawn))
    {
        UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent();
        if (ASC)
        {
            const FMazeGameplayTags& MazeTags = FMazeGameplayTags::Get();
            ASC->RegisterGameplayTagEvent(MazeTags.State_Debuff_Stun, EGameplayTagEventType::NewOrRemoved)
               .AddUObject(this, &ABotAIController::OnStunTagChanged);
        }
    }
}

void ABotAIController::OnUnPossess()
{
    // Unregister stun tag event
    if (APawn* ControlledPawn = GetPawn())
    {
        if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(ControlledPawn))
        {
            UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent();
            if (ASC)
            {
                const FMazeGameplayTags& MazeTags = FMazeGameplayTags::Get();
                ASC->RegisterGameplayTagEvent(MazeTags.State_Debuff_Stun, EGameplayTagEventType::NewOrRemoved)
                   .RemoveAll(this);
            }
        }
    }

    Super::OnUnPossess();
}

void ABotAIController::OnStunTagChanged(const FGameplayTag Tag, int32 Count)
{
    if (!HasAuthority())
    {
        return;
    }

    if (!StateTreeComponent)
    {
        return;
    }

    const FMazeGameplayTags& MazeTags = FMazeGameplayTags::Get();

    if (Count > 0)
    {
        // Stun applied → send Stunned event to StateTree
        FStateTreeEvent Event;
        Event.Tag = MazeTags.StateTree_Event_Stunned;
        StateTreeComponent->SendStateTreeEvent(Event);
    }
    else
    {
        // Stun removed → send StunEnded event to StateTree
        FStateTreeEvent Event;
        Event.Tag = MazeTags.StateTree_Event_StunEnded;
        StateTreeComponent->SendStateTreeEvent(Event);
    }
}

void ABotAIController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
    if (!HasAuthority())
    {
        return;
    }

    if (!Actor)
    {
        return;
    }

    // Only care about MazeCharacter actors that are NOT bots
    AMazeCharacter* MazeChar = Cast<AMazeCharacter>(Actor);
    if (!MazeChar)
    {
        return;
    }

    if (Cast<ABotCharacter>(Actor))
    {
        return;
    }

    if (Stimulus.WasSuccessfullySensed())
    {
        PerceivedPlayer = Actor;

        if (StateTreeComponent)
        {
            FStateTreeEvent Event;
            Event.Tag = FMazeGameplayTags::Get().StateTree_Event_PlayerSpotted;
            StateTreeComponent->SendStateTreeEvent(Event);
        }
    }
    else
    {
        // Lost sight
        PerceivedPlayer = nullptr;
    }
}

AActor* ABotAIController::GetPerceivedPlayer() const
{
    return PerceivedPlayer;
}
