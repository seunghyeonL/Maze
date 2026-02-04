#include "MatchWidget.h"

#include "LobbySearchResultItem.h"
#include "UIFlowSubsystem.h"
#include "LoadingOverlayWidget.h"
#include "CommonModalWidget.h"
#include "OnlineSubsystem/SOSManager.h"

#include "Components/Button.h"
#include "Components/ListView.h"
#include "GameMode/TitleGameMode.h"

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
	
	if (auto* TitleGameMode = Cast<ATitleGameMode>(GetWorld()->GetAuthGameMode()))
	{
		TitleGameMode->OnLoginFailed.RemoveDynamic(this, &UMatchWidget::ShowAlert);
		TitleGameMode->OnLoginFailed.AddDynamic(this, &UMatchWidget::ShowAlert);
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
	
	// if (auto TitleGameMode = Cast<ATitle>(GetWorld()->GetAuthGameMode()))

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

void UMatchWidget::ShowLoading(const FText& Message)
{
	if (LoadingOverlay)
	{
		LoadingOverlay->Show(Message);
	}
}

void UMatchWidget::HideLoading()
{
	if (LoadingOverlay)
	{
		LoadingOverlay->Hide();
	}
}

void UMatchWidget::ShowAlert(const FText& Title, const FText& Message)
{
	if (AlertModal)
	{
		AlertModal->ShowAlert(Title, Message);
	}
}

void UMatchWidget::HandleCreateLobbyClicked()
{
	UE_LOG(LogTemp, Log, TEXT("MazeUI: Match CreateSession clicked"));
	if (SOSManager)
	{
		ShowLoading(FText::FromString(TEXT("세션 생성 중...")));
		SOSManager->CreateSession(2, TEXT("/Game/Levels/TitleLevel"), true);
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
		ShowLoading(FText::FromString(TEXT("방 검색 중...")));
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
		ShowLoading(FText::FromString(TEXT("방 참가 중...")));
		SOSManager->JoinSessionByIndex(ResultItem->SessionInfo.Index);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("MazeUI: SOSManager missing on JoinSession"));
	}
}

void UMatchWidget::HandleSessionCreated(bool bSuccess)
{
	HideLoading();
	UE_LOG(LogTemp, Log, TEXT("MazeUI: Session created %s"), bSuccess ? TEXT("Success") : TEXT("Failure"));

	if (bSuccess && UIFlowSubsystem)
	{
		UIFlowSubsystem->SetScreenLobby(true);
		RemoveFromParent();
	}
	else if (!bSuccess)
	{
		ShowAlert(
			FText::FromString(TEXT("오류")),
			FText::FromString(TEXT("세션 생성에 실패했습니다."))
		);
	}
}

void UMatchWidget::HandleSessionsFound(bool bSuccess, const TArray<FFoundSessionInfo>& Results)
{
	HideLoading();
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
		ShowAlert(
			FText::FromString(TEXT("오류")),
			FText::FromString(TEXT("방 검색에 실패했습니다."))
		);
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
	HideLoading();
	UE_LOG(LogTemp, Log, TEXT("MazeUI: Session joined %s"), bSuccess ? TEXT("Success") : TEXT("Failure"));

	if (bSuccess && UIFlowSubsystem)
	{
		UIFlowSubsystem->SetScreenLobby(false);
		RemoveFromParent();
	}
	else if (!bSuccess)
	{
		ShowAlert(
			FText::FromString(TEXT("오류")),
			FText::FromString(TEXT("방 참가에 실패했습니다.\n방이 가득 찼거나 연결할 수 없습니다."))
		);
	}
}
