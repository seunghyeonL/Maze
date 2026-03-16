#include "MazeGameInstance.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/NetDriver.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UI/UIFlowSubsystem.h"
#include "OnlineSubsystem/SOSManager.h"
#include "Settings/MazeLevelSettings.h"

void UMazeGameInstance::Init()
{
	Super::Init();

	// Bind to engine's network failure delegate
	if (GEngine)
	{
		GEngine->OnNetworkFailure().AddUObject(this, &UMazeGameInstance::OnNetworkFailure);
	}
}

void UMazeGameInstance::Shutdown()
{
	// Unbind from engine's network failure delegate
	if (GEngine)
	{
		GEngine->OnNetworkFailure().RemoveAll(this);
	}

	Super::Shutdown();
}

void UMazeGameInstance::OnNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	// Guard against recursive error handling
	if (bHandlingFailure)
	{
		return;
	}

	bHandlingFailure = true;

	// Set up 2-second timeout to reset guard flag
	GetTimerManager().SetTimer(
		FailureGuardTimerHandle,
		[this]() { bHandlingFailure = false; },
		2.0f,
		false
	);

	// Filter out host-side failures (only handle client-side)
	if (NetDriver && NetDriver->GetNetMode() != NM_Client)
	{
		return;
	}

	// Determine error message with Korean detection
	bool bHasKorean = false;
	for (const TCHAR Ch : ErrorString)
	{
		if (Ch >= 0xAC00 && Ch <= 0xD7AF)
		{
			bHasKorean = true;
			break;
		}
	}

	FText ErrorMessage;
	if (ErrorString.Contains(TEXT("full")) || ErrorString.Contains(TEXT("Server is full")))
	{
		ErrorMessage = FText::FromString(TEXT("방이 가득 찼습니다."));
	}
	else if (bHasKorean)
	{
		ErrorMessage = FText::FromString(ErrorString);
	}
	else
	{
		ErrorMessage = FText::FromString(TEXT("서버와의 연결이 끊어졌습니다."));
	}

	// Set pending error and update UI flow
	if (UUIFlowSubsystem* UIFlow = GetSubsystem<UUIFlowSubsystem>())
	{
		UIFlow->SetPendingError(ErrorMessage);
		UIFlow->SetScreenTitle();
		UIFlow->SetScreenMatch();
	}

	// Clear session state
	if (USOSManager* SOS = GetSubsystem<USOSManager>())
	{
		SOS->DestroySession();
	}

	// Travel to title level if not already there
	UWorld* CurrentWorld = GetWorld();
	if (CurrentWorld)
	{
		const UMazeLevelSettings* LevelSettings = GetDefault<UMazeLevelSettings>();
		FString CurrentMapName = CurrentWorld->GetMapName();
		CurrentMapName.RemoveFromStart(CurrentWorld->StreamingLevelsPrefix);

		FString TitleLevelName = LevelSettings->GetTitleLevelName();

		// Only travel if current map is NOT the title level
		if (CurrentMapName != TitleLevelName)
		{
			FString TitleLevelPath = LevelSettings->GetTitleLevelPath();
			UGameplayStatics::OpenLevel(this, FName(*TitleLevelPath), true);
		}
	}
}
