#include "MazeLobbyPlayerState.h"

#include "Net/UnrealNetwork.h"

AMazeLobbyPlayerState::AMazeLobbyPlayerState()
{
	bReplicates = true;
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



void AMazeLobbyPlayerState::ServerSetReady_Implementation(bool bNewReady)
{
	if (bIsReady != bNewReady)
	{
		bIsReady = bNewReady;
		OnRep_IsReady();
	}
}



void AMazeLobbyPlayerState::OnRep_IsReady()
{
	OnReadyChanged.Broadcast(this, bIsReady);
}



void AMazeLobbyPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMazeLobbyPlayerState, bIsReady);
}
