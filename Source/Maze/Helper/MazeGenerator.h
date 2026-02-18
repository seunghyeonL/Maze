// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MazeGenerator.generated.h"

/**
 * 
 */

USTRUCT(BlueprintType)
struct FCell
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite)
	bool RightWall;
	
	UPROPERTY(BlueprintReadWrite)
	bool DownWall;
	
	UPROPERTY(BlueprintReadWrite, Category = "Maze")
	int32 PlayerStartNum; // 플레이어 시작 지점 (0: 없음, 1~: 플레이어 번호)
	
	UPROPERTY(BlueprintReadWrite, Category = "Maze")
	bool IsGoal;         // 목표 지점 여부
	
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

UCLASS()
class MAZE_API UMazeGenerator : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category="Maze")
	static void GenerateMaze(
		UObject* WorldContextObject,
		int32 Width,
		int32 Height,
		int32 PlayerNum,
		float CellSize,
		TSubclassOf<AActor> WallClass,
		TSubclassOf<AActor> FloorClass,
		TSubclassOf<AActor> GoalActorClass);
	
private:
	static void BuildMazeGrid(int32 Height, int32 Width, TArray<FCellRow>& Grid);
	static int32 FindRoot(int32 u, TArray<int32>& UF);
	static bool UnionSet(int32 u, int32 v, TArray<int32>& UF);
	
	// 셀 (Row, Col) -> NodeIndex
	static FORCEINLINE int32 ToNode(int32 Row, int32 Col, int32 Width)
	{
		return Row * Width + Col;
	}

	// NodeIndex -> (Row, Col)
	static FORCEINLINE void ToRC(int32 Node, int32 Width, int32& OutRow, int32& OutCol)
	{
		OutRow = Node / Width;
		OutCol = Node % Width;
	}

	// 셀 중심 월드 좌표 (미로의 한 꼭지점이 (0,0,0)인 기준)
	static FORCEINLINE FVector CellCenter(int32 Row, int32 Col, float CellSize, float Z = 0.f)
	{
		// X: Col 방향, Y: Row 방향 (UE: X forward, Y right)
		return FVector((Col + 0.5f) * CellSize, (Row + 0.5f) * CellSize, Z);
	}

	// 경계(셀 가장자리) 위치: 예) 셀 (Row, Col)의 "동쪽(+) 경계"는 Col+1 라인
	static FORCEINLINE FVector VerticalBoundaryCenter(int32 Row, int32 BoundaryCol, float CellSize, float Z = 0.f)
	{
		// 세로 벽(남북 방향으로 길게 서는 벽): X가 경계 위치, Y는 셀 중심
		return FVector(BoundaryCol * CellSize, (Row + 0.5f) * CellSize, Z);
	}

	static FORCEINLINE FVector HorizontalBoundaryCenter(int32 BoundaryRow, int32 Col, float CellSize, float Z = 0.f)
	{
		// 가로 벽(동서 방향으로 길게 서는 벽): Y가 경계 위치, X는 셀 중심
		return FVector((Col + 0.5f) * CellSize, BoundaryRow * CellSize, Z);
	}
};
