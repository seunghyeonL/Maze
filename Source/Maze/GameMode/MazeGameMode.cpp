// Fill out your copyright notice in the Description page of Project Settings.

#include "MazeGameMode.h"
#include "PlayerController/MazePlayerController.h"
#include "GameState/MazeGameState.h"
#include "GameSession/MazeGameSession.h"
#include "../Actor/MazeTargetPoint.h"
#include "Helper/MazeGenerator.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Settings/MazeLevelSettings.h"
#include "OnlineSubsystemUtils.h"
#include "Online/OnlineSessionNames.h"
#include "OnlineSessionSettings.h"

AMazeGameMode::AMazeGameMode()
{
	PlayerControllerClass = AMazePlayerController::StaticClass();
	GameSessionClass = AMazeGameSession::StaticClass();
	bDelayedStart = false;
	GameStateClass = AMazeGameState::StaticClass();
}

void AMazeGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	const int32 Size = UGameplayStatics::GetIntOption(Options, TEXT("MazeSize"), 9);
	if (Size == 5 || Size == 7 || Size == 9 || Size == 11)
	{
		MazeWidth = Size;
		MazeHeight = Size;
	}

	UE_LOG(LogTemp, Log, TEXT("MazeGameMode: InitGame MazeSize=%d (Width=%d, Height=%d)"), Size, MazeWidth, MazeHeight);

}

void AMazeGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	ArrivedPlayers.AddUnique(NewPlayer);
	UE_LOG(LogTemp, Log, TEXT("MazeGameMode: Player arrived (%d so far)"), ArrivedPlayers.Num());

	if (ArrivedPlayers.Num() == 1 && !bGameFlowStarted)
	{
		GetWorldTimerManager().SetTimer(
			ArrivalTimeoutHandle,
			this,
			&AMazeGameMode::OnArrivalTimeout,
			ArrivalTimeoutDuration,
			false
		);
	}

	GetWorldTimerManager().SetTimerForNextTick(this, &AMazeGameMode::TryStartGameFlow);
}

void AMazeGameMode::Logout(AController* Exiting)
{
	// 커스텀 정리: phase-aware ArrivedPlayers 관리
	if (Exiting)
	{
		if (!bGameFlowStarted)
		{
			// 게임 시작 전: Remove 안전 (MazeTargetPoints 매핑이 아직 없음)
			ArrivedPlayers.Remove(Exiting);
		}
		else
		{
			// 게임 시작 후: null-out으로 인덱스 매핑 보존
			// ArrivedPlayers[i] ↔ MazeTargetPoints[i] 매핑이 TeleportPlayers에서 사용됨
			const int32 Index = ArrivedPlayers.Find(Exiting);
			if (Index != INDEX_NONE)
			{
				ArrivedPlayers[Index] = nullptr;
			}
		}

		// 끊긴 플레이어의 Pawn 파괴
		if (APawn* LeavingPawn = Exiting->GetPawn())
		{
			if (IsValid(LeavingPawn))
			{
				LeavingPawn->Destroy();
			}
		}
	}

	// 엔진 내부 정리 (NumPlayers, GameSession, InactivePlayerArray 등)
	Super::Logout(Exiting);
}

void AMazeGameMode::TryStartGameFlow()
{
	if (bGameFlowStarted || bMatchEnded) return;

	int32 ExpectedCount = MinExpectedPlayers;
	if (AMazeGameSession* MazeSession = Cast<AMazeGameSession>(GameSession))
	{
		ExpectedCount = MazeSession->GetExpectedPlayerCount();
	}
	
	if (ArrivedPlayers.Num() < ExpectedCount)
	{
		UE_LOG(LogTemp, Log, TEXT("MazeGameMode: Waiting for players (%d/%d)"),
			ArrivedPlayers.Num(), ExpectedCount);
		return;
	}

	bGameFlowStarted = true;
	GetWorldTimerManager().ClearTimer(ArrivalTimeoutHandle);
	UE_LOG(LogTemp, Log, TEXT("MazeGameMode: All players arrived. Generating maze..."));
	GenerateAndSpawnMaze();
}

