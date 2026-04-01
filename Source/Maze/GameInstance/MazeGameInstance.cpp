#include "MazeGameInstance.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/NetDriver.h"
#include "TimerManager.h"
#include "UI/UIFlowSubsystem.h"
#include "OnlineSubsystem/SOSManager.h"
#include "OnlineSubsystemUtils.h"
#include "Online/OnlineSessionNames.h"

void UMazeGameInstance::Init()
{
	Super::Init();

	if (GEngine)
	{
		GEngine->OnNetworkFailure().AddUObject(this, &UMazeGameInstance::OnNetworkFailure);
	}
}

void UMazeGameInstance::Shutdown()
{
	if (GEngine)
	{
		GEngine->OnNetworkFailure().RemoveAll(this);
	}

	Super::Shutdown();
}

void UMazeGameInstance::OnNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	UE_LOG(LogTemp, Warning, TEXT("MazeGI: OnNetworkFailure - Type: %d, NetDriver: %s, Error: %s"),
		static_cast<int32>(FailureType),
		NetDriver ? *NetDriver->GetName() : TEXT("nullptr"),
		*ErrorString);

	// === Filters ===

	if (!World)
	{
		World = GetWorld();
	}
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("MazeGI: FILTERED - no World"));
		return;
	}

	const ENetMode NetMode = World->GetNetMode();

	// Host/server errors are handled by their own departure logic
	if (NetMode == NM_ListenServer || NetMode == NM_DedicatedServer)
	{
		UE_LOG(LogTemp, Warning, TEXT("MazeGI: FILTERED - host/server"));
		return;
	}

	// Standalone without an active session is startup noise (PendingNetDriver)
	if (NetMode == NM_Standalone)
	{
		IOnlineSessionPtr Sessions = Online::GetSessionInterface(World);
		const bool bHasGameSession = Sessions.IsValid() && (Sessions->GetNamedSession(NAME_GameSession) != nullptr);
		if (!bHasGameSession)
		{
			UE_LOG(LogTemp, Warning, TEXT("MazeGI: FILTERED - standalone no session"));
			return;
		}
	}

	// === Duplicate call guard (2 seconds) ===

	if (bHandlingFailure)
	{
		UE_LOG(LogTemp, Warning, TEXT("MazeGI: FILTERED - bHandlingFailure guard"));
		return;
	}

	bHandlingFailure = true;

	GetTimerManager().SetTimer(
		FailureGuardTimerHandle,
		[this]() { bHandlingFailure = false; },
		2.0f,
		false
	);

	// === Error message (FailureType-based, no ErrorString dependency for simplicity) ===

	FText ErrorMessage;
	if (ErrorString.Contains(TEXT("full")) || ErrorString.Contains(TEXT("Server is full")))
	{
		ErrorMessage = FText::FromString(TEXT("The room is full."));
	}
	else if (FailureType == ENetworkFailure::FailureReceived)
	{
		ErrorMessage = FText::FromString(TEXT("Connection was refused by the server."));
	}
	else
	{
		ErrorMessage = FText::FromString(TEXT("Connection to the server has been lost."));
	}

	// === UI state reset ===

	if (UUIFlowSubsystem* UIFlow = GetSubsystem<UUIFlowSubsystem>())
	{
		UIFlow->SetPendingError(ErrorMessage);
		UIFlow->SetScreenTitle();
		UIFlow->SetScreenMatch();
	}

	// === Session state cleanup ===
	// ForceReset clears in-flight handles/pending flags,
	// then DestroySession handles the actual OSS session teardown.

	if (USOSManager* SOS = GetSubsystem<USOSManager>())
	{
		SOS->ForceReset();
		SOS->DestroySession();
	}

	// Travel is NOT performed here.
	// The engine's CallHandleDisconnectForFailure (called in UEngine::HandleNetworkFailure,
	// which is a separate listener on the same OnNetworkFailure delegate, registered before ours)
	// handles it via SetClientTravel("?closed") → Browse() → GameDefaultMap (TitleLevel).
	// The ?closed path also performs network cleanup: PendingNetGame cancel, Loader reset, GC.
}
