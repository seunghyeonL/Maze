#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "Helper/MazeGenerator.h"
#include "MazeGameState.generated.h"

class UMazeCountdownWidget;
class UCommonModalWidget;
class USoundBase;
class UAudioComponent;

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
    void SetMazeData(int32 InSeed, int32 InWidth, int32 InHeight);

    UPROPERTY(ReplicatedUsing=OnRep_Phase, BlueprintReadOnly, Category="Maze")
    EMazePhase Phase = EMazePhase::WaitingForPlayers;

    UPROPERTY(ReplicatedUsing=OnRep_MatchResult, BlueprintReadOnly, Category="Maze")
    TObjectPtr<APlayerState> WinnerPlayer;

    UPROPERTY(Replicated, BlueprintReadOnly, Category="Maze")
    float CountdownEndTime = 0.f;

    UPROPERTY(ReplicatedUsing=OnRep_MazeSeed, BlueprintReadOnly, Category="Maze")
    int32 MazeSeed = 0;

    UPROPERTY(Replicated, BlueprintReadOnly, Category="Maze")
    int32 MazeWidth = 0;

    UPROPERTY(Replicated, BlueprintReadOnly, Category="Maze")
    int32 MazeHeight = 0;

    UPROPERTY(EditDefaultsOnly, Category="Maze|Gameplay")
    TSubclassOf<AActor> WallClass;

    UPROPERTY(EditDefaultsOnly, Category="Maze|Gameplay")
    float CellSize = 500.f;

    UPROPERTY(EditDefaultsOnly, Category="Maze|UI")
    TSubclassOf<UMazeCountdownWidget> CountdownWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category="Maze|UI")
    TSubclassOf<UCommonModalWidget> ResultModalClass;

    UPROPERTY(EditDefaultsOnly, Category="Maze|Audio")
    TObjectPtr<USoundBase> BGMSound;

    UPROPERTY(EditDefaultsOnly, Category="Maze|Audio")
    TObjectPtr<USoundBase> WinSound;

    UPROPERTY(EditDefaultsOnly, Category="Maze|Audio")
    TObjectPtr<USoundBase> LoseSound;

private:
    UFUNCTION()
    void OnRep_Phase();

    UFUNCTION()
    void OnRep_MatchResult();

    UFUNCTION()
    void OnRep_MazeSeed();

    UPROPERTY()
    TObjectPtr<UMazeCountdownWidget> CountdownWidgetInstance;

    UPROPERTY()
    TObjectPtr<UCommonModalWidget> ResultWidgetInstance;

    UPROPERTY()
    TObjectPtr<UAudioComponent> BGMComponent;
};
