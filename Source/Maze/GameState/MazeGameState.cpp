#include "MazeGameState.h"
#include "UI/MazeCountdownWidget.h"
#include "UI/CommonModalWidget.h"
#include "Net/UnrealNetwork.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"

void AMazeGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AMazeGameState, Phase);
    DOREPLIFETIME(AMazeGameState, WinnerPlayer);
    DOREPLIFETIME(AMazeGameState, CountdownEndTime);
}

void AMazeGameState::OnRep_Phase()
{
    if (GetNetMode() == NM_DedicatedServer) return;

    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC) return;

    if (Phase == EMazePhase::Countdown)
    {
        if (CountdownWidgetClass && !CountdownWidgetInstance)
        {
            CountdownWidgetInstance = CreateWidget<UMazeCountdownWidget>(PC, CountdownWidgetClass);
            if (CountdownWidgetInstance)
            {
                CountdownWidgetInstance->AddToViewport();
            }
        }
    }
    else if (Phase == EMazePhase::Playing)
    {
        if (CountdownWidgetInstance)
        {
            CountdownWidgetInstance->RemoveFromParent();
            CountdownWidgetInstance = nullptr;
        }

        // Start BGM loop
        if (BGMSound)
        {
            BGMComponent = UGameplayStatics::SpawnSound2D(this, BGMSound, 0.7);
            if (BGMComponent)
            {
                BGMComponent->bAutoDestroy = false;  // Manual control for Stop()
            }
        }
    }
    else if (Phase == EMazePhase::GameOver)
    {
        if (CountdownWidgetInstance)
        {
            CountdownWidgetInstance->RemoveFromParent();
            CountdownWidgetInstance = nullptr;
        }
        // NOTE: BGM stopped in OnRep_MatchResult, not here (replication order safety)
    }
}

void AMazeGameState::OnRep_MatchResult()
{
    if (GetNetMode() == NM_DedicatedServer) return;
    if (!WinnerPlayer) return;

    // Stop BGM first (before sounds/modal)
    if (BGMComponent)
    {
        BGMComponent->Stop();
        BGMComponent = nullptr;
    }

    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC || !ResultModalClass) return;

    const bool bIsWinner = (WinnerPlayer == PC->PlayerState);

    // Play win/lose sound
    USoundBase* SoundToPlay = bIsWinner ? WinSound : LoseSound;
    if (SoundToPlay)
    {
        UGameplayStatics::PlaySound2D(this, SoundToPlay);
    }

    // Build result message
    const FText Title = bIsWinner
        ? FText::FromString(TEXT("승리!"))
        : FText::FromString(TEXT("패배"));

    FText Message;
    if (bIsWinner)
    {
        Message = FText::FromString(TEXT("먼저 목표에 도달했습니다!"));
    }
    else
    {
        const FString WinnerName = WinnerPlayer->GetPlayerName().IsEmpty()
            ? TEXT("상대방")
            : WinnerPlayer->GetPlayerName();
        Message = FText::FromString(FString::Printf(TEXT("%s 님이 승리했습니다."), *WinnerName));
    }

    // Show modal
    ResultWidgetInstance = CreateWidget<UCommonModalWidget>(PC, ResultModalClass);
    if (ResultWidgetInstance)
    {
        ResultWidgetInstance->AddToViewport();
        ResultWidgetInstance->ShowAlert(Title, Message);
    }
}

void AMazeGameState::SetPhase(EMazePhase NewPhase)
{
    Phase = NewPhase;
    if (HasAuthority())
    {
        OnRep_Phase();
    }
}

void AMazeGameState::SetWinnerPlayer(APlayerState* NewWinner)
{
    WinnerPlayer = NewWinner;
    if (HasAuthority())
    {
        OnRep_MatchResult();
    }
}
