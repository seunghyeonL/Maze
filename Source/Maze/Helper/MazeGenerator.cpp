// Fill out your copyright notice in the Description page of Project Settings.

#include "MazeGenerator.h"

#include "Algo/RandomShuffle.h"
#include "GameFramework/PlayerStart.h"

void UMazeGenerator::GenerateMaze(
	UObject* WorldContextObject,
	int32 Width,
	int32 Height,
	int32 PlayerNum,
	float CellSize,
	TSubclassOf<AActor> WallClass,
	TSubclassOf<AActor> FloorClass,
	TSubclassOf<AActor> GoalActorClass
	)
{
	if (!WorldContextObject)
    {
        UE_LOG(LogTemp, Error, TEXT("GenerateMaze: WorldContextObject is null"));
        return;
    }

    UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("GenerateMaze: Failed to get World"));
        return;
    }

    if (!WallClass)
    {
        UE_LOG(LogTemp, Error, TEXT("GenerateMaze: WallClass is null"));
        return;
    }

    if (!FloorClass)
    {
        UE_LOG(LogTemp, Error, TEXT("GenerateMaze: FloorClass is null"));
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

    const int32 NodeNum = Width * Height;

    // Goal 1개 + PlayerNum개를 뽑아야 함
    if (PlayerNum < 1 || PlayerNum + 1 > NodeNum)
    {
        UE_LOG(LogTemp, Error, TEXT("GenerateMaze: Invalid PlayerNum. Need PlayerNum>=1 and PlayerNum+1 <= Width*Height"));
        return;
    }

    // 인접행렬 생성 (0으로 초기화)
    TArray<TArray<int32>> AdjMat;
    AdjMat.SetNum(NodeNum);
    for (auto& Row : AdjMat)
    {
        Row.Init(0, NodeNum);
    }

    // (주의) BuildAdjMat(N, M)에서 N=Height, M=Width로 통일
    BuildAdjMat(Height, Width, AdjMat);

    // 랜덤 노드 뽑기
    TArray<int32> Nodes;
    Nodes.Reserve(NodeNum);
    for (int32 i = 0; i < NodeNum; ++i)
    {
        Nodes.Emplace(i);
    }
    Algo::RandomShuffle(Nodes);

    const int32 GoalNode = Nodes[0];

    TArray<int32> PlayerStartNodes;
    PlayerStartNodes.Reserve(PlayerNum);
    for (int32 i = 1; i <= PlayerNum; ++i)
    {
        PlayerStartNodes.Emplace(Nodes[i]);
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    // ---- 1) 바닥 스폰 (각 셀마다) ----
    // FloorClass가 "한 장으로 전체를 덮는" 메쉬면 여기 루프 대신 한 번만 스폰하고 스케일 조절하는 게 더 좋음.
    for (int32 r = 0; r < Height; ++r)
    {
        for (int32 c = 0; c < Width; ++c)
        {
            const FVector Pos = CellCenter(r, c, CellSize, 0.f);
            World->SpawnActor<AActor>(FloorClass, Pos, FRotator::ZeroRotator, SpawnParams);
        }
    }

    // ---- 2) 벽 스폰 ----
    // 벽 메시 기본 방향 가정:
    // - "가로 벽"(동서로 길게): Yaw = 0
    // - "세로 벽"(남북으로 길게): Yaw = 90
    //
    // 만약 네 WallClass가 기본으로 다른 방향을 바라보면 Yaw 0/90을 바꿔주면 됨.

    auto SpawnWall = [&](const FVector& Pos, const FRotator& Rot)
    {
        World->SpawnActor<AActor>(WallClass, Pos, Rot, SpawnParams);
    };

    // 외곽(테두리) + 내부 벽:
    for (int32 r = 0; r < Height; ++r)
    {
        for (int32 c = 0; c < Width; ++c)
        {
            const int32 u = ToNode(r, c, Width);

            // (A) 북쪽 외곽 (r == 0): BoundaryRow = 0
            if (r == 0)
            {
                const FVector Pos = HorizontalBoundaryCenter(0, c, CellSize, 0.f);
                SpawnWall(Pos, FRotator(0.f, 0.f, 0.f));
            }

            // (B) 서쪽 외곽 (c == 0): BoundaryCol = 0
            if (c == 0)
            {
                const FVector Pos = VerticalBoundaryCenter(r, 0, CellSize, 0.f);
                SpawnWall(Pos, FRotator(0.f, 90.f, 0.f));
            }

            // (C) 동쪽 경계: 내부면 (c < Width-1) 체크 후, 길이 없으면 벽 / 외곽이면 무조건 벽
            if (c == Width - 1)
            {
                // 동쪽 외곽: BoundaryCol = Width
                const FVector Pos = VerticalBoundaryCenter(r, Width, CellSize, 0.f);
                SpawnWall(Pos, FRotator(0.f, 90.f, 0.f));
            }
            else
            {
                const int32 v = ToNode(r, c + 1, Width);
                if (AdjMat[u][v] == 0)
                {
                    const FVector Pos = VerticalBoundaryCenter(r, c + 1, CellSize, 0.f);
                    SpawnWall(Pos, FRotator(0.f, 90.f, 0.f));
                }
            }

            // (D) 남쪽 경계: 내부면 (r < Height-1) 체크 후, 길이 없으면 벽 / 외곽이면 무조건 벽
            if (r == Height - 1)
            {
                // 남쪽 외곽: BoundaryRow = Height
                const FVector Pos = HorizontalBoundaryCenter(Height, c, CellSize, 0.f);
                SpawnWall(Pos, FRotator(0.f, 0.f, 0.f));
            }
            else
            {
                const int32 v = ToNode(r + 1, c, Width);
                if (AdjMat[u][v] == 0)
                {
                    const FVector Pos = HorizontalBoundaryCenter(r + 1, c, CellSize, 0.f);
                    SpawnWall(Pos, FRotator(0.f, 0.f, 0.f));
                }
            }
        }
    }

    // ---- 3) Goal 스폰 ----
    {
        int32 gr, gc;
        ToRC(GoalNode, Width, gr, gc);

        const FVector Pos = CellCenter(gr, gc, CellSize, 0.f);
        World->SpawnActor<AActor>(GoalActorClass, Pos, FRotator::ZeroRotator, SpawnParams);
    }

    // ---- 4) PlayerStart 스폰 ----
    // 여기선 기본 엔진의 APlayerStart를 사용. (원하면 TSubclassOf<APlayerStart>를 파라미터로 받는 게 더 좋음)
    for (int32 i = 0; i < PlayerStartNodes.Num(); ++i)
    {
        int32 pr, pc;
        ToRC(PlayerStartNodes[i], Width, pr, pc);

        const FVector Pos = CellCenter(pr, pc, CellSize, 0.f);
        World->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), Pos, FRotator::ZeroRotator, SpawnParams);
    }
}

