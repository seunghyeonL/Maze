// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Maze : ModuleRules
{
	public Maze(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateIncludePaths.AddRange(new string[] { "Maze" });
		// PublicIncludePaths.AddRange(new string[] { "Maze" });
	
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput",
			"UMG",
			"Niagara", "NiagaraCore",
			"OnlineSubsystem", "OnlineSubsystemUtils",
			"GameplayAbilities", "GameplayTags", "GameplayTasks",
			"AIModule", "NavigationSystem", "StateTreeModule", "GameplayStateTreeModule"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore", "DeveloperSettings" });
		
		DynamicallyLoadedModuleNames.Add("OnlineSubsystemSteam");

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
