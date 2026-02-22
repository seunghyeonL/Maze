// Fill out your copyright notice in the Description page of Project Settings.

#include "TitleGameMode.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "OnlineSessionSettings.h"
#include "Interfaces/OnlineSessionInterface.h"

ATitleGameMode::ATitleGameMode()
{
	bUseSeamlessTravel = true;
}

void ATitleGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId,
	FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
	
	if (IOnlineSessionPtr Sessions = Online::GetSessionInterface(GetWorld()))
	{
		if (FNamedOnlineSession* Named = Sessions->GetNamedSession(NAME_GameSession))
		{
			int32 MaxPlayers = Named->SessionSettings.NumPublicConnections;
			
			if (GetNumPlayers() >= MaxPlayers)
			{
				UE_LOG(LogTemp, Log, TEXT("MazeUI: CreateSession rejected on client"));
				ErrorMessage = TEXT("Server is full");
				// OnLoginFailed.Broadcast(FText::FromString("접속 실패"), FText::FromString("인원 초과"));
			}
		}
	}
}
