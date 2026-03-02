#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameplayTagsManager.h"

/**
 * Native GameplayTag definitions for the Maze GAS attack+stun system.
 * Tags are registered at startup via InitializeNativeTags().
 */
struct MAZE_API FMazeGameplayTags
{
	/**
	 * Get the singleton instance of Maze gameplay tags.
	 */
	static const FMazeGameplayTags& Get()
	{
		return GameplayTags;
	}

	/**
	 * Initialize all native gameplay tags.
	 * Called once at engine startup to register tags with UGameplayTagsManager.
	 */
	static void InitializeNativeTags();

	// Ability tags
	FGameplayTag Ability_Attack;

	// State tags
	FGameplayTag State_Debuff_Stun;
	FGameplayTag State_Invincible;

	// Event tags
	FGameplayTag Event_Montage_AttackHit;
	
	// Gameplay Cue Tags
	FGameplayTag GameplayCue_Actor_ElectricTrail;

private:
	static FMazeGameplayTags GameplayTags;
};
