#include "MazeLobbyPlayerState.h"

#include "Net/UnrealNetwork.h"

AMazeLobbyPlayerState::AMazeLobbyPlayerState()
{
	bReplicates = true;
}

static bool IsValidMazeSize(int32 Size)
{
	return Size == 5 || Size == 7 || Size == 9 || Size == 11;
}

void AMazeLobbyPlayerState::RequestSetReady(bool bNewReady)
{
	if (HasAuthority())
	{
		if (bIsReady != bNewReady)
		{
			bIsReady = bNewReady;
			OnRep_IsReady();
		}
		return;
	}

	ServerSetReady(bNewReady);
}

void AMazeLobbyPlayerState::RequestSetMazeSize(int32 NewSize)
{
	if (!IsValidMazeSize(NewSize))
	{
		return;
	}
	if (HasAuthority())
	{
		if (SelectedMazeSize != NewSize)
		{
			SelectedMazeSize = NewSize;
			OnRep_SelectedMazeSize();
		}
		return;
	}
	
	ServerSetMazeSize(NewSize);
}

void AMazeLobbyPlayerState::ServerSetReady_Implementation(bool bNewReady)
{
	if (bIsReady != bNewReady)
	{
		bIsReady = bNewReady;
		OnRep_IsReady();
	}
}

void AMazeLobbyPlayerState::ServerSetMazeSize_Implementation(int32 NewSize)
{
	if (!IsValidMazeSize(NewSize))
	{
		return;
	}
	if (SelectedMazeSize != NewSize)
	{
		SelectedMazeSize = NewSize;
		OnRep_SelectedMazeSize();
	}
}

void AMazeLobbyPlayerState::OnRep_IsReady()
{
	OnReadyChanged.Broadcast(this, bIsReady);
}

void AMazeLobbyPlayerState::OnRep_SelectedMazeSize()
{
	OnMazeSizeChanged.Broadcast(this, SelectedMazeSize);
}

void AMazeLobbyPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMazeLobbyPlayerState, bIsReady);
	DOREPLIFETIME(AMazeLobbyPlayerState, SelectedMazeSize);
}
