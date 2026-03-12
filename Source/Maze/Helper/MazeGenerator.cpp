// Fill out your copyright notice in the Description page of Project Settings.

#include "MazeGenerator.h"

#include "GameFramework/Pawn.h"
#include "Math/RandomStream.h"
#include "../Actor/MazeTargetPoint.h"

void UMazeGenerator::GenerateMaze(
	UObject* WorldContextObject,
	int32 Width,
	int32 Height,
	int32 PlayerNum,
	float CellSize,
	TSubclassOf<AActor> WallClass,
	TSubclassOf<AActor> GoalActorClass,
	TSubclassOf<APawn> BotClass,
	int32 BotCount
	)
{
	if (!WorldContextObject)
	{
		UE_LOG(LogTemp, Error, TEXT("GenerateMaze: WorldContextObject is null"));
		return;
	}

	if (!WallClass)
	{
		UE_LOG(LogTemp, Error, TEXT("GenerateMaze: WallClass is null"));
		return;
	}

	if (!GoalActorClass)
	{
		UE_LOG(LogTemp, Error, TEXT("GenerateMaze: GoalActorClass is null"));
		return;
	}

	if (Width < 2 || Height < 2)
	{
		UE_LOG(LogTemp, Error, TEXT("GenerateMaze: Width and Height must be at least 2"));
		return;
	}

	if (PlayerNum < 1 || PlayerNum > 4)
	{
		UE_LOG(LogTemp, Error, TEXT("GenerateMaze: PlayerNum(%d) must be 1~4"), PlayerNum);
		return;
	}

	TArray<FCellRow> Grid;
	Grid.SetNum(Height);
	for (auto& Row : Grid)
	{
		Row.Cells.SetNum(Width);
	}

	const int32 Seed = FMath::Rand();
	BuildMazeGrid(Height, Width, Seed, Grid);
	SpawnWalls(WorldContextObject, Grid, Height, Width, CellSize, WallClass);
	SpawnGameplayActors(WorldContextObject, Grid, Height, Width, CellSize, PlayerNum, GoalActorClass, BotClass, BotCount);
}

