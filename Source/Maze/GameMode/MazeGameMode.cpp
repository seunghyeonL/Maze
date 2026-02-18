// Fill out your copyright notice in the Description page of Project Settings.

#include "MazeGameMode.h"

#include "MazePlayerController.h"
#include "Helper/MazeGenerator.h"
#include "Engine/World.h"
#include "TimerManager.h"


AMazeGameMode::AMazeGameMode()
{
	bDelayedStart = true;
	PlayerControllerClass = AMazePlayerController::StaticClass();
}

void AMazeGameMode::BeginPlay()
{
	Super::BeginPlay();
}

void AMazeGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	// 미로 생성 전에는 RestartPlayer 호출하지 않음.
	// 미로가 이미 생성된 경우(뒤늦게 접속한 플레이어)에만 즉시 스폰.
	if (bMazeGenerated)
	{
		Super::HandleStartingNewPlayer_Implementation(NewPlayer);
	}
	// else: PostLogin에서 WaitingPlayers에 추가됐으므로 TryStartMatch가 처리함
}

void AMazeGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	WaitingPlayers.AddUnique(NewPlayer);
	UE_LOG(LogTemp, Log, TEXT("MazeGameMode: Player logged in (%d/%d)"),
		WaitingPlayers.Num(), GetExpectedPlayerCount());

	TryStartMatch();
}

void AMazeGameMode::Logout(AController* Exiting)
{
	if (APlayerController* PC = Cast<APlayerController>(Exiting))
	{
		WaitingPlayers.Remove(PC);
	}
	Super::Logout(Exiting);
}

int32 AMazeGameMode::GetExpectedPlayerCount() const
{
	// 현재 접속된 플레이어 수 기준 (최소 1명)
	return FMath::Max(1, NumPlayers);
}

void AMazeGameMode::TryStartMatch()
{
	if (bMazeGenerated || bMatchEnded)
	{
		return;
	}

	const int32 Expected = GetExpectedPlayerCount();
	if (WaitingPlayers.Num() < Expected)
	{
		UE_LOG(LogTemp, Log, TEXT("MazeGameMode: Waiting for players (%d/%d)"),
			WaitingPlayers.Num(), Expected);
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("MazeGameMode: All players connected. Generating maze..."));
	GenerateAndSpawnMaze();
}

void AMazeGameMode::GenerateAndSpawnMaze()
{
	bMazeGenerated = true;

	if (!WallClass || !FloorClass || !GoalActorClass)
	{
		UE_LOG(LogTemp, Error, TEXT("MazeGameMode: WallClass/FloorClass/GoalActorClass not set in BP defaults!"));
		return;
	}

	const int32 PlayerNum = WaitingPlayers.Num();
	UMazeGenerator::GenerateMaze(this, MazeWidth, MazeHeight, PlayerNum, CellSize,
		WallClass, FloorClass, GoalActorClass);

	SpawnAllPlayers();
}

void AMazeGameMode::SpawnAllPlayers()
{
	for (APlayerController* PC : WaitingPlayers)
	{
		if (!PC)
		{
			continue;
		}

		RestartPlayer(PC);

		if (AMazePlayerController* MazePC = Cast<AMazePlayerController>(PC))
		{
			MazePC->ClientHideLoading();
		}
	}
}

void AMazeGameMode::OnGoalReached(APlayerController* Winner)
{
	if (bMatchEnded)
	{
		return;
	}

	bMatchEnded = true;

	UE_LOG(LogTemp, Log, TEXT("MazeGameMode: Goal reached by player!"));
	NotifyGameResult(Winner);

	GetWorldTimerManager().SetTimer(
		ReturnTimerHandle,
		this,
		&AMazeGameMode::ReturnToLobby,
		ReturnToLobbyDelay,
		false
	);
}

void AMazeGameMode::NotifyGameResult(APlayerController* Winner)
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AMazePlayerController* MazePC = Cast<AMazePlayerController>(It->Get());
		if (!MazePC)
		{
			continue;
		}

		const bool bIsWinner = (MazePC == Winner);
		MazePC->ClientShowGameResult(bIsWinner);
	}
}

void AMazeGameMode::ReturnToLobby()
{
	UE_LOG(LogTemp, Log, TEXT("MazeGameMode: Returning to TitleLevel (lobby)..."));
	// ServerTravel: 리슨서버 + 세션 유지, 모든 클라이언트가 함께 이동
	// UIFlowSubsystem은 GameInstance 서브시스템이라 유지됨 -> LobbyWidget 자동 표시
	// MazeLobbyPlayerState 재생성 -> bIsReady=false 자동 초기화
	GetWorld()->ServerTravel(TEXT("/Game/Levels/TitleLevel"));
}