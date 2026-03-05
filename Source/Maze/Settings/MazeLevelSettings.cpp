#include "MazeLevelSettings.h"

UMazeLevelSettings::UMazeLevelSettings()
{
    TitleLevel = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/Levels/TitleLevel.TitleLevel")));
    MazeLevel  = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/Levels/MazeLevel.MazeLevel")));
}
