#include "SOSManager.h"

#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

const FName USOSManager::GAME_SESSION_NAME = NAME_GameSession;

IOnlineSessionPtr USOSManager::GetSessionInterface() const
{
	// World 기반으로 가져오는 게 PIE/Standalone에서 제일 덜 삐끗함
	if (UWorld* World = GetWorld())
	{
		return Online::GetSessionInterface(World);
	}
	return nullptr;
}

void USOSManager::CreateSession(int32 MaxPlayers, const FString& SessionMap, bool bLAN)
{
	if (const UWorld* World = GetWorld())
	{
		if (World->GetNetMode() == NM_Client)
		{
			UE_LOG(LogTemp, Warning, TEXT("MazeUI: CreateSession rejected on client"));
			OnSessionCreated.Broadcast(false);
			return;
		}
	}

	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid())
	{
		OnSessionCreated.Broadcast(false);
		return;
	}

	// If a stale session exists (e.g., client was disconnected without destroying GameSession),
	// destroy it first then retry automatically.
	if (Sessions->GetNamedSession(GAME_SESSION_NAME) != nullptr)
	{
		UE_LOG(LogTemp, Log, TEXT("MazeUI: CreateSession found existing GameSession; destroying then retry"));
		bPendingCreateAfterDestroy = true;
		PendingCreateMaxPlayers = MaxPlayers;
		PendingSessionMap = SessionMap;
		bPendingCreateLAN = bLAN;
		DestroySession();
		return;
	}

	PendingSessionMap = SessionMap;
	LastFoundSessions.Reset();

	// 혹시 남아있는 세션이 있으면 먼저 정리하고 싶다면 Destroy 후 재시도 로직을 넣는 게 안전함
	// (여기서는 단순화)

	FOnlineSessionSettings Settings;
	Settings.bIsLANMatch = bLAN;
	Settings.NumPublicConnections = FMath::Max(2, MaxPlayers);
	Settings.bShouldAdvertise = true;
	Settings.bAllowJoinInProgress = true;

	// LAN(Null OSS) → Presence/Lobby OFF, Steam → Presence/Lobby ON
	Settings.bUsesPresence = !bLAN;
	Settings.bUseLobbiesIfAvailable = !bLAN;

	// 맵 이름 같은 메타데이터도 올려두면 편함
	Settings.Set(SETTING_MAPNAME, SessionMap, EOnlineDataAdvertisementType::ViaOnlineService);

	// Create 완료 델리게이트
	CreateHandle = Sessions->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &USOSManager::HandleCreateSessionComplete)
	);

	// 로컬 유저 인덱스 0 기준 (일반적인 싱글 로컬 플레이 기준)
	const int32 LocalUserNum = 0;
	bHosting = true;

	const bool bStarted = Sessions->CreateSession(LocalUserNum, GAME_SESSION_NAME, Settings);
	if (!bStarted)
	{
		Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);
		OnSessionCreated.Broadcast(false);
	}
}

void USOSManager::HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (Sessions.IsValid())
	{
		Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);
	}

	OnSessionCreated.Broadcast(bWasSuccessful);

	if (!bWasSuccessful || !GetWorld()) return;

	// NOTE: No separate LobbyLevel. Keep the host on TitleLevel and switch UI only.
	// If we are not already a listen server (e.g., standalone host), reopen the current map as listen once.
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (World->GetNetMode() == NM_Standalone)
	{
		const FString TravelURL = FString::Printf(TEXT("%s?listen"), *PendingSessionMap);
		World->ServerTravel(TravelURL);
	}
}

void USOSManager::FindSessions(int32 MaxResults, bool bLAN)
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid())
	{
		OnSessionsFound.Broadcast(false, {});
		return;
	}

	// 이미 검색 중이면 무시
	if (FindHandle.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("MazeUI: FindSessions already in progress, ignoring"));
		return;
	}

	LastFoundSessions.Reset();

	SessionSearch = MakeShared<FOnlineSessionSearch>();
	SessionSearch->MaxSearchResults = FMath::Clamp(MaxResults, 1, 500);
	SessionSearch->bIsLanQuery = bLAN;

	// Steam(bLAN=false) → Lobby 기반 검색
	if (!bLAN)
	{
		SessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
	}

	FindHandle = Sessions->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &USOSManager::HandleFindSessionsComplete)
	);

	const int32 LocalUserNum = 0;
	const bool bStarted = Sessions->FindSessions(LocalUserNum, SessionSearch.ToSharedRef());
	if (!bStarted)
	{
		Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle);
		OnSessionsFound.Broadcast(false, {});
	}
}

void USOSManager::HandleFindSessionsComplete(bool bWasSuccessful)
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (Sessions.IsValid())
	{
		Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle);
	}
	FindHandle.Reset();

	if (!bWasSuccessful || !SessionSearch.IsValid())
	{
		OnSessionsFound.Broadcast(false, {});
		return;
	}

	for (int32 i = 0; i < SessionSearch->SearchResults.Num(); ++i)
	{
		const FOnlineSessionSearchResult& R = SessionSearch->SearchResults[i];
		if (!R.IsValid()) continue;

		FFoundSessionInfo Info;
		Info.Index = i;
		Info.PingMs = R.PingInMs;

		Info.HostName = R.Session.OwningUserName;
		Info.MaxPlayers = R.Session.SessionSettings.NumPublicConnections;
		Info.CurrentPlayers = Info.MaxPlayers - R.Session.NumOpenPublicConnections;

		Info.SessionId = R.GetSessionIdStr();
		LastFoundSessions.Add(Info);
	}

	OnSessionsFound.Broadcast(true, LastFoundSessions);
}

