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
		SOSManager->OnLobbyCreated.RemoveDynamic(this, &UMatchWidget::HandleLobbyCreated);
		SOSManager->OnLobbyCreated.AddDynamic(this, &UMatchWidget::HandleLobbyCreated);

		SOSManager->OnLobbiesFound.RemoveDynamic(this, &UMatchWidget::HandleLobbiesFound);
		SOSManager->OnLobbiesFound.AddDynamic(this, &UMatchWidget::HandleLobbiesFound);

		SOSManager->OnLobbyJoined.RemoveDynamic(this, &UMatchWidget::HandleLobbyJoined);
		SOSManager->OnLobbyJoined.AddDynamic(this, &UMatchWidget::HandleLobbyJoined);
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
		SOSManager->OnLobbyCreated.RemoveDynamic(this, &UMatchWidget::HandleLobbyCreated);
		SOSManager->OnLobbiesFound.RemoveDynamic(this, &UMatchWidget::HandleLobbiesFound);
		SOSManager->OnLobbyJoined.RemoveDynamic(this, &UMatchWidget::HandleLobbyJoined);
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
	UE_LOG(LogTemp, Log, TEXT("MazeUI: Match CreateLobby clicked"));
	if (SOSManager)
	{
		SOSManager->CreateLobby(4, TEXT("/Game/Levels/TitleLevel"), true);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("MazeUI: SOSManager missing on CreateLobby"));
	}
}

void UMatchWidget::HandleFindLobbyClicked()
{
	UE_LOG(LogTemp, Log, TEXT("MazeUI: Match FindLobby clicked"));
	if (SOSManager)
	{
		SOSManager->FindLobbies(50, true);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("MazeUI: SOSManager missing on FindLobbies"));
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
		UE_LOG(LogTemp, Warning, TEXT("MazeUI: Lobby item cast failed"));
		return;
	}

	if (SOSManager)
	{
		UE_LOG(LogTemp, Log, TEXT("MazeUI: Join lobby index %d"), ResultItem->LobbyInfo.Index);
		SOSManager->JoinLobbyByIndex(ResultItem->LobbyInfo.Index);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("MazeUI: SOSManager missing on JoinLobby"));
	}
}

void UMatchWidget::HandleLobbyCreated(bool bSuccess)
{
	UE_LOG(LogTemp, Log, TEXT("MazeUI: Lobby created %s"), bSuccess ? TEXT("Success") : TEXT("Failure"));
	if (bSuccess && UIFlowSubsystem)
	{
		UIFlowSubsystem->SetScreenLobby(true);
		RemoveFromParent();
	}
}

void UMatchWidget::HandleLobbiesFound(bool bSuccess, const TArray<FFoundLobbyInfo>& Results)
{
	UE_LOG(LogTemp, Log, TEXT("MazeUI: Lobbies found %s (%d)"), bSuccess ? TEXT("Success") : TEXT("Failure"), Results.Num());

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

	for (const FFoundLobbyInfo& Result : Results)
	{
		ULobbySearchResultItem* NewItem = NewObject<ULobbySearchResultItem>(this);
		NewItem->Init(Result);
		LobbyItems.Add(NewItem);
		LobbyList->AddItem(NewItem);
	}
}

void UMatchWidget::HandleLobbyJoined(bool bSuccess)
{
	UE_LOG(LogTemp, Log, TEXT("MazeUI: Lobby joined %s"), bSuccess ? TEXT("Success") : TEXT("Failure"));
	if (bSuccess && UIFlowSubsystem)
	{
		UIFlowSubsystem->SetScreenLobby(false);
		RemoveFromParent();
	}
}
