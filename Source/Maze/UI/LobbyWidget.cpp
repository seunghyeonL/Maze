#include "LobbyWidget.h"

#include "LobbyPlayerEntryItem.h"
#include "PlayerState/MazeLobbyPlayerState.h"
#include "OnlineSubsystem/SOSManager.h"
#include "UIFlowSubsystem.h"
#include "LoadingOverlayWidget.h"
#include "CommonModalWidget.h"
#include "Settings/MazeLevelSettings.h"
#include "GameState/MazeLobbyGameState.h"

#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/ListView.h"
#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/GameplayStatics.h"

void ULobbyWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetIsFocusable(true);
	CacheSubsystems();
	UpdateRoleVisibility();

	if (GameStartButton)
	{
		GameStartButton->OnClicked.RemoveDynamic(this, &ULobbyWidget::HandleGameStartClicked);
		GameStartButton->OnClicked.AddDynamic(this, &ULobbyWidget::HandleGameStartClicked);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("MazeUI: GameStartButton missing"));
	}

	if (ReadyButton)
	{
		ReadyButton->OnClicked.RemoveDynamic(this, &ULobbyWidget::HandleReadyClicked);
		ReadyButton->OnClicked.AddDynamic(this, &ULobbyWidget::HandleReadyClicked);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("MazeUI: ReadyButton missing"));
	}

	if (ExitToMatchingButton)
	{
		ExitToMatchingButton->OnClicked.RemoveDynamic(this, &ULobbyWidget::HandleExitToMatchingClicked);
		ExitToMatchingButton->OnClicked.AddDynamic(this, &ULobbyWidget::HandleExitToMatchingClicked);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("MazeUI: ExitToMatchingButton missing"));
	}
	if (MazeSizeComboBox)
	{
		MazeSizeComboBox->ClearOptions();
		MazeSizeComboBox->AddOption(TEXT("5"));
		MazeSizeComboBox->AddOption(TEXT("7"));
		MazeSizeComboBox->AddOption(TEXT("9"));
		MazeSizeComboBox->AddOption(TEXT("11"));
		MazeSizeComboBox->SetSelectedOption(TEXT("9"));
	}

	if (MazeSizeComboBox)
	{
		MazeSizeComboBox->OnSelectionChanged.RemoveDynamic(this, &ULobbyWidget::HandleMazeSizeSelectionChanged);
		MazeSizeComboBox->OnSelectionChanged.AddDynamic(this, &ULobbyWidget::HandleMazeSizeSelectionChanged);
	}


	if (!PlayerList)
	{
		UE_LOG(LogTemp, Warning, TEXT("MazeUI: PlayerList missing"));
	}

	if (!IsLobbyHost())
	{
		UE_LOG(LogTemp, Log, TEXT("MazeUI: LobbyWidget showing loading overlay for client sync"));
		ShowLoading(FText::FromString(TEXT("로비 정보 동기화 중...")));
	}

	RefreshPlayerList();

	SetKeyboardFocus();
}

void ULobbyWidget::NativeDestruct()
{
	if (GameStartButton)
	{
		GameStartButton->OnClicked.RemoveDynamic(this, &ULobbyWidget::HandleGameStartClicked);
	}

	if (ReadyButton)
	{
		ReadyButton->OnClicked.RemoveDynamic(this, &ULobbyWidget::HandleReadyClicked);
	}

	if (ExitToMatchingButton)
	{
		ExitToMatchingButton->OnClicked.RemoveDynamic(this, &ULobbyWidget::HandleExitToMatchingClicked);
	}

	if (MazeSizeComboBox)
	{
		MazeSizeComboBox->OnSelectionChanged.RemoveDynamic(this, &ULobbyWidget::HandleMazeSizeSelectionChanged);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(GameStartTimerHandle);
	}

	UnbindPlayerStates();

	Super::NativeDestruct();
}

void ULobbyWidget::CacheSubsystems()
{
	if (GetGameInstance())
	{
		SOSManager = GetGameInstance()->GetSubsystem<USOSManager>();
		UIFlowSubsystem = GetGameInstance()->GetSubsystem<UUIFlowSubsystem>();
	}
}

