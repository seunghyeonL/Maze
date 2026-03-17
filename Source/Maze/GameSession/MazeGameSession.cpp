#include "MazeGameSession.h"
#include "GameFramework/GameMode.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"

void AMazeGameSession::InitOptions(const FString& Options)
{
	Super::InitOptions(Options);

	// OnlineSession의 NumPublicConnections → MaxPlayers 동기화
	// Listen Server에서는 호스트가 NumPublicConnections에 포함되지 않으므로 +1 보정
	if (UWorld* World = GetWorld())
	{
		IOnlineSessionPtr Sessions = Online::GetSessionInterface(World);
		if (Sessions.IsValid())
		{
			if (const FNamedOnlineSession* Named = Sessions->GetNamedSession(NAME_GameSession))
			{
				MaxPlayers = Named->SessionSettings.NumPublicConnections;
				UE_LOG(LogTemp, Log, TEXT("MazeGameSession: InitOptions - MaxPlayers=%d (NumPublicConnections=%d)"),
					MaxPlayers, Named->SessionSettings.NumPublicConnections);
			}
		}
	}
}

FString AMazeGameSession::ApproveLogin(const FString& Options)
{
	// 1. 부모 클래스 검증 (AtCapacity 포함 — MaxPlayers 기반 정원 체크)
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

	return FString();
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
				int32 Expected = 0;
				if (NamedSession->SessionSettings.Get(FName(TEXT("ExpectedPlayers")), Expected) && Expected > 0)
				{
					return Expected;
				}
			}
		}
	}

	return 1;
}
