#include "UIFlowSubsystem.h"

void UUIFlowSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Screen = EUIFlowScreen::Title;
	bLobbyHost = false;
	PendingErrorMessage = FText::GetEmpty();
}

void UUIFlowSubsystem::SetScreenTitle()
{
	Screen = EUIFlowScreen::Title;
	bLobbyHost = false;
	OnScreenChanged.Broadcast(Screen);
}

void UUIFlowSubsystem::SetScreenMatch()
{
	Screen = EUIFlowScreen::Match;
	bLobbyHost = false;
	OnScreenChanged.Broadcast(Screen);
}

void UUIFlowSubsystem::SetScreenLobby(bool bHost)
{
	Screen = EUIFlowScreen::Lobby;
	bLobbyHost = bHost;
	OnScreenChanged.Broadcast(Screen);
}

void UUIFlowSubsystem::SetPendingError(const FText& ErrorMessage)
{
	PendingErrorMessage = ErrorMessage;
}

FText UUIFlowSubsystem::ConsumePendingError()
{
	FText Result = PendingErrorMessage;
	PendingErrorMessage = FText::GetEmpty();
	return Result;
}
