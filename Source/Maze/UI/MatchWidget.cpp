#include "MatchWidget.h"

#include "LobbySearchResultItem.h"
#include "UIFlowSubsystem.h"
#include "LoadingOverlayWidget.h"
#include "CommonModalWidget.h"
#include "OnlineSubsystem/SOSManager.h"
#include "Settings/MazeSettings.h"

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
	
	// if (auto* TitleGameMode = Cast<ATitleGameMode>(GetWorld()->GetAuthGameMode()))
	// {
	// 	TitleGameMode->OnLoginFailed.RemoveDynamic(this, &UMatchWidget::ShowAlert);
	// 	TitleGameMode->OnLoginFailed.AddDynamic(this, &UMatchWidget::ShowAlert);
	// }

	// Pending error 체크를 지연 호출 (레벨 전환 후 위젯 재생성 시 모달이 사라지는 문제 방지)
	if (UIFlowSubsystem && UIFlowSubsystem->HasPendingError())
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				PendingErrorTimerHandle,
				this,
				&UMatchWidget::CheckPendingError,
				0.5f,  // 0.5초 딜레이
				false
			);
		}
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

	// 타이머 정리
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PendingErrorTimerHandle);
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
		SOSManager->CreateSession(MaxPlayerNum, GetDefault<UMazeSettings>()->GetTitleLevelPath(), true);
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
	UE_LOG(LogTemp, Log, TEXT("MazeUI: Session joined %s"), bSuccess ? TEXT("Success") : TEXT("Failure"));

	if (bSuccess && UIFlowSubsystem)
	{
		// 상태만 설정 — 브로드캐스트 없이 (구 World에서 premature LobbyWidget 생성 방지)
		// 새 World의 TitlePlayerController::BeginPlay → RefreshUI()가 Screen=Lobby를 읽어 LobbyWidget 생성
		UIFlowSubsystem->SetScreenLobbyForTravel(false);
		// HideLoading() 호출 않음 — "방 참가 중..." 로드 유지
		// RemoveFromParent() 호출 않음 — ClientTravel 시 World 파괴와 함께 자동 정리
	}
	else if (!bSuccess)
	{
		HideLoading();
		ShowAlert(
			FText::FromString(TEXT("오류")),
			FText::FromString(TEXT("방 참가에 실패했습니다.\n방이 가득 찼거나 연결할 수 없습니다."))
		);
	}
}

void UMatchWidget::CheckPendingError()
{
	if (UIFlowSubsystem && UIFlowSubsystem->HasPendingError())
	{
		const FText ErrorMessage = UIFlowSubsystem->ConsumePendingError();
		ShowAlert(FText::FromString(TEXT("연결 실패")), ErrorMessage);
	}
}
