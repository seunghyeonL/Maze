// Fill out your copyright notice in the Description page of Project Settings.

#include "TitleGameMode.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "OnlineSessionSettings.h"
#include "Interfaces/OnlineSessionInterface.h"

ATitleGameMode::ATitleGameMode()
{
}

void ATitleGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId,
	FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
	if (!ErrorMessage.IsEmpty()) return;
	
	if (IOnlineSessionPtr Sessions = Online::GetSessionInterface(GetWorld()))
	{
		if (FNamedOnlineSession* Named = Sessions->GetNamedSession(NAME_GameSession))
		{
			const int32 MaxPlayers = Named->SessionSettings.NumPublicConnections;
			const int32 Current = GetNumPlayers();
			
			if (Current + PendingJoinCount >= MaxPlayers)
			{
				UE_LOG(LogTemp, Log, TEXT("MazeUI: CreateSession rejected on client"));
				ErrorMessage = TEXT("Server is full");
				return;
			}
			
			PendingJoinCount++;
		}
	}
}

void ATitleGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	
	PendingJoinCount = FMath::Max(PendingJoinCount - 1, 0);
}

void ATitleGameMode::NotifyPendingConnectionLost(const FUniqueNetIdRepl& ConnectionUniqueId)
{
	// Super::NotifyPendingConnectionLost(ConnectionUniqueId); - Empty Function
	PendingJoinCount = FMath::Max(PendingJoinCount - 1, 0);
}