void AMazeGameMode::GenerateAndSpawnMaze()
{
	AMazeGameState* GS = GetGameState<AMazeGameState>();
	if (!GS)
	{
		UE_LOG(LogTemp, Error, TEXT("MazeGameMode: GameState is null"));
		return;
	}
	
	if (!GS->WallClass || !GoalActorClass)
	{
		UE_LOG(LogTemp, Error, TEXT("MazeGameMode: WallClass/GoalActorClass not set in defaults!"));
		return;
	}
	
	TRACE_BOOKMARK(TEXT("MazeGameMode: GenerateAndSpawnMaze"));
	
	// 1. 시드 생성 (비제로 보장)
	const int32 Seed = FMath::Rand() | 1;
	
	CachedPlayerNum = ArrivedPlayers.Num();
	CachedCellSize = GS->CellSize;
	UE_LOG(LogTemp, Log, TEXT("MazeGameMode: GenerateMaze %dx%d Players=%d CellSize=%.0f Seed=%d"),
		MazeWidth, MazeHeight, CachedPlayerNum, CachedCellSize, Seed);
	
	// 2. 그리드 생성 (결정론적)
	CachedGrid.Reset();
	CachedGrid.SetNum(MazeHeight);
	for (auto& Row : CachedGrid)
	{
		Row.Cells.SetNum(MazeWidth);
	}
	UMazeGenerator::BuildMazeGrid(MazeHeight, MazeWidth, Seed, CachedGrid);
	
	// 3. 클라이언트에 시드 전달 → OnRep_MazeSeed에서 클라이언트 벽 스폰 시작
	GS->SetMazeData(Seed, MazeWidth, MazeHeight);
	
	// 4. 서버: 지연 벽 스폰 → 완료 후 OnWallSpawnComplete 콜백
	UMazeGenerator::SpawnWallsWithDelay(
		this,
		CachedGrid,
		MazeHeight, MazeWidth,
		CachedCellSize,
		GS->WallClass,
		GS->WallSpawnInterval,
		FSimpleDelegate::CreateUObject(this, &AMazeGameMode::OnWallSpawnComplete));
}

void AMazeGameMode::OnWallSpawnComplete()
{
	AMazeGameState* GS = GetGameState<AMazeGameState>();
	if (!GS) return;

	UE_LOG(LogTemp, Log, TEXT("MazeGameMode: Wall spawn complete. Spawning gameplay actors..."));

	// 게임플레이 액터 스폰 (서버 리플리케이트 — 골/봇/TargetPoint)
	UMazeGenerator::SpawnGameplayActors(
		this, CachedGrid,
		MazeHeight, MazeWidth,
		CachedCellSize,
		CachedPlayerNum,
		GoalActorClass, BotClass, BotCount);

	// Grid 메모리 해제
	CachedGrid.Reset();

	// MazeTargetPoint 수집
	MazeTargetPoints.Reset();
	for (TActorIterator<AMazeTargetPoint> It(GetWorld()); It; ++It)
		MazeTargetPoints.Add(*It);
	MazeTargetPoints.Sort([](const AMazeTargetPoint& A, const AMazeTargetPoint& B)
		{ return A.PlayerIndex < B.PlayerIndex; });
	UE_LOG(LogTemp, Log, TEXT("MazeGameMode: Found %d MazeTargetPoints"), MazeTargetPoints.Num());

	// Phase: Countdown
	GS->SetPhase(EMazePhase::Countdown);
	GS->CountdownEndTime = GetWorld()->GetTimeSeconds() + CountdownDuration;
	GS->ForceNetUpdate();

	// 게임 시작 알림
	if (GameSession) GameSession->HandleMatchHasStarted();
	if (AMazeGameSession* MazeSession = Cast<AMazeGameSession>(GameSession))
		MazeSession->SetMatchStarted(true);

	// 카운트다운 타이머
	GetWorldTimerManager().SetTimer(
		CountdownTimerHandle,
		this,
		&AMazeGameMode::TeleportPlayers,
		CountdownDuration,
		false);
}

