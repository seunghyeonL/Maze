#include "MazeGameSession.h"
#include "GameFramework/GameMode.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"
#include "Kismet/GameplayStatics.h"

FString AMazeGameSession::ApproveLogin(const FString& Options)
{
	// 1. 부모 클래스 검증 (기본 에러 체크)
	FString Result = Super::ApproveLogin(Options);
	if (!Result.IsEmpty())
	{
		return Result;
	}

	// 2. 게임 시작 여부 확인
	if (bMatchStarted)
	{
		UE_LOG(LogTemp, Warning, TEXT("MazeGameSession: ApproveLogin rejected (match already started)"));
		return TEXT("게임이 이미 시작됐습니다.");
	}

	// 3. 서버 풀 상태 확인
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("MazeGameSession: ApproveLogin - no World"));
		return TEXT("Server error: no World");
	}
	AGameMode* GameMode = Cast<AGameMode>(World->GetAuthGameMode());
	if (!GameMode)
	{
		UE_LOG(LogTemp, Warning, TEXT("MazeGameSession: ApproveLogin - no GameMode"));
		return TEXT("Server error: no GameMode");
	}

	const int32 CurrentPlayers = GameMode->GetNumPlayers();
	const int32 TotalPending = PendingJoinCount + CurrentPlayers;

	if (TotalPending >= MaxPlayers)
	{
		UE_LOG(LogTemp, Log, TEXT("MazeGameSession: ApproveLogin rejected (server full) - Current=%d, Pending=%d, Max=%d"),
			CurrentPlayers, PendingJoinCount, MaxPlayers);
		return TEXT("Server is full");
	}

	// 4. PendingJoinCount 증가 후 승인
	PendingJoinCount++;
	UE_LOG(LogTemp, Log, TEXT("MazeGameSession: ApproveLogin approved - PendingJoinCount=%d"), PendingJoinCount);
	return FString();
}

void AMazeGameSession::RegisterPlayer(APlayerController* NewPlayer, const FUniqueNetIdRepl& UniqueId, bool bWasFromInvite)
{
	Super::RegisterPlayer(NewPlayer, UniqueId, bWasFromInvite);
	
	// 접속 완료 시 PendingJoinCount 감소
	PendingJoinCount = FMath::Max(0, PendingJoinCount - 1);
	UE_LOG(LogTemp, Log, TEXT("MazeGameSession: RegisterPlayer - PendingJoinCount=%d"), PendingJoinCount);
}

void AMazeGameSession::DecPendingJoin()
{
	// 접속 실패 시 PendingJoinCount 감소
	PendingJoinCount = FMath::Max(0, PendingJoinCount - 1);
	UE_LOG(LogTemp, Log, TEXT("MazeGameSession: DecPendingJoin - PendingJoinCount=%d"), PendingJoinCount);
}

void AMazeGameSession::SetMatchStarted(bool bStarted)
{
	bMatchStarted = bStarted;
	UE_LOG(LogTemp, Log, TEXT("MazeGameSession: SetMatchStarted = %s"), bStarted ? TEXT("true") : TEXT("false"));
}

void AMazeGameSession::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();
	UE_LOG(LogTemp, Log, TEXT("MazeGameSession: HandleMatchHasStarted"));
}

void AMazeGameSession::HandleMatchHasEnded()
{
	Super::HandleMatchHasEnded();
	UE_LOG(LogTemp, Log, TEXT("MazeGameSession: HandleMatchHasEnded"));
}

void AMazeGameSession::SetExpectedPlayers(int32 Count)
{
	IOnlineSessionPtr Sessions = Online::GetSessionInterface(GetWorld());
	if (!Sessions.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("MazeGameSession: SetExpectedPlayers - no session interface"));
		return;
	}

	FNamedOnlineSession* NamedSession = Sessions->GetNamedSession(NAME_GameSession);
	if (!NamedSession)
	{
		UE_LOG(LogTemp, Warning, TEXT("MazeGameSession: SetExpectedPlayers - no named session"));
		return;
	}

	NamedSession->SessionSettings.Set(
		FName(TEXT("ExpectedPlayers")), Count,
		EOnlineDataAdvertisementType::ViaOnlineService
	);
	Sessions->UpdateSession(NAME_GameSession, NamedSession->SessionSettings, true);
	UE_LOG(LogTemp, Log, TEXT("MazeGameSession: SetExpectedPlayers = %d"), Count);
}

int32 AMazeGameSession::GetExpectedPlayerCount() const
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
					UE_LOG(LogTemp, Log, TEXT("MazeGameSession: GetExpectedPlayerCount from ExpectedPlayers = %d"), Expected);
					return Expected;
				}
			}
		}
	}

	// Fallback: 1 (최소 플레이어 수)
	const int32 Fallback = 1;
	UE_LOG(LogTemp, Warning, TEXT("MazeGameSession: No session, fallback ExpectedCount=%d"), Fallback);
	return Fallback;
}
