#include "TitlePlayerController.h"

#include "LobbyWidget.h"
#include "MatchWidget.h"
#include "TitleWidget.h"
#include "UIFlowSubsystem.h"

#include "OnlineSubsystem/SOSManager.h"

#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	bool IsMazeLevel(const UWorld* World)
	{
		if (!World)
		{
			return false;
		}

	const FString LevelName = UGameplayStatics::GetCurrentLevelName(World, true);
	return LevelName.Equals(TEXT("MazeLevel"), ESearchCase::CaseSensitive);
	}
}

void ATitlePlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	if (GEngine)
	{
		GEngine->OnNetworkFailure().RemoveAll(this);
		GEngine->OnNetworkFailure().AddUObject(this, &ATitlePlayerController::HandleNetworkFailure);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(UIRefreshTimerHandle, this, &ATitlePlayerController::RefreshUI, 0.1f, true);
	}

	RefreshUI();
}

void ATitlePlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GEngine)
	{
		GEngine->OnNetworkFailure().RemoveAll(this);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(UIRefreshTimerHandle);
	}

	ClearActiveWidget();
	Super::EndPlay(EndPlayReason);
}

void ATitlePlayerController::RefreshUI()
{
	if (!IsLocalController())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (IsMazeLevel(World))
	{
		ClearActiveWidget();
		SetupGameInput();
		return;
	}

	UUIFlowSubsystem* Flow = GetGameInstance() ? GetGameInstance()->GetSubsystem<UUIFlowSubsystem>() : nullptr;
	const EUIFlowScreen Screen = Flow ? Flow->GetScreen() : EUIFlowScreen::Title;

	UClass* DesiredClass = nullptr;
	switch (Screen)
	{
	case EUIFlowScreen::Title:
		DesiredClass = TitleWidgetClass;
		break;
	case EUIFlowScreen::Match:
		DesiredClass = MatchWidgetClass;
		break;
	case EUIFlowScreen::Lobby:
		DesiredClass = LobbyWidgetClass;
		break;
	default:
		DesiredClass = TitleWidgetClass;
		break;
	}

	if (!DesiredClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("MazeUI: Missing widget class for screen %d"), static_cast<int32>(Screen));
		return;
	}

	if (ActiveWidget && ActiveWidget->GetClass() == DesiredClass)
	{
		return;
	}

	ClearActiveWidget();

	ActiveWidget = CreateWidget<UUserWidget>(this, DesiredClass);
	if (!ActiveWidget)
	{
		UE_LOG(LogTemp, Warning, TEXT("MazeUI: Failed to create widget for screen %d"), static_cast<int32>(Screen));
		return;
	}

	ActiveWidget->AddToViewport();
	SetupUIInput(ActiveWidget);
}

void ATitlePlayerController::ClearActiveWidget()
{
	if (ActiveWidget)
	{
		ActiveWidget->RemoveFromParent();
		ActiveWidget = nullptr;
	}
}

void ATitlePlayerController::SetupUIInput(UUserWidget* Widget)
{
	bShowMouseCursor = true;

	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	if (Widget)
	{
		InputMode.SetWidgetToFocus(Widget->TakeWidget());
	}
	SetInputMode(InputMode);

	if (Widget)
	{
		Widget->SetKeyboardFocus();
	}
}

void ATitlePlayerController::SetupGameInput()
{
	bShowMouseCursor = false;
	SetInputMode(FInputModeGameOnly());
}

void ATitlePlayerController::HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	(void)World;
	(void)NetDriver;
	(void)FailureType;
	(void)ErrorString;

	UUIFlowSubsystem* Flow = GetGameInstance() ? GetGameInstance()->GetSubsystem<UUIFlowSubsystem>() : nullptr;
	if (Flow)
	{
		Flow->SetScreenMatch();
	}

	// Ensure local OnlineSession state is cleared, otherwise Join/Create can fail with
	// "Session (GameSession) already exists" after a disconnect.
	if (USOSManager* SOS = GetGameInstance() ? GetGameInstance()->GetSubsystem<USOSManager>() : nullptr)
	{
		SOS->DestroySession();
	}

	RefreshUI();
}