void AMazeGameMode::TeleportPlayers()
{
	UE_LOG(LogTemp, Log, TEXT("MazeGameMode: Teleporting players to maze start positions"));

	for (int32 i = 0; i < ArrivedPlayers.Num(); ++i)
	{
		AController* Controller = ArrivedPlayers[i];
		if (!Controller) continue;

		APawn* Pawn = Controller->GetPawn();
		if (!Pawn) continue;

		if (MazeTargetPoints.IsValidIndex(i) && MazeTargetPoints[i])
		{
			const FVector TargetLoc = MazeTargetPoints[i]->GetActorLocation() + FVector(0.f, 0.f, 100.f);
			Pawn->SetActorLocation(TargetLoc, false, nullptr, ETeleportType::TeleportPhysics);
			UE_LOG(LogTemp, Log, TEXT("MazeGameMode: Teleported player[%d] to %s"), i, *TargetLoc.ToString());
		}
	}

	// GameState: Playing
	if (AMazeGameState* GS = GetGameState<AMazeGameState>())
	{
		GS->SetPhase(EMazePhase::Playing);
		GS->ForceNetUpdate();
	}
}

void AMazeGameMode::OnGoalReached(APlayerController* Winner)
{
	if (bMatchEnded) return;
	bMatchEnded = true;

	UE_LOG(LogTemp, Log, TEXT("MazeGameMode: Goal reached! Winner: %s"),
		Winner ? *Winner->GetName() : TEXT("Unknown"));

	if (AMazeGameState* GS = GetGameState<AMazeGameState>())
	{
		GS->SetWinnerPlayer(Winner ? Winner->PlayerState : nullptr);
		GS->SetPhase(EMazePhase::GameOver);
		GS->ForceNetUpdate();
	}

	// 게임 종료 알림
	if (GameSession)
	{
		GameSession->HandleMatchHasEnded();
	}

	GetWorldTimerManager().SetTimer(
		ReturnTimerHandle,
		this,
		&AMazeGameMode::ReturnToLobby,
		ReturnToLobbyDelay,
		false
	);
}

void AMazeGameMode::ReturnToLobby()
{
	UE_LOG(LogTemp, Log, TEXT("MazeGameMode: Returning to TitleLevel..."));

	// 세션 메타데이터 복원 (로비 복귀 — 중간 참가 재허용)
	if (IOnlineSessionPtr Sessions = Online::GetSessionInterface(GetWorld()))
	{
		if (FNamedOnlineSession* NamedSession = Sessions->GetNamedSession(NAME_GameSession))
		{
			NamedSession->SessionSettings.bAllowJoinInProgress = true;
			NamedSession->SessionSettings.Set(
				SETTING_MAPNAME,
				GetDefault<UMazeLevelSettings>()->GetTitleLevelPath(),
				EOnlineDataAdvertisementType::ViaOnlineService
			);
			Sessions->UpdateSession(NAME_GameSession, NamedSession->SessionSettings, true);
			UE_LOG(LogTemp, Log, TEXT("MazeGameMode: Session metadata restored - bAllowJoinInProgress=true, MAPNAME=TitleLevel"));
		}
	}

	// 다음 라운드를 위해 매치 상태 초기화
	if (AMazeGameSession* MazeSession = Cast<AMazeGameSession>(GameSession))
	{
		MazeSession->SetMatchStarted(false);
	}

	GetWorld()->ServerTravel(FString::Printf(TEXT("%s?listen"), *GetDefault<UMazeLevelSettings>()->GetTitleLevelPath()));
}

void AMazeGameMode::OnArrivalTimeout()
{
	if (bGameFlowStarted || bMatchEnded) return;

	if (ArrivedPlayers.Num() >= 1)
	{
		UE_LOG(LogTemp, Warning, TEXT("MazeGameMode: Arrival timeout! Starting with %d players"),
			ArrivedPlayers.Num());
		bGameFlowStarted = true;
		GenerateAndSpawnMaze();
	}
}


