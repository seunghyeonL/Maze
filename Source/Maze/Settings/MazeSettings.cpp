#include "MazeSettings.h"

UMazeSettings::UMazeSettings()
{
    TitleLevel = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/Levels/TitleLevel.TitleLevel")));
    MazeLevel  = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/Levels/MazeLevel.MazeLevel")));
}

FText UMazeSettings::GetSectionText() const
{
    return NSLOCTEXT("MazeSettings", "SectionName", "Maze");
}
