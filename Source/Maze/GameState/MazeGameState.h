#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "MazeGameState.generated.h"

class UMazeCountdownWidget;
class UCommonModalWidget;

UENUM(BlueprintType)
enum class EMazePhase : uint8
{
    WaitingForPlayers,
    Countdown,
    Playing,
    GameOver
};

UCLASS()
class MAZE_API AMazeGameState : public AGameState
{
    GENERATED_BODY()

public:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    void SetPhase(EMazePhase NewPhase);
    void SetWinnerPlayer(APlayerState* NewWinner);

    UPROPERTY(ReplicatedUsing=OnRep_Phase, BlueprintReadOnly, Category="Maze")
    EMazePhase Phase = EMazePhase::WaitingForPlayers;

    UPROPERTY(ReplicatedUsing=OnRep_MatchResult, BlueprintReadOnly, Category="Maze")
    TObjectPtr<APlayerState> WinnerPlayer;

    UPROPERTY(Replicated, BlueprintReadOnly, Category="Maze")
    float CountdownEndTime = 0.f;

    UPROPERTY(EditDefaultsOnly, Category="Maze|UI")
    TSubclassOf<UMazeCountdownWidget> CountdownWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category="Maze|UI")
    TSubclassOf<UCommonModalWidget> ResultModalClass;

private:
    UFUNCTION()
    void OnRep_Phase();

    UFUNCTION()
    void OnRep_MatchResult();

    UPROPERTY()
    TObjectPtr<UMazeCountdownWidget> CountdownWidgetInstance;

    UPROPERTY()
    TObjectPtr<UCommonModalWidget> ResultWidgetInstance;
};
