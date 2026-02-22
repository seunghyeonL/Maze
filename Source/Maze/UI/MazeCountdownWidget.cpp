#include "MazeCountdownWidget.h"
#include "GameState/MazeGameState.h"
#include "Components/TextBlock.h"

void UMazeCountdownWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UMazeCountdownWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (AMazeGameState* GS = GetWorld()->GetGameState<AMazeGameState>())
	{
		const float TimeRemaining = GS->CountdownEndTime - GetWorld()->GetTimeSeconds();
		const int32 Remaining = FMath::Max(0, FMath::CeilToInt(TimeRemaining));

		if (CountdownText)
		{
			if (Remaining > 0)
			{
				CountdownText->SetText(FText::AsNumber(Remaining));
			}
			else
			{
				CountdownText->SetText(FText::FromString(TEXT("GO!")));
			}
		}
	}
}
