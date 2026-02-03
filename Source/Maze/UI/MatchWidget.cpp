#include "MatchWidget.h"

#include "LobbySearchResultItem.h"
#include "UIFlowSubsystem.h"
#include "OnlineSubsystem/SOSManager.h"

#include "Components/Button.h"
#include "Components/ListView.h"

void UMatchWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetIsFocusable(true);

	CacheSubsystems();

	if (CreateLobbyButton)
	{
		CreateLobbyButton->OnClicked.RemoveDynamic(this, &UMatchWidget::HandleCreateLobbyClicked);
		CreateLobbyButton->OnClicked.AddDynamic(this, &UMatchWidget::HandleCreateLobbyClicked);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("MazeUI: CreateLobbyButton missing"));
	}

	if (FindLobbyButton)
	{
		FindLobbyButton->OnClicked.RemoveDynamic(this, &UMatchWidget::HandleFindLobbyClicked);
		FindLobbyButton->OnClicked.AddDynamic(this, &UMatchWidget::HandleFindLobbyClicked);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("MazeUI: FindLobbyButton missing"));
	}

	if (ExitToTitleButton)
	{
		ExitToTitleButton->OnClicked.RemoveDynamic(this, &UMatchWidget::HandleExitToTitleClicked);
		ExitToTitleButton->OnClicked.AddDynamic(this, &UMatchWidget::HandleExitToTitleClicked);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("MazeUI: ExitToTitleButton missing"));
	}

	if (LobbyList)
	{
		LobbyList->OnItemClicked().RemoveAll(this);
		LobbyList->OnItemClicked().AddUObject(this, &UMatchWidget::HandleLobbyItemClicked);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("MazeUI: LobbyList missing"));
	}

	if (SOSManager)
	{
		SOSManager->OnSessionCreated.RemoveDynamic(this, &UMatchWidget::HandleSessionCreated);
		SOSManager->OnSessionCreated.AddDynamic(this, &UMatchWidget::HandleSessionCreated);

		SOSManager->OnSessionsFound.RemoveDynamic(this, &UMatchWidget::HandleSessionsFound);
		SOSManager->OnSessionsFound.AddDynamic(this, &UMatchWidget::HandleSessionsFound);

		SOSManager->OnSessionJoined.RemoveDynamic(this, &UMatchWidget::HandleSessionJoined);
		SOSManager->OnSessionJoined.AddDynamic(this, &UMatchWidget::HandleSessionJoined);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("MazeUI: SOSManager missing"));
	}

	SetKeyboardFocus();
}

void UMatchWidget::NativeDestruct()
{
	if (CreateLobbyButton)
	{
		CreateLobbyButton->OnClicked.RemoveDynamic(this, &UMatchWidget::HandleCreateLobbyClicked);
	}

	if (FindLobbyButton)
	{
		FindLobbyButton->OnClicked.RemoveDynamic(this, &UMatchWidget::HandleFindLobbyClicked);
	}

	if (ExitToTitleButton)
	{
		ExitToTitleButton->OnClicked.RemoveDynamic(this, &UMatchWidget::HandleExitToTitleClicked);
	}

	if (LobbyList)
	{
		LobbyList->OnItemClicked().RemoveAll(this);
	}

	if (SOSManager)
	{
		SOSManager->OnSessionCreated.RemoveDynamic(this, &UMatchWidget::HandleSessionCreated);
		SOSManager->OnSessionsFound.RemoveDynamic(this, &UMatchWidget::HandleSessionsFound);
		SOSManager->OnSessionJoined.RemoveDynamic(this, &UMatchWidget::HandleSessionJoined);
	}

	Super::NativeDestruct();
}

void UMatchWidget::CacheSubsystems()
{
	if (GetGameInstance())
	{
		SOSManager = GetGameInstance()->GetSubsystem<USOSManager>();
		UIFlowSubsystem = GetGameInstance()->GetSubsystem<UUIFlowSubsystem>();
	}
}

void UMatchWidget::HandleCreateLobbyClicked()
{
	UE_LOG(LogTemp, Log, TEXT("MazeUI: Match CreateSession clicked"));
	if (SOSManager)
	{
		SOSManager->CreateSession(4, TEXT("/Game/Levels/TitleLevel"), true);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("MazeUI: SOSManager missing on CreateSession"));
	}
}

void UMatchWidget::HandleFindLobbyClicked()
{
	UE_LOG(LogTemp, Log, TEXT("MazeUI: Match FindSessions clicked"));
	if (SOSManager)
	{
		SOSManager->FindSessions(50, true);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("MazeUI: SOSManager missing on FindSessions"));
	}
}

void UMatchWidget::HandleExitToTitleClicked()
{
	UE_LOG(LogTemp, Log, TEXT("MazeUI: Match ExitToTitle clicked"));
	if (UIFlowSubsystem)
	{
		UIFlowSubsystem->SetScreenTitle();
	}
	RemoveFromParent();
}

void UMatchWidget::HandleLobbyItemClicked(UObject* Item)
{
	const ULobbySearchResultItem* ResultItem = Cast<ULobbySearchResultItem>(Item);
	if (!ResultItem)
	{
		UE_LOG(LogTemp, Warning, TEXT("MazeUI: Session item cast failed"));
		return;
	}

	if (SOSManager)
	{
		UE_LOG(LogTemp, Log, TEXT("MazeUI: Join session index %d"), ResultItem->SessionInfo.Index);
		SOSManager->JoinSessionByIndex(ResultItem->SessionInfo.Index);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("MazeUI: SOSManager missing on JoinSession"));
	}
}

void UMatchWidget::HandleSessionCreated(bool bSuccess)
{
	UE_LOG(LogTemp, Log, TEXT("MazeUI: Session created %s"), bSuccess ? TEXT("Success") : TEXT("Failure"));
	if (bSuccess && UIFlowSubsystem)
	{
		UIFlowSubsystem->SetScreenLobby(true);
		RemoveFromParent();
	}
}

void UMatchWidget::HandleSessionsFound(bool bSuccess, const TArray<FFoundSessionInfo>& Results)
{
	UE_LOG(LogTemp, Log, TEXT("MazeUI: Sessions found %s (%d)"), bSuccess ? TEXT("Success") : TEXT("Failure"), Results.Num());

	if (!LobbyList)
	{
		UE_LOG(LogTemp, Warning, TEXT("MazeUI: LobbyList missing on results"));
		return;
	}

	LobbyList->ClearListItems();
	LobbyItems.Reset();

	if (!bSuccess)
	{
		return;
	}

	for (const FFoundSessionInfo& Result : Results)
	{
		ULobbySearchResultItem* NewItem = NewObject<ULobbySearchResultItem>(this);
		NewItem->Init(Result);
		LobbyItems.Add(NewItem);
		LobbyList->AddItem(NewItem);
	}
}

void UMatchWidget::HandleSessionJoined(bool bSuccess)
{
	UE_LOG(LogTemp, Log, TEXT("MazeUI: Session joined %s"), bSuccess ? TEXT("Success") : TEXT("Failure"));
	if (bSuccess && UIFlowSubsystem)
	{
		UIFlowSubsystem->SetScreenLobby(false);
		RemoveFromParent();
	}
}
