#include "LobbyWidget.h"

#include "LobbyPlayerEntryItem.h"
#include "PlayerState/MazeLobbyPlayerState.h"
#include "OnlineSubsystem/SOSManager.h"
#include "UIFlowSubsystem.h"

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

	RefreshPlayerList();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(RefreshTimerHandle, this, &ULobbyWidget::RefreshPlayerList, 1.0f, true);
	}

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
		World->GetTimerManager().ClearTimer(RefreshTimerHandle);
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

	const AGameStateBase* GameState = World->GetGameState();
	if (!GameState)
	{
		UE_LOG(LogTemp, Warning, TEXT("MazeUI: GameStart failed (no GameState)"));
		return;
	}

	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		const AMazeLobbyPlayerState* LobbyPlayerState = Cast<AMazeLobbyPlayerState>(PlayerState);
		if (!LobbyPlayerState)
		{
			continue;
		}

		if (!LobbyPlayerState->IsReady())
		{
			UE_LOG(LogTemp, Log, TEXT("MazeUI: GameStart blocked (not all players ready)"));
			return;
		}
	}

	// 현재 로비 인원수를 세션 설정에 저장 (MazeGameMode가 읽어서 사용)
	if (SOSManager)
	{
		SOSManager->SetExpectedPlayers(GameState->PlayerArray.Num());
	}

	const FString SelectedSize = MazeSizeComboBox ? MazeSizeComboBox->GetSelectedOption() : TEXT("9");
	const FString TravelURL = FString::Printf(TEXT("/Game/Levels/MazeLevel?listen?MazeSize=%s"), *SelectedSize);
	UE_LOG(LogTemp, Log, TEXT("MazeUI: GameStart travel to MazeLevel (ExpectedPlayers=%d, MazeSize=%s)"),
		GameState->PlayerArray.Num(), *SelectedSize);
	World->ServerTravel(*TravelURL);
}

void ULobbyWidget::HandleExitToMatchingClicked()
{
	UE_LOG(LogTemp, Log, TEXT("MazeUI: Lobby ExitToMatching clicked"));

	static const FString TitleLevelUrl = TEXT("/Game/Levels/TitleLevel");

	UWorld* World = GetWorld();
	const bool bHost = IsLobbyHost();
	if (bHost && World && World->GetNetMode() != NM_Client)
	{
		if (World->GetAuthGameMode())
		{
			for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
			{
				APlayerController* PC = It->Get();
				if (!PC || PC->IsLocalController())
				{
					continue;
				}

				// Force remote clients to disconnect and return to TitleLevel.
				PC->ClientTravel(TitleLevelUrl, ETravelType::TRAVEL_Absolute);
			}
		}
	}

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
		BindPlayerStateMazeSize(LobbyPlayerState);

		// 초기 동기화: 호스트가 이미 크기를 변경했다면 ComboBox 업데이트
		if (!IsLobbyHost())
		{
			const int32 CurrentMazeSize = LobbyPlayerState->GetSelectedMazeSize();
			if (MazeSizeComboBox && CurrentMazeSize != 9)
			{
				MazeSizeComboBox->SetSelectedOption(FString::FromInt(CurrentMazeSize));
			}
		}
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

void ULobbyWidget::BindPlayerStateMazeSize(AMazeLobbyPlayerState* PlayerState)
{
	if (!PlayerState)
	{
		return;
	}

	PlayerState->OnMazeSizeChanged.RemoveDynamic(this, &ULobbyWidget::HandleMazeSizeChanged);
	PlayerState->OnMazeSizeChanged.AddDynamic(this, &ULobbyWidget::HandleMazeSizeChanged);
	BoundPlayerStates.AddUnique(PlayerState);
}

void ULobbyWidget::UnbindPlayerStates()
{
	for (const TWeakObjectPtr<AMazeLobbyPlayerState>& PlayerState : BoundPlayerStates)
	{
		if (PlayerState.IsValid())
		{
			PlayerState->OnReadyChanged.RemoveDynamic(this, &ULobbyWidget::HandleReadyChanged);
			PlayerState->OnMazeSizeChanged.RemoveDynamic(this, &ULobbyWidget::HandleMazeSizeChanged);
		}
	}

	BoundPlayerStates.Reset();
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
	AMazeLobbyPlayerState* LobbyPS = GetOwningPlayerState<AMazeLobbyPlayerState>();
	if (!LobbyPS)
	{
		return;
	}
	const int32 NewSize = FCString::Atoi(*SelectedItem);
	LobbyPS->RequestSetMazeSize(NewSize);
}

void ULobbyWidget::HandleMazeSizeChanged(AMazeLobbyPlayerState* PlayerState, int32 NewMazeSize)
{
	(void)PlayerState;
	if (!MazeSizeComboBox)
	{
		return;
	}
	// 호스트의 ComboBox는 source of truth이므로 업데이트 건너뜀
	if (IsLobbyHost())
	{
		return;
	}
	MazeSizeComboBox->SetSelectedOption(FString::FromInt(NewMazeSize));
}
