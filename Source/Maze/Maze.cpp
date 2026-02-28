// Copyright Epic Games, Inc. All Rights Reserved.

#include "Maze.h"
#include "GAS/MazeGameplayTags.h"
#include "Modules/ModuleManager.h"

class FMazeGameModule : public FDefaultGameModuleImpl
{
public:
    virtual void StartupModule() override
    {
        FDefaultGameModuleImpl::StartupModule();
        FMazeGameplayTags::InitializeNativeTags();
    }
};

IMPLEMENT_PRIMARY_GAME_MODULE(FMazeGameModule, Maze, "Maze");