bool ULobbyWidget::IsLobbyHost() const
{
	if (UIFlowSubsystem)
	{
		return UIFlowSubsystem->IsLobbyHost();
	}

	if (const UWorld* World = GetWorld())
	{
		return World->GetNetMode() != NM_Client;
	}

	return false;
}

void ULobbyWidget::UpdateRoleVisibility()
{
	const bool bIsHost = IsLobbyHost();

	if (GameStartButton)
	{
		GameStartButton->SetVisibility(bIsHost ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	if (ReadyButton)
	{
		ReadyButton->SetVisibility(ESlateVisibility::Visible);
	}
	
	if (MazeSizeComboBox)
	{
		MazeSizeComboBox->SetVisibility(ESlateVisibility::Visible);
		MazeSizeComboBox->SetIsEnabled(bIsHost);
	}
}

void ULobbyWidget::HandleReadyClicked()
{
	AMazeLobbyPlayerState* LobbyPlayerState = GetOwningPlayerState<AMazeLobbyPlayerState>();
	if (!LobbyPlayerState)
	{
		UE_LOG(LogTemp, Warning, TEXT("MazeUI: Ready clicked without lobby player state"));
		return;
	}

	const bool bNewReady = !LobbyPlayerState->IsReady();
	UE_LOG(LogTemp, Log, TEXT("MazeUI: Ready clicked -> %s"), bNewReady ? TEXT("Ready") : TEXT("NotReady"));
	LobbyPlayerState->RequestSetReady(bNewReady);
}

void ULobbyWidget::HandleGameStartClicked()
{
	if (!IsLobbyHost())
	{
		UE_LOG(LogTemp, Warning, TEXT("MazeUI: GameStart clicked by non-host"));
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("MazeUI: GameStart failed (no world)"));
		return;
	}

	if (World->GetNetMode() == NM_Client)
	{
		UE_LOG(LogTemp, Warning, TEXT("MazeUI: GameStart rejected on client"));
		return;
	}

	AGameStateBase* GameState = World->GetGameState();
	if (!GameState)
	{
		UE_LOG(LogTemp, Warning, TEXT("MazeUI: GameStart failed (no GameState)"));
		return;
	}

	AMazeLobbyGameState* LobbyGameState = Cast<AMazeLobbyGameState>(GameState);

	// Ready 체크
	for (APlayerState* PS : GameState->PlayerArray)
	{
		const AMazeLobbyPlayerState* LobbyPS = Cast<AMazeLobbyPlayerState>(PS);
		if (!LobbyPS) continue;
		if (!LobbyPS->IsReady())
		{
			UE_LOG(LogTemp, Log, TEXT("MazeUI: GameStart blocked (not all players ready)"));
			ShowAlert(FText::FromString(TEXT("알림")), FText::FromString(TEXT("모두 준비되지 않았습니다.")));
			return;
		}
	}

	// === 모두 Ready — 게임 시작 시퀀스 ===

	// 1. 더블클릭 방지
	if (GameStartButton) GameStartButton->SetIsEnabled(false);

	// 2. PlayerListChanged 구독 해제 (ShowLoading auto-hide 방지)
	if (BoundGameState.IsValid()) BoundGameState->OnPlayerListChanged.RemoveDynamic(this, &ULobbyWidget::HandlePlayerListChanged);

	// 3. 호스트 즉시 LoadingOverlay 표시
	ShowLoading(FText::FromString(TEXT("게임 시작 중...")));

	// 4. bGameStarted 설정 (클라이언트에 OnRep 전달)
	if (LobbyGameState) LobbyGameState->SetGameStarted(true);

	// 5. 세션 설정
	if (SOSManager) SOSManager->SetExpectedPlayers(GameState->PlayerArray.Num());

	// 6. MazeSize를 GameState에서 읽기
	const FString SelectedSize = LobbyGameState
		? FString::FromInt(LobbyGameState->GetSelectedMazeSize())
		: (MazeSizeComboBox ? MazeSizeComboBox->GetSelectedOption() : TEXT("9"));

	// 7. 0.3초 지연 후 ServerTravel (OnRep이 클라이언트에 도달할 시간 확보)
	const FString TravelURL = FString::Printf(TEXT("%s?listen?MazeSize=%s"), *GetDefault<UMazeLevelSettings>()->GetMazeLevelPath(), *SelectedSize);
	UE_LOG(LogTemp, Log, TEXT("MazeUI: GameStart travel to MazeLevel (ExpectedPlayers=%d, MazeSize=%s)"),
		GameState->PlayerArray.Num(), *SelectedSize);

	FTimerDelegate TimerDelegate;
	TimerDelegate.BindLambda([World, TravelURL]()
	{
		if (World) World->ServerTravel(TravelURL);
	});
	World->GetTimerManager().SetTimer(GameStartTimerHandle, TimerDelegate, 0.3f, false);
}

void ULobbyWidget::HandleExitToMatchingClicked()
{
	UE_LOG(LogTemp, Log, TEXT("MazeUI: Lobby ExitToMatching clicked"));

	const FString TitleLevelUrl = GetDefault<UMazeLevelSettings>()->GetTitleLevelPath();

	UWorld* World = GetWorld();
	const bool bHost = IsLobbyHost();

	if (UIFlowSubsystem)
	{
		UIFlowSubsystem->SetScreenMatch();
	}

	if (bHost && World && World->GetNetMode() != NM_Client)
	{
		if (SOSManager)
		{
			SOSManager->DestroySession();
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("MazeUI: SOSManager missing on ExitToMatching"));
		}

		UE_LOG(LogTemp, Log, TEXT("MazeUI: Host returning to TitleLevel as Standalone"));
		UGameplayStatics::OpenLevel(this, FName(*TitleLevelUrl), true);
		return;
	}

	// Client: actually disconnect by reopening TitleLevel locally.
	// Without this, the client may remain connected to the listen server and keep its GameSession around.
	if (!bHost && World && World->GetNetMode() == NM_Client)
	{
		UE_LOG(LogTemp, Log, TEXT("MazeUI: Client returning to TitleLevel as Standalone"));
		if (SOSManager)
		{
			SOSManager->DestroySession();
		}
		UGameplayStatics::OpenLevel(this, FName(*TitleLevelUrl), true);
		return;
	}

	RemoveFromParent();
}

