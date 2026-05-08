// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Delegates/DelegateCombinations.h"
#include "MazeActor.generated.h"

USTRUCT(BlueprintType)
struct FCell
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	bool RightWall;

	UPROPERTY(BlueprintReadWrite)
	bool DownWall;

	UPROPERTY(BlueprintReadWrite, Category = "Maze")
	int32 PlayerStartNum;

	UPROPERTY(BlueprintReadWrite, Category = "Maze")
	bool IsGoal;

	FCell() : RightWall(true), DownWall(true), PlayerStartNum(0), IsGoal(false) {}
};

USTRUCT(BlueprintType)
struct FCellRow
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Maze")
	TArray<FCell> Cells;

	FCellRow() {}
};

/** 벽 스폰 위치/회전 정보 */
struct FWallSpawnInfo
{
	FVector  Position;
	FRotator Rotation;
};

UCLASS()
class MAZE_API AMazeActor : public AActor
{
	GENERATED_BODY()

public:
	AMazeActor();

	// === 순수 격자 빌더 (서버/클라 양쪽에서 호출) ===
	static void BuildMazeGrid(int32 Height, int32 Width, int32 Seed, TArray<FCellRow>& Grid);

	// === 좌표 헬퍼 (Transform 적용된 월드 좌표 반환) ===
	FVector GetCellCenter(int32 Row, int32 Col, float Z = 0.f) const;
	FVector GetVerticalBoundaryCenter(int32 Row, int32 BoundaryCol, float Z = 0.f) const;
	FVector GetHorizontalBoundaryCenter(int32 BoundaryRow, int32 Col, float Z = 0.f) const;

	// === 스폰 책임 ===
	TArray<FWallSpawnInfo> CollectWallSpawnData(const TArray<FCellRow>& Grid, int32 Height, int32 Width) const;
	int32 SpawnWalls(const TArray<FCellRow>& Grid, int32 Height, int32 Width);
	void SpawnWallsWithDelay(const TArray<FCellRow>& Grid, int32 Height, int32 Width,
	                         FSimpleDelegate OnComplete = FSimpleDelegate());
	void SpawnGameplayActors(const TArray<FCellRow>& Grid, int32 Height, int32 Width,
	                         int32 PlayerNum, TSubclassOf<AActor> GoalActorClass,
	                         TSubclassOf<APawn> BotClass, int32 BotCount);

	// === 게터 ===
	float GetCellSize() const { return CellSize; }
	TSubclassOf<AActor> GetWallClass() const { return WallClass; }
	float GetWallSpawnInterval() const { return WallSpawnInterval; }

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& Event) override;
	virtual void PostEditMove(bool bFinished) override;
#endif

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Maze|Config")
	TSubclassOf<AActor> WallClass;

	UPROPERTY(EditDefaultsOnly, Category = "Maze|Config")
	float CellSize = 500.f;

	UPROPERTY(EditDefaultsOnly, Category = "Maze|Config")
	float WallSpawnInterval = 0.02f;

	UPROPERTY(EditAnywhere, Category = "Maze|Preview",
		meta = (ClampMin = 2, ClampMax = 20,
				ToolTip = "에디터 프리뷰 전용. 실제 게임 사이즈는 MazeSize URL 옵션으로 결정됩니다."))
	int32 PreviewWidth = 9;

	UPROPERTY(EditAnywhere, Category = "Maze|Preview",
		meta = (ClampMin = 2, ClampMax = 20,
				ToolTip = "에디터 프리뷰 전용. 실제 게임 사이즈는 MazeSize URL 옵션으로 결정됩니다."))
	int32 PreviewHeight = 9;

private:
	FORCEINLINE FVector LocalCellCenter(int32 Row, int32 Col, float Z = 0.f) const
	{
		return FVector((Col + 0.5f) * CellSize, (Row + 0.5f) * CellSize, Z);
	}
	FORCEINLINE FVector LocalVerticalBoundary(int32 Row, int32 BoundaryCol, float Z = 0.f) const
	{
		return FVector(BoundaryCol * CellSize, (Row + 0.5f) * CellSize, Z);
	}
	FORCEINLINE FVector LocalHorizontalBoundary(int32 BoundaryRow, int32 Col, float Z = 0.f) const
	{
		return FVector((Col + 0.5f) * CellSize, BoundaryRow * CellSize, Z);
	}

	static int32 FindRoot(int32 u, TArray<int32>& UF);
	static bool UnionSet(int32 u, int32 v, TArray<int32>& UF);

#if WITH_EDITOR
	void EnforceTransformConstraints();
	void DrawPreview();

	uint64 LastEnforceFrame = (uint64)-1;
	bool bEnforcing = false;
#endif
};
