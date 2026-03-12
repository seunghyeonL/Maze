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
    DOREPLIFETIME(AMazeGameState, MazeSeed);
    DOREPLIFETIME(AMazeGameState, MazeWidth);
    DOREPLIFETIME(AMazeGameState, MazeHeight);
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
            BGMComponent = UGameplayStatics::SpawnSound2D(this, BGMSound);
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

void AMazeGameState::SetMazeData(int32 InSeed, int32 InWidth, int32 InHeight)
{
    MazeSeed = InSeed;
    MazeWidth = InWidth;
    MazeHeight = InHeight;
    ForceNetUpdate();
}

void AMazeGameState::OnRep_MazeSeed()
{
    // Listen server guard — server already spawned walls in GenerateAndSpawnMaze
    if (HasAuthority()) return;
    
    TRACE_BOOKMARK(TEXT("AMazeGameState::OnRep_MazeSeed"));

    // Sentinel guard — ignore initial value (0) or invalid sizes
    if (MazeSeed == 0 || MazeWidth < 2 || MazeHeight < 2) return;

    // Build grid (same seed → same grid guaranteed)
    TArray<FCellRow> Grid;
    Grid.SetNum(MazeHeight);
    for (auto& Row : Grid)
    {
        Row.Cells.SetNum(MazeWidth);
    }
    UMazeGenerator::BuildMazeGrid(MazeHeight, MazeWidth, MazeSeed, Grid);

    // Spawn walls locally (no replication — BP_MazeWall bReplicates=false assumed)
    if (!WallClass)
    {
        UE_LOG(LogTemp, Error, TEXT("MazeGameState: WallClass is null — cannot spawn walls on client"));
        return;
    }
    const int32 WallCount = UMazeGenerator::SpawnWalls(this, Grid, MazeHeight, MazeWidth, CellSize, WallClass);
    UE_LOG(LogTemp, Log, TEXT("MazeGameState: Client spawned %d walls (Seed=%d)"), WallCount, MazeSeed);
}
