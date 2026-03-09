#include "MazeGameplayTags.h"
#include "GameplayTagsManager.h"

FMazeGameplayTags FMazeGameplayTags::GameplayTags;

void FMazeGameplayTags::InitializeNativeTags()
{
	UGameplayTagsManager& Manager = UGameplayTagsManager::Get();

	// Register native tags
	GameplayTags.Ability_Attack = Manager.AddNativeGameplayTag(
		FName("Ability.Attack"),
		FString("Attack ability tag")
	);

	GameplayTags.State_Debuff_Stun = Manager.AddNativeGameplayTag(
		FName("State.Debuff.Stun"),
		FString("Stun debuff tag")
	);

	GameplayTags.State_Invincible = Manager.AddNativeGameplayTag(
		FName("State.Invincible"),
		FString("Invincibility tag")
	);

	GameplayTags.Event_Montage_AttackHit = Manager.AddNativeGameplayTag(
		FName("Event.Montage.AttackHit"),
		FString("Sent by AnimNotify at attack hit window")
	);

	GameplayTags.StateTree_Event_Stunned = Manager.AddNativeGameplayTag(
		FName("StateTree.Event.Stunned"),
		FString("Sent to StateTree when stun tag applied")
	);

	GameplayTags.StateTree_Event_StunEnded = Manager.AddNativeGameplayTag(
		FName("StateTree.Event.StunEnded"),
		FString("Sent to StateTree when stun tag removed")
	);

	GameplayTags.StateTree_Event_PlayerSpotted = Manager.AddNativeGameplayTag(
		FName("StateTree.Event.PlayerSpotted"),
		FString("Sent to StateTree when player detected by perception")
	);
}
