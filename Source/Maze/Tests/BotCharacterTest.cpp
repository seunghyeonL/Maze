#include "AIController.h"
#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Character/BotCharacter.h"
#include "Character/MazeCharacter.h"
#include "AI/BotAIController.h"
#include "GAS/MazeGameplayTags.h"
#include "AbilitySystemInterface.h"
#include "Components/StateTreeAIComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "GenericTeamAgentInterface.h"

// Test 1: ABotCharacter inherits from AMazeCharacter
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBotCharacterInheritanceTest,
    "Maze.AI.BotCharacter.InheritsFromMazeCharacter",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)
bool FBotCharacterInheritanceTest::RunTest(const FString& Parameters)
{
    // Verify class hierarchy
    TestTrue(
        TEXT("ABotCharacter should inherit from AMazeCharacter"),
        ABotCharacter::StaticClass()->IsChildOf(AMazeCharacter::StaticClass())
    );

    // Verify IAbilitySystemInterface implementation
    TestTrue(
        TEXT("ABotCharacter should implement IAbilitySystemInterface"),
        ABotCharacter::StaticClass()->ImplementsInterface(UAbilitySystemInterface::StaticClass())
    );

    return true;
}

// Test 2: ABotAIController has correct team ID
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBotAIControllerTeamTest,
    "Maze.AI.BotAIController.TeamId",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)
bool FBotAIControllerTeamTest::RunTest(const FString& Parameters)
{
    // Verify ABotAIController inherits from AAIController
    TestTrue(
        TEXT("ABotAIController should inherit from AAIController"),
        ABotAIController::StaticClass()->IsChildOf(AAIController::StaticClass())
    );

    // Verify team ID is 1 (bot team, distinct from player team 0)
    const ABotAIController* CDO = GetDefault<ABotAIController>();
    if (CDO)
    {
        const FGenericTeamId TeamId = CDO->GetGenericTeamId();
        TestEqual(
            TEXT("ABotAIController team ID should be 1 (bot team)"),
            TeamId.GetId(),
            (uint8)1
        );
    }

    return true;
}

// Test 3: MazeGameplayTags StateTree event tags are valid
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMazeGameplayTagsStateTreeTest,
    "Maze.AI.MazeGameplayTags.StateTreeEventTags",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter
)
bool FMazeGameplayTagsStateTreeTest::RunTest(const FString& Parameters)
{
    const FMazeGameplayTags& Tags = FMazeGameplayTags::Get();

    TestTrue(
        TEXT("StateTree_Event_Stunned tag should be valid"),
        Tags.StateTree_Event_Stunned.IsValid()
    );

    TestTrue(
        TEXT("StateTree_Event_StunEnded tag should be valid"),
        Tags.StateTree_Event_StunEnded.IsValid()
    );

    TestTrue(
        TEXT("StateTree_Event_PlayerSpotted tag should be valid"),
        Tags.StateTree_Event_PlayerSpotted.IsValid()
    );

    return true;
}