void USOSManager::JoinSessionByIndex(int32 ResultIndex)
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid() || !SessionSearch.IsValid())
	{
		OnSessionJoined.Broadcast(false);
		return;
	}

	if (Sessions->GetNamedSession(GAME_SESSION_NAME) != nullptr)
	{
		UE_LOG(LogTemp, Log, TEXT("MazeUI: JoinSession found existing GameSession; destroying then retry"));
		bPendingJoinAfterDestroy = true;
		PendingJoinIndex = ResultIndex;
		DestroySession();
		return;
	}

	if (!SessionSearch->SearchResults.IsValidIndex(ResultIndex))
	{
		OnSessionJoined.Broadcast(false);
		return;
	}

	// UE 5.5+ workaround: Steam OSS가 검색 결과에 Presence/Lobby 플래그를 채워주지 않아
	// JoinSession 시 bUsesPresence != bUseLobbiesIfAvailable 에러 발생 방지
	FOnlineSessionSearchResult& SearchResult = SessionSearch->SearchResults[ResultIndex];
	SearchResult.Session.SessionSettings.bUsesPresence = true;
	SearchResult.Session.SessionSettings.bUseLobbiesIfAvailable = true;

	JoinHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateUObject(this, &USOSManager::HandleJoinSessionComplete)
	);

	const int32 LocalUserNum = 0;
	const bool bStarted = Sessions->JoinSession(LocalUserNum, GAME_SESSION_NAME, SearchResult);
	if (!bStarted)
	{
		Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);
		OnSessionJoined.Broadcast(false);
	}
}

void USOSManager::HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (Sessions.IsValid())
	{
		Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);
	}

	const bool bSuccess = (Result == EOnJoinSessionCompleteResult::Success);
	UE_LOG(LogTemp, Log, TEXT("MazeOSS: JoinSession result = %d"), static_cast<int32>(Result));

	if (!bSuccess || !Sessions.IsValid())
	{
		OnSessionJoined.Broadcast(false);
		return;
	}

	FString ConnectString;
	if (!Sessions->GetResolvedConnectString(SessionName, ConnectString))
	{
		UE_LOG(LogTemp, Error, TEXT("MazeOSS: GetResolvedConnectString FAILED"));
		OnSessionJoined.Broadcast(false);
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("MazeOSS: ConnectString = %s"), *ConnectString);

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		UE_LOG(LogTemp, Log, TEXT("MazeOSS: ClientTravel to %s"), *ConnectString);
		PC->ClientTravel(ConnectString, ETravelType::TRAVEL_Absolute);
		OnSessionJoined.Broadcast(true);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("MazeOSS: PlayerController not found"));
		OnSessionJoined.Broadcast(false);
	}
}

void USOSManager::DestroySession()
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid())
	{
		OnSessionDestroyed.Broadcast(false);
		return;
	}

	DestroyHandle = Sessions->AddOnDestroySessionCompleteDelegate_Handle(
		FOnDestroySessionCompleteDelegate::CreateUObject(this, &USOSManager::HandleDestroySessionComplete)
	);

	const bool bStarted = Sessions->DestroySession(GAME_SESSION_NAME);
	if (!bStarted)
	{
		Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroyHandle);
		OnSessionDestroyed.Broadcast(false);
	}
}

void USOSManager::HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (Sessions.IsValid())
	{
		Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroyHandle);
	}

	bHosting = false;
	OnSessionDestroyed.Broadcast(bWasSuccessful);

	if (!bWasSuccessful)
	{
		bPendingCreateAfterDestroy = false;
		bPendingJoinAfterDestroy = false;
		PendingJoinIndex = -1;
		PendingCreateMaxPlayers = 0;
		bPendingCreateLAN = false;
		return;
	}

	if (bPendingCreateAfterDestroy)
	{
		bPendingCreateAfterDestroy = false;
		const int32 MaxPlayers = PendingCreateMaxPlayers;
		const FString Map = PendingSessionMap;
		const bool bLAN = bPendingCreateLAN;
		PendingCreateMaxPlayers = 0;
		bPendingCreateLAN = false;
		UE_LOG(LogTemp, Log, TEXT("MazeUI: Retrying CreateSession after destroy"));
		CreateSession(MaxPlayers, Map, bLAN);
		return;
	}

	if (bPendingJoinAfterDestroy)
	{
		bPendingJoinAfterDestroy = false;
		const int32 JoinIndex = PendingJoinIndex;
		PendingJoinIndex = -1;
		UE_LOG(LogTemp, Log, TEXT("MazeUI: Retrying JoinSession after destroy"));
		JoinSessionByIndex(JoinIndex);
		return;
	}
}
