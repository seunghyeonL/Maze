#include "TitlePlayerController.h"

#include "../UI/LobbyWidget.h"
#include "../UI/MatchWidget.h"
#include "../UI/TitleWidget.h"
#include "../UI/UIFlowSubsystem.h"

#include "OnlineSubsystem/SOSManager.h"

#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Settings/MazeSettings.h"

namespace
{
	bool IsMazeLevel(const UWorld* World)
	{
		if (!World)
		{
			return false;
		}

	const FString LevelName = UGameplayStatics::GetCurrentLevelName(World, true);
	return LevelName.Equals(GetDefault<UMazeSettings>()->GetMazeLevelName(), ESearchCase::CaseSensitive);
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

	if (UUIFlowSubsystem* Flow = GetGameInstance() ? GetGameInstance()->GetSubsystem<UUIFlowSubsystem>() : nullptr)
	{
		Flow->OnScreenChanged.RemoveDynamic(this, &ATitlePlayerController::HandleScreenChanged);
		Flow->OnScreenChanged.AddDynamic(this, &ATitlePlayerController::HandleScreenChanged);
	}

	RefreshUI();
}

void ATitlePlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GEngine)
	{
		GEngine->OnNetworkFailure().RemoveAll(this);
	}

	if (UUIFlowSubsystem* Flow = GetGameInstance() ? GetGameInstance()->GetSubsystem<UUIFlowSubsystem>() : nullptr)
	{
		Flow->OnScreenChanged.RemoveDynamic(this, &ATitlePlayerController::HandleScreenChanged);
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

void ATitlePlayerController::HandleScreenChanged(EUIFlowScreen NewScreen)
{
	RefreshUI();
}

void ATitlePlayerController::HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	(void)World;
	(void)NetDriver;

	UE_LOG(LogTemp, Warning, TEXT("MazeUI: NetworkFailure - Type: %d, Error: %s"), 
		static_cast<int32>(FailureType), *ErrorString);

	UUIFlowSubsystem* Flow = GetGameInstance() ? GetGameInstance()->GetSubsystem<UUIFlowSubsystem>() : nullptr;
	if (Flow)
	{
		// PreLogin 거부 메시지나 기타 에러를 pending error로 저장
		FText ErrorMessage;
		if (ErrorString.Contains(TEXT("full")) || ErrorString.Contains(TEXT("Server is full")))
		{
			ErrorMessage = FText::FromString(TEXT("방이 가득 찼습니다."));
		}
		else if (!ErrorString.IsEmpty())
		{
			ErrorMessage = FText::Format(
				FText::FromString(TEXT("연결 실패: {0}")),
				FText::FromString(ErrorString)
			);
		}
		else
		{
			ErrorMessage = FText::FromString(TEXT("서버와의 연결이 끊어졌습니다."));
		}

		Flow->SetPendingError(ErrorMessage);
		ClearActiveWidget();  // MatchWidget 로드 오버레이 stuck 방지: 강제 재생성 보장
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