void ULobbyWidget::HandleReadyChanged(AMazeLobbyPlayerState* PlayerState, bool bIsReady)
{
	(void)PlayerState;
	(void)bIsReady;
	RefreshPlayerList();
}

void ULobbyWidget::HandlePlayerListChanged()
{
	RefreshPlayerList();
}

void ULobbyWidget::RefreshPlayerList()
{
	if (!PlayerList)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const AGameStateBase* GameState = World->GetGameState();
	if (!GameState)
	{
		return;
	}

	PlayerList->ClearListItems();
	PlayerItems.Reset();
	UnbindPlayerStates();
	BindGameStateMazeSize();

	AMazeLobbyGameState* LobbyGameState = GetWorld() ? GetWorld()->GetGameState<AMazeLobbyGameState>() : nullptr;
	if (!IsLobbyHost() && LobbyGameState)
	{
		const int32 CurrentMazeSize = LobbyGameState->GetSelectedMazeSize();
		if (MazeSizeComboBox && CurrentMazeSize != 9)
		{
			MazeSizeComboBox->SetSelectedOption(FString::FromInt(CurrentMazeSize));
		}
	}

	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		AMazeLobbyPlayerState* LobbyPlayerState = Cast<AMazeLobbyPlayerState>(PlayerState);
		if (!LobbyPlayerState)
		{
			continue;
		}

		ULobbyPlayerEntryItem* NewItem = NewObject<ULobbyPlayerEntryItem>(this);
		NewItem->Init(LobbyPlayerState->GetPlayerName(), LobbyPlayerState->IsReady());
		PlayerItems.Add(NewItem);
		PlayerList->AddItem(NewItem);
		BindPlayerStateReady(LobbyPlayerState);
	}

	if (LoadingOverlay && LoadingOverlay->IsShowing() && PlayerItems.Num() > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("MazeUI: LobbyWidget hiding loading overlay (sync complete, %d players)"), PlayerItems.Num());
		HideLoading();
	}
}

