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

void USOSManager::CreateLobby(int32 MaxPlayers, const FString& LobbyMap, bool bLAN)
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid())
	{
		OnLobbyCreated.Broadcast(false);
		return;
	}

	PendingLobbyMap = LobbyMap;
	LastFoundLobbies.Reset();

	// 혹시 남아있는 세션이 있으면 먼저 정리하고 싶다면 Destroy 후 재시도 로직을 넣는 게 안전함
	// (여기서는 단순화)

	FOnlineSessionSettings Settings;
	Settings.bIsLANMatch = bLAN;
	Settings.NumPublicConnections = FMath::Max(2, MaxPlayers);
	Settings.bShouldAdvertise = true;
	Settings.bAllowJoinInProgress = true;

	// “Steam Lobby 방식” 핵심 옵션들
	Settings.bUsesPresence = true;
	Settings.bUseLobbiesIfAvailable = true;

	// 맵 이름 같은 메타데이터도 올려두면 편함
	Settings.Set(SETTING_MAPNAME, LobbyMap, EOnlineDataAdvertisementType::ViaOnlineService);

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
		OnLobbyCreated.Broadcast(false);
	}
}

void USOSManager::HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (Sessions.IsValid())
	{
		Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);
	}

	OnLobbyCreated.Broadcast(bWasSuccessful);

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
		const FString TravelURL = FString::Printf(TEXT("%s?listen"), *PendingLobbyMap);
		World->ServerTravel(TravelURL);
	}
}

void USOSManager::FindLobbies(int32 MaxResults, bool bLAN)
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid())
	{
		OnLobbiesFound.Broadcast(false, {});
		return;
	}

	LastFoundLobbies.Reset();

	SessionSearch = MakeShared<FOnlineSessionSearch>();
	SessionSearch->MaxSearchResults = FMath::Clamp(MaxResults, 1, 500);
	SessionSearch->bIsLanQuery = bLAN;

	// Steam Lobby/Presence 기반 검색에서 자주 필요한 조건
	SessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);

	FindHandle = Sessions->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &USOSManager::HandleFindSessionsComplete)
	);

	const int32 LocalUserNum = 0;
	const bool bStarted = Sessions->FindSessions(LocalUserNum, SessionSearch.ToSharedRef());
	if (!bStarted)
	{
		Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle);
		OnLobbiesFound.Broadcast(false, {});
	}
}

void USOSManager::HandleFindSessionsComplete(bool bWasSuccessful)
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (Sessions.IsValid())
	{
		Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle);
	}

	if (!bWasSuccessful || !SessionSearch.IsValid())
	{
		OnLobbiesFound.Broadcast(false, {});
		return;
	}

	for (int32 i = 0; i < SessionSearch->SearchResults.Num(); ++i)
	{
		const FOnlineSessionSearchResult& R = SessionSearch->SearchResults[i];
		if (!R.IsValid()) continue;

		FFoundLobbyInfo Info;
		Info.Index = i;
		Info.PingMs = R.PingInMs;

		Info.HostName = R.Session.OwningUserName;
		Info.MaxPlayers = R.Session.SessionSettings.NumPublicConnections;
		Info.CurrentPlayers = Info.MaxPlayers - R.Session.NumOpenPublicConnections;

		Info.SessionId = R.GetSessionIdStr();
		LastFoundLobbies.Add(Info);
	}

	OnLobbiesFound.Broadcast(true, LastFoundLobbies);
}

void USOSManager::JoinLobbyByIndex(int32 ResultIndex)
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid() || !SessionSearch.IsValid())
	{
		OnLobbyJoined.Broadcast(false);
		return;
	}

	if (!SessionSearch->SearchResults.IsValidIndex(ResultIndex))
	{
		OnLobbyJoined.Broadcast(false);
		return;
	}

	JoinHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateUObject(this, &USOSManager::HandleJoinSessionComplete)
	);

	const int32 LocalUserNum = 0;
	const bool bStarted = Sessions->JoinSession(LocalUserNum, GAME_SESSION_NAME, SessionSearch->SearchResults[ResultIndex]);
	if (!bStarted)
	{
		Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);
		OnLobbyJoined.Broadcast(false);
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
	OnLobbyJoined.Broadcast(bSuccess);

	if (!bSuccess || !Sessions.IsValid()) return;

	FString ConnectString;
	if (!Sessions->GetResolvedConnectString(SessionName, ConnectString))
	{
		OnLobbyJoined.Broadcast(false);
		return;
	}

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		PC->ClientTravel(ConnectString, ETravelType::TRAVEL_Absolute);
	}
}

void USOSManager::DestroyLobby()
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid())
	{
		OnLobbyDestroyed.Broadcast(false);
		return;
	}

	DestroyHandle = Sessions->AddOnDestroySessionCompleteDelegate_Handle(
		FOnDestroySessionCompleteDelegate::CreateUObject(this, &USOSManager::HandleDestroySessionComplete)
	);

	const bool bStarted = Sessions->DestroySession(GAME_SESSION_NAME);
	if (!bStarted)
	{
		Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroyHandle);
		OnLobbyDestroyed.Broadcast(false);
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
	OnLobbyDestroyed.Broadcast(bWasSuccessful);
}
