// Fill out your copyright notice in the Description page of Project Settings.

#include "TitleGameMode.h"
#include "GameState/MazeLobbyGameState.h"
#include "GameSession/MazeGameSession.h"

ATitleGameMode::ATitleGameMode()
{
	GameStateClass = AMazeLobbyGameState::StaticClass();
	GameSessionClass = AMazeGameSession::StaticClass();
}

