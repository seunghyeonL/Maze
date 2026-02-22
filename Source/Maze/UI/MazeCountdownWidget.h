#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MazeCountdownWidget.generated.h"

class UTextBlock;
class AMazeGameState;

UCLASS()
class MAZE_API UMazeCountdownWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> CountdownText;
};