void ULobbyWidget::BindPlayerStateReady(AMazeLobbyPlayerState* PlayerState)
{
	if (!PlayerState)
	{
		return;
	}

	PlayerState->OnReadyChanged.RemoveDynamic(this, &ULobbyWidget::HandleReadyChanged);
	PlayerState->OnReadyChanged.AddDynamic(this, &ULobbyWidget::HandleReadyChanged);
	BoundPlayerStates.AddUnique(PlayerState);
}

void ULobbyWidget::BindGameStateMazeSize()
{
	AMazeLobbyGameState* LobbyGameState = GetWorld() ? GetWorld()->GetGameState<AMazeLobbyGameState>() : nullptr;
	if (!LobbyGameState || BoundGameState.Get() == LobbyGameState) return;

	UnbindGameState();
	LobbyGameState->OnMazeSizeChanged.AddDynamic(this, &ULobbyWidget::HandleMazeSizeChanged);
	LobbyGameState->OnGameStarted.AddDynamic(this, &ULobbyWidget::HandleGameStarted);
	LobbyGameState->OnPlayerListChanged.AddDynamic(this, &ULobbyWidget::HandlePlayerListChanged);
	BoundGameState = LobbyGameState;
}

void ULobbyWidget::UnbindGameState()
{
	if (BoundGameState.IsValid())
	{
		BoundGameState->OnMazeSizeChanged.RemoveDynamic(this, &ULobbyWidget::HandleMazeSizeChanged);
		BoundGameState->OnGameStarted.RemoveDynamic(this, &ULobbyWidget::HandleGameStarted);
		BoundGameState->OnPlayerListChanged.RemoveDynamic(this, &ULobbyWidget::HandlePlayerListChanged);
	}
	BoundGameState.Reset();
}

void ULobbyWidget::UnbindPlayerStates()
{
	for (const TWeakObjectPtr<AMazeLobbyPlayerState>& PlayerState : BoundPlayerStates)
	{
		if (PlayerState.IsValid())
		{
			PlayerState->OnReadyChanged.RemoveDynamic(this, &ULobbyWidget::HandleReadyChanged);
		}
	}

	BoundPlayerStates.Reset();
	UnbindGameState();
}

void ULobbyWidget::HandleMazeSizeSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	// 피드백 루프 방지: 프로그래밍적 변경은 무시
	if (SelectionType == ESelectInfo::Direct)
	{
		return;
	}
	// 클라이언트 방어: 호스트만 요청 전송
	if (!IsLobbyHost())
	{
		return;
	}
	const int32 NewSize = FCString::Atoi(*SelectedItem);
	AMazeLobbyGameState* LobbyGameState = GetWorld() ? GetWorld()->GetGameState<AMazeLobbyGameState>() : nullptr;
	if (LobbyGameState) { LobbyGameState->SetSelectedMazeSize(NewSize); }
}

void ULobbyWidget::HandleMazeSizeChanged(int32 NewMazeSize)
{
	if (!MazeSizeComboBox) return;
	if (IsLobbyHost()) return;  // 호스트 ComboBox는 source of truth
	MazeSizeComboBox->SetSelectedOption(FString::FromInt(NewMazeSize));
}

void ULobbyWidget::ShowLoading(const FText& Message)
{
	if (LoadingOverlay)
	{
		LoadingOverlay->Show(Message);
	}
}

void ULobbyWidget::HideLoading()
{
	if (LoadingOverlay)
	{
		LoadingOverlay->Hide();
	}
}

void ULobbyWidget::ShowAlert(const FText& Title, const FText& Message)
{
	if (AlertModal)
	{
		AlertModal->ShowAlert(Title, Message);
	}
}

void ULobbyWidget::HandleGameStarted()
{
	if (IsLobbyHost()) return;  // 호스트는 HandleGameStartClicked에서 직접 처리
	
	// PlayerListChanged 구독 해제 (ShowLoading auto-hide 방지)
	if (BoundGameState.IsValid()) BoundGameState->OnPlayerListChanged.RemoveDynamic(this, &ULobbyWidget::HandlePlayerListChanged);
	
	ShowLoading(FText::FromString(TEXT("게임 시작 중...")));
}