void UMazeGenerator::BuildAdjMat(int32 N, int32 M, TArray<TArray<int32>>& AdjMat)
{
	int32 NodeNum = AdjMat.Num();
	
	// Random Kruskal
	TArray<int32> UF;
	UF.Init(-1, NodeNum);
	
	TArray<TPair<int32, int32>> Edges;
	Edges.Reserve((N - 1) * M + N * (M - 1));

	// 세로 간선: (i,j) - (i+1,j)
	for (int32 i = 0; i < N - 1; ++i)
		for (int32 j = 0; j < M; ++j)
		{
			int32 u = i * M + j;
			int32 v = (i + 1) * M + j;
			Edges.Emplace(u, v);
		}

	// 가로 간선: (i,j) - (i,j+1)
	for (int32 i = 0; i < N; ++i)
		for (int32 j = 0; j < M - 1; ++j)
		{
			int32 u = i * M + j;
			int32 v = i * M + (j + 1);
			Edges.Emplace(u, v);
		}
	
	Algo::RandomShuffle(Edges);
	
	int32 Count = 0;
	for (auto [u, v] : Edges)
	{
		if (Count == NodeNum - 1) 
			break;
		
		if (UnionSet(u, v, UF))
		{
			AdjMat[u][v] = 1;
			AdjMat[v][u] = 1;
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