int32 UMazeGenerator::SpawnWalls(
	UObject* WorldContextObject,
	const TArray<FCellRow>& Grid,
	int32 Height,
	int32 Width,
	float CellSize,
	TSubclassOf<AActor> WallClass)
{
	if (!WorldContextObject)
	{
		UE_LOG(LogTemp, Error, TEXT("SpawnWalls: WorldContextObject is null"));
		return 0;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("SpawnWalls: Failed to get World"));
		return 0;
	}

	if (!WallClass)
	{
		UE_LOG(LogTemp, Error, TEXT("SpawnWalls: WallClass is null"));
		return 0;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.bCreateActorPackage = false;
	
	int32 SpawnedWallCount = 0;
	auto SpawnWall = [&](const FVector& Pos, const FRotator& Rot)
	{
		World->SpawnActor<AActor>(WallClass, Pos, Rot, SpawnParams);
		++SpawnedWallCount;
	};

	for (int32 r = 0; r < Height; ++r)
	{
		for (int32 c = 0; c < Width; ++c)
		{
			if (r == 0)
			{
				const FVector Pos = HorizontalBoundaryCenter(0, c, CellSize, 0.f);
				SpawnWall(Pos, FRotator(0.f, 0.f, 0.f));
			}

			if (c == 0)
			{
				const FVector Pos = VerticalBoundaryCenter(r, 0, CellSize, 0.f);
				SpawnWall(Pos, FRotator(0.f, 90.f, 0.f));
			}

			if (c == Width - 1)
			{
				const FVector Pos = VerticalBoundaryCenter(r, Width, CellSize, 0.f);
				SpawnWall(Pos, FRotator(0.f, 90.f, 0.f));
			}
			else if (Grid[r].Cells[c].RightWall)
			{
				const FVector Pos = VerticalBoundaryCenter(r, c + 1, CellSize, 0.f);
				SpawnWall(Pos, FRotator(0.f, 90.f, 0.f));
			}

			if (r == Height - 1)
			{
				const FVector Pos = HorizontalBoundaryCenter(Height, c, CellSize, 0.f);
				SpawnWall(Pos, FRotator(0.f, 0.f, 0.f));
			}
			else if (Grid[r].Cells[c].DownWall)
			{
				const FVector Pos = HorizontalBoundaryCenter(r + 1, c, CellSize, 0.f);
				SpawnWall(Pos, FRotator(0.f, 0.f, 0.f));
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("MazeGenerator: Spawned %d walls"), SpawnedWallCount);
	return SpawnedWallCount;
}

void UMazeGenerator::SpawnGameplayActors(
	UObject* WorldContextObject,
	const TArray<FCellRow>& Grid,
	int32 Height,
	int32 Width,
	float CellSize,
	int32 PlayerNum,
	TSubclassOf<AActor> GoalActorClass,
	TSubclassOf<APawn> BotClass,
	int32 BotCount)
{
	if (!WorldContextObject)
	{
		UE_LOG(LogTemp, Error, TEXT("SpawnGameplayActors: WorldContextObject is null"));
		return;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("SpawnGameplayActors: Failed to get World"));
		return;
	}

	if (!GoalActorClass)
	{
		UE_LOG(LogTemp, Error, TEXT("SpawnGameplayActors: GoalActorClass is null"));
		return;
	}

	(void)Grid;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	{
		int32 gr = Height / 2;
		int32 gc = Width / 2;
		const FVector Pos = CellCenter(gr, gc, CellSize, 0.f);
		World->SpawnActor<AActor>(GoalActorClass, Pos, FRotator::ZeroRotator, SpawnParams);
		UE_LOG(LogTemp, Log, TEXT("MazeGenerator: Goal spawned at %s"), *Pos.ToString());
	}

	TArray<TPair<int32, int32>> PlayerStartNodes{{0, 0}, {0, Width - 1}, {Height - 1, 0}, {Height - 1, Width - 1}};
	for (int32 i = 0; i < PlayerNum; ++i)
	{
		auto [pr, pc] = PlayerStartNodes[i];
		const FVector Pos = CellCenter(pr, pc, CellSize, 0.f);
		AMazeTargetPoint* TP = World->SpawnActor<AMazeTargetPoint>(AMazeTargetPoint::StaticClass(), Pos, FRotator::ZeroRotator, SpawnParams);
		if (TP)
		{
			TP->PlayerIndex = i;
		}
		UE_LOG(LogTemp, Log, TEXT("MazeGenerator: MazeTargetPoint[%d] spawned at %s"), i, *Pos.ToString());
	}

	if (BotClass && BotCount > 0)
	{
		TArray<TPair<int32, int32>> BotStartNodes = {
			{Height / 4,              Width / 4},
			{Height - 1 - Height / 4, Width / 4},
			{Height / 4,              Width - 1 - Width / 4},
			{Height - 1 - Height / 4, Width - 1 - Width / 4},
		};

		const int32 ActualBotCount = FMath::Min(BotCount, BotStartNodes.Num());
		for (int32 i = 0; i < ActualBotCount; ++i)
		{
			auto [br, bc] = BotStartNodes[i];
			const FVector Pos = CellCenter(br, bc, CellSize, 0.f);
			World->SpawnActor<APawn>(BotClass, Pos, FRotator::ZeroRotator, SpawnParams);
			UE_LOG(LogTemp, Log, TEXT("MazeGenerator: Bot[%d] spawned at %s"), i, *Pos.ToString());
		}
	}
}

void UMazeGenerator::BuildMazeGrid(int32 Height, int32 Width, int32 Seed, TArray<FCellRow>& Grid)
{
	const int32 NodeNum = Height * Width;

	// Random Kruskal
	TArray<int32> UF;
	UF.Init(-1, NodeNum);

	TArray<TPair<int32, int32>> Edges;
	Edges.Reserve((Height - 1) * Width + Height * (Width - 1));

	// 세로 간선: (i,j) - (i+1,j)  →  DownWall
	for (int32 i = 0; i < Height - 1; ++i)
		for (int32 j = 0; j < Width; ++j)
		{
			int32 u = i * Width + j;
			int32 v = (i + 1) * Width + j;
			Edges.Emplace(u, v);
		}

	// 가로 간선: (i,j) - (i,j+1)  →  RightWall
	for (int32 i = 0; i < Height; ++i)
		for (int32 j = 0; j < Width - 1; ++j)
		{
			int32 u = i * Width + j;
			int32 v = i * Width + (j + 1);
			Edges.Emplace(u, v);
		}

	FRandomStream Rng(Seed);
	for (int32 i = Edges.Num() - 1; i > 0; --i)
	{
		const int32 j = Rng.RandHelper(i + 1);
		Edges.Swap(i, j);
	}

	int32 Count = 0;
	for (auto [u, v] : Edges)
	{
		if (Count == NodeNum - 1)
			break;

		if (UnionSet(u, v, UF))
		{
			const int32 Row = u / Width;
			const int32 Col = u % Width;
			if (v - u == 1)
				Grid[Row].Cells[Col].RightWall = false;  // 가로 통로: 오른쪽 벽 제거
			else
				Grid[Row].Cells[Col].DownWall = false;   // 세로 통로: 아래 벽 제거
			Count++;
		}
	}
}

int32 UMazeGenerator::FindRoot(int32 u, TArray<int32>& UF)
{
	if (UF[u] < 0) return u;
	return UF[u] = FindRoot(UF[u], UF);
}

bool UMazeGenerator::UnionSet(int32 u, int32 v, TArray<int32>& UF)
{
	u = FindRoot(u, UF);
	v = FindRoot(v, UF);
		
	if (u == v)
		return false;
		
	if (UF[u] < UF[v])
		std::swap(u, v);
		
	// u -> v
	UF[v] += UF[u];
	UF[u] = v;
	return true;
}
