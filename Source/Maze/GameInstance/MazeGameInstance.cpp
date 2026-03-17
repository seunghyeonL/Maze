#include "MazeGameInstance.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/NetDriver.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UI/UIFlowSubsystem.h"
#include "OnlineSubsystem/SOSManager.h"
#include "OnlineSubsystemUtils.h"
#include "Online/OnlineSessionNames.h"
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
	UE_LOG(LogTemp, Warning, TEXT("MazeGI: OnNetworkFailure - Type: %d, NetDriver: %s, Error: %s"),
		static_cast<int32>(FailureType),
		NetDriver ? *NetDriver->GetName() : TEXT("nullptr"),
		*ErrorString);

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

	// 호스트/서버는 무시 (자체 퇴장 로직으로 처리)
	if (!World)
	{
		return;
	}

	const ENetMode NetMode = World->GetNetMode();
	if (NetMode == NM_ListenServer || NetMode == NM_DedicatedServer)
	{
		return;
	}

	// Standalone: 활성 세션이 있을 때만 처리 (JoinSession 후 거부된 경우)
	// 세션 없으면 시작 시 PendingNetDriver 노이즈이므로 무시
	if (NetMode == NM_Standalone)
	{
		IOnlineSessionPtr Sessions = Online::GetSessionInterface(World);
		if (!Sessions.IsValid() || !Sessions->GetNamedSession(NAME_GameSession))
		{
			return;
		}
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
