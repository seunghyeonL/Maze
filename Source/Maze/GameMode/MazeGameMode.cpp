// Fill out your copyright notice in the Description page of Project Settings.

#include "MazeGameMode.h"

#include "MazePlayerController.h"
#include "Helper/MazeGenerator.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "GameFramework/PlayerStart.h"
#include "EngineUtils.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Online/OnlineSessionNames.h"
#include "OnlineSessionSettings.h"
#include "Interfaces/OnlineSessionInterface.h"

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

		// 뒤늦게 접속한 플레이어의 로딩 오버레이 해제
		if (AMazePlayerController* MazePC = Cast<AMazePlayerController>(NewPlayer))
		{
			MazePC->ClientHideLoading();
		}
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
	// 세션의 MaxPlayers를 기준으로 대기 (가장 정확)
	if (const UWorld* World = GetWorld())
	{
		IOnlineSessionPtr Sessions = Online::GetSessionInterface(World);
		if (Sessions.IsValid())
		{
			if (const FNamedOnlineSession* NamedSession = Sessions->GetNamedSession(NAME_GameSession))
			{
				const int32 Count = NamedSession->SessionSettings.NumPublicConnections;
				UE_LOG(LogTemp, Log, TEXT("MazeGameMode: GetExpectedPlayerCount from session = %d"), Count);
				return Count;
			}
		}
	}
	// Fallback: 현재 접속된 플레이어 수 (세션 정보 없을 때)
	UE_LOG(LogTemp, Warning, TEXT("MazeGameMode: No session found! Fallback ExpectedPlayerCount = %d"), FMath::Max(1, NumPlayers));
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
	if (!WallClass || !GoalActorClass)
	{
		UE_LOG(LogTemp, Error, TEXT("MazeGameMode: WallClass/GoalActorClass not set in BP defaults!"));
		return;
	}

	bMazeGenerated = true;

	// 레벨에 미리 배치된 PlayerStart가 있으면 ChoosePlayerStart가 그쪽을 선택할 수 있음 → 전부 제거
	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		It->Destroy();
	}

	const int32 PlayerNum = WaitingPlayers.Num();
	UE_LOG(LogTemp, Log, TEXT("MazeGameMode: GenerateMaze Width=%d Height=%d Players=%d CellSize=%.0f"),
		MazeWidth, MazeHeight, PlayerNum, CellSize);
	UMazeGenerator::GenerateMaze(this, MazeWidth, MazeHeight, PlayerNum, CellSize,
		WallClass, GoalActorClass);

	// StartMatch → match state를 InProgress로 전환
	// AGameMode::RestartPlayer는 IsMatchInProgress()==true일 때만 동작함
	// Super::HandleMatchHasStarted가 RestartPlayer를 호출하므로, SpawnAllPlayers에서는 이중 스폰 방지
	StartMatch();

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

		// Super::HandleMatchHasStarted가 이미 RestartPlayer를 호출했을 수 있음 → Pawn 체크
		if (!PC->GetPawn())
		{
			RestartPlayer(PC);
		}

		UE_LOG(LogTemp, Log, TEXT("MazeGameMode: SpawnAllPlayers - %s Pawn=%s"),
			*PC->GetName(), PC->GetPawn() ? *PC->GetPawn()->GetName() : TEXT("NULL"));

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