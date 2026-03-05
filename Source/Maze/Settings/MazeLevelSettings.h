#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "MazeLevelSettings.generated.h"

/**
 * Centralized level path settings for the Maze project.
 * Editable from Project Settings → Game → Maze.
 */
UCLASS(config=Game, defaultconfig, meta=(DisplayName="Maze"))
class MAZE_API UMazeLevelSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UMazeLevelSettings();

    virtual FName GetCategoryName() const override { return FName(TEXT("Game")); }

    // ---- Map Paths ----
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category="Maps",
        meta=(AllowedClasses="/Script/Engine.World"))
    TSoftObjectPtr<UWorld> TitleLevel;

    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category="Maps",
        meta=(AllowedClasses="/Script/Engine.World"))
    TSoftObjectPtr<UWorld> MazeLevel;

    // ---- Helpers ----
    /** Returns the package path for use in ServerTravel / ClientTravel.
     *  Example: /Game/Levels/TitleLevel
     *  NEVER use .ToString() — it appends ".AssetName" which breaks travel. */
    FString GetTitleLevelPath() const
    {
        return TitleLevel.ToSoftObjectPath().GetLongPackageName();
    }

    FString GetMazeLevelPath() const
    {
        return MazeLevel.ToSoftObjectPath().GetLongPackageName();
    }

    /** Returns just the asset name for level name comparisons.
     *  Example: "TitleLevel" or "MazeLevel" */
    FString GetTitleLevelName() const
    {
        return TitleLevel.ToSoftObjectPath().GetAssetName();
    }

    FString GetMazeLevelName() const
    {
        return MazeLevel.ToSoftObjectPath().GetAssetName();
    }
};
