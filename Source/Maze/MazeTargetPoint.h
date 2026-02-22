#pragma once
#include "CoreMinimal.h"
#include "Engine/TargetPoint.h"
#include "MazeTargetPoint.generated.h"

UCLASS()
class MAZE_API AMazeTargetPoint : public ATargetPoint
{
    GENERATED_BODY()

public:
    AMazeTargetPoint();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Maze")
    int32 PlayerIndex = -1;
};
