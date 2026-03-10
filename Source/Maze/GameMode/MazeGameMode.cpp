// Fill out your copyright notice in the Description page of Project Settings.

#include "MazeGameMode.h"
#include "PlayerController/MazePlayerController.h"
#include "GameState/MazeGameState.h"
#include "../Actor/MazeTargetPoint.h"
#include "Helper/MazeGenerator.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "EngineUtils.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Online/OnlineSessionNames.h"
#include "OnlineSessionSettings.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Settings/MazeLevelSettings.h"

AMazeGameMode::AMazeGameMode()
{
	PlayerControllerClass = AMazePlayerController::StaticClass();
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

	TryStartGameFlow();
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

	const int32 ExpectedCount = GetExpectedPlayerCount();
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
	if (!WallClass || !GoalActorClass)
	{
		UE_LOG(LogTemp, Error, TEXT("MazeGameMode: WallClass/GoalActorClass not set in BP defaults!"));
		return;
	}

	const int32 PlayerNum = ArrivedPlayers.Num();
	UE_LOG(LogTemp, Log, TEXT("MazeGameMode: GenerateMaze %dx%d Players=%d CellSize=%.0f"),
		MazeWidth, MazeHeight, PlayerNum, CellSize);
	UMazeGenerator::GenerateMaze(this, MazeWidth, MazeHeight, PlayerNum, CellSize, WallClass, GoalActorClass, BotClass, BotCount);

	// 스폰된 MazeTargetPoint 수집, PlayerIndex 기준 정렬
	MazeTargetPoints.Reset();
	for (TActorIterator<AMazeTargetPoint> It(GetWorld()); It; ++It)
	{
		MazeTargetPoints.Add(*It);
	}
	MazeTargetPoints.Sort([](const AMazeTargetPoint& A, const AMazeTargetPoint& B)
	{
		return A.PlayerIndex < B.PlayerIndex;
	});
	UE_LOG(LogTemp, Log, TEXT("MazeGameMode: Found %d MazeTargetPoints"), MazeTargetPoints.Num());

	// GameState 업데이트 (Phase: Countdown)
	if (AMazeGameState* GS = GetGameState<AMazeGameState>())
	{
		GS->SetPhase(EMazePhase::Countdown);
		GS->CountdownEndTime = GetWorld()->GetTimeSeconds() + CountdownDuration;
		GS->ForceNetUpdate();
	}

	// 카운트다운 타이머
	GetWorldTimerManager().SetTimer(
		CountdownTimerHandle,
		this,
		&AMazeGameMode::TeleportPlayers,
		CountdownDuration,
		false
	);
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

void AMazeGameMode::PreLogin(const FString& Options, const FString& Address,
	const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);

	if (!ErrorMessage.IsEmpty()) return;

	if (bGameFlowStarted)
	{
		ErrorMessage = TEXT("게임이 이미 시작됐습니다.");
		UE_LOG(LogTemp, Warning, TEXT("MazeGameMode: PreLogin rejected (game already started)"));
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
	UE_LOG(LogTemp, Log, TEXT("MazeGameMode: Returning to TitleLevel (seamless)..."));
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

int32 AMazeGameMode::GetExpectedPlayerCount() const
{
	if (const UWorld* World = GetWorld())
	{
		IOnlineSessionPtr Sessions = Online::GetSessionInterface(World);
		if (Sessions.IsValid())
		{
			if (const FNamedOnlineSession* NamedSession = Sessions->GetNamedSession(NAME_GameSession))
			{
				// GameStart 시점에 저장한 실제 인원수 우선 사용
				int32 Expected = 0;
				if (NamedSession->SessionSettings.Get(FName(TEXT("ExpectedPlayers")), Expected) && Expected > 0)
				{
					UE_LOG(LogTemp, Log, TEXT("MazeGameMode: GetExpectedPlayerCount from ExpectedPlayers = %d"), Expected);
					return Expected;
				}

				// fallback: 세션 최대 인원
				const int32 Count = NamedSession->SessionSettings.NumPublicConnections;
				UE_LOG(LogTemp, Log, TEXT("MazeGameMode: GetExpectedPlayerCount from NumPublicConnections = %d"), Count);
				return Count;
			}
		}
	}
	const int32 Fallback = FMath::Max(MinExpectedPlayers, NumPlayers);
	UE_LOG(LogTemp, Warning, TEXT("MazeGameMode: No session, fallback ExpectedCount=%d"), Fallback);
	return Fallback;
}
