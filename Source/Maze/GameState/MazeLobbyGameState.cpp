#include "MazeLobbyGameState.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

static bool IsValidMazeSize(int32 Size)
{
	return Size == 5 || Size == 7 || Size == 9 || Size == 11;
}

void AMazeLobbyGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMazeLobbyGameState, SelectedMazeSize);
	DOREPLIFETIME(AMazeLobbyGameState, bGameStarted);
}

void AMazeLobbyGameState::OnRep_SelectedMazeSize()
{
	OnMazeSizeChanged.Broadcast(SelectedMazeSize);
}

void AMazeLobbyGameState::OnRep_bGameStarted()
{
	OnGameStarted.Broadcast();
}

void AMazeLobbyGameState::SetSelectedMazeSize(int32 NewSize)
{
	if (!IsValidMazeSize(NewSize)) return;
	if (SelectedMazeSize == NewSize) return;
	SelectedMazeSize = NewSize;
	if (HasAuthority())
	{
		OnRep_SelectedMazeSize();
	}
}

void AMazeLobbyGameState::SetGameStarted(bool bStarted)
{
	if (bGameStarted == bStarted) return;
	bGameStarted = bStarted;
	if (HasAuthority())
	{
		OnRep_bGameStarted();
	}
}

void AMazeLobbyGameState::AddPlayerState(APlayerState* PlayerState)
{
	Super::AddPlayerState(PlayerState);

	// PlayerName 등 프로퍼티가 리플리케이트될 시간을 확보한 뒤 Broadcast
	TWeakObjectPtr<AMazeLobbyGameState> WeakThis(this);
	GetWorldTimerManager().SetTimer(PlayerListChangedTimerHandle, [WeakThis]()
	{
		if (WeakThis.IsValid())
		{
			WeakThis->OnPlayerListChanged.Broadcast();
		}
	}, 0.2f, false);
}

void AMazeLobbyGameState::RemovePlayerState(APlayerState* PlayerState)
{
	Super::RemovePlayerState(PlayerState);
	OnPlayerListChanged.Broadcast();
}
