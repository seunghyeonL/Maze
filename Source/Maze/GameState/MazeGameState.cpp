#include "MazeGameState.h"
#include "UI/MazeCountdownWidget.h"
#include "UI/CommonModalWidget.h"
#include "Net/UnrealNetwork.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerController.h"

void AMazeGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AMazeGameState, Phase);
    DOREPLIFETIME(AMazeGameState, WinnerPlayer);
    DOREPLIFETIME(AMazeGameState, CountdownEndTime);
}

void AMazeGameState::OnRep_Phase()
{
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
    else if (Phase == EMazePhase::Playing || Phase == EMazePhase::GameOver)
    {
        if (CountdownWidgetInstance)
        {
            CountdownWidgetInstance->RemoveFromParent();
            CountdownWidgetInstance = nullptr;
        }
    }
}

void AMazeGameState::OnRep_MatchResult()
{
    if (!WinnerPlayer) return;

    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC || !ResultModalClass) return;

    const bool bIsWinner = (WinnerPlayer == PC->PlayerState);
    const FText Title = bIsWinner ? FText::FromString(TEXT("승리!")) : FText::FromString(TEXT("패배"));
    const FText Message = bIsWinner
        ? FText::FromString(TEXT("먼저 목표에 도달했습니다!"))
        : FText::FromString(TEXT("다음에 도전해보세요."));

    ResultWidgetInstance = CreateWidget<UCommonModalWidget>(PC, ResultModalClass);
    if (ResultWidgetInstance)
    {
        ResultWidgetInstance->ShowAlert(Title, Message);
        ResultWidgetInstance->AddToViewport();
    }
}
