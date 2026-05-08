// Fill out your copyright notice in the Description page of Project Settings.

#include "MazeActor.h"

#include "MazeTargetPoint.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Math/RandomStream.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"

#if WITH_EDITOR
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#endif

#define LOCTEXT_NAMESPACE "MazeActor"

AMazeActor::AMazeActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
}

void AMazeActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

#if WITH_EDITOR
	UWorld* World = GetWorld();
	if (!World) return;

	if (World->WorldType != EWorldType::Editor && World->WorldType != EWorldType::EditorPreview)
	{
		return;
	}

	EnforceTransformConstraints();

	FlushPersistentDebugLines(World);
	DrawPreview();
#endif
}

void AMazeActor::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* World = GetWorld())
	{
		FlushPersistentDebugLines(World);
	}
}

#if WITH_EDITOR
void AMazeActor::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
	Super::PostEditChangeProperty(Event);

	EnforceTransformConstraints();

	if (UWorld* World = GetWorld();
		World && (World->WorldType == EWorldType::Editor || World->WorldType == EWorldType::EditorPreview))
	{
		FlushPersistentDebugLines(World);
		DrawPreview();
	}
}

void AMazeActor::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);

	EnforceTransformConstraints();

	if (bFinished)
	{
		if (UWorld* World = GetWorld();
			World && (World->WorldType == EWorldType::Editor || World->WorldType == EWorldType::EditorPreview))
		{
			FlushPersistentDebugLines(World);
			DrawPreview();
		}
	}
}

void AMazeActor::EnforceTransformConstraints()
{
	// 재진입 방지: SetActorRotation/SetActorScale3D 자체가 PostEditMove를 재귀 트리거하는 케이스 차단
	// 효과 x 그 이유가 아닌듯 하다.
	// if (bEnforcing) return;
	// TGuardValue<bool> Guard(bEnforcing, true);

	const bool bRotViolation = !GetActorRotation().Equals(FRotator::ZeroRotator);
	const bool bScaleViolation = !GetActorScale3D().Equals(FVector::OneVector);

	if (bRotViolation)
	{
		SetActorRotation(FRotator::ZeroRotator);
	}
	if (bScaleViolation)
	{
		SetActorScale3D(FVector::OneVector);
	}

	if (!bRotViolation && !bScaleViolation) return;

	// Dedupe: 같은 사용자 액션에서 PostEditChangeProperty와 PostEditMove가 모두 호출돼
	// 토스트가 두 번 뜨는 것을 방지 (복원은 위에서 매번 수행)
	// 프로퍼티를 바꾸고 콜백중 값이 바뀌어도 바꾼 콜백으로 다시 적용되는 듯 하다. 
	if (GFrameCounter == LastEnforceFrame) return;
	LastEnforceFrame = GFrameCounter;

	const FText ToastText = bRotViolation
		? FText::FromString(TEXT("MazeActor: 회전은 지원되지 않습니다. (월드축 기반 봇 패트롤 호환)"))
		: FText::FromString(TEXT("MazeActor: 스케일은 지원되지 않습니다. (CellSize 사용)"));
	
	FNotificationInfo Info(ToastText);
	Info.ExpireDuration = 3.f;
	FSlateNotificationManager::Get().AddNotification(Info);
}

void AMazeActor::DrawPreview()
{
	UWorld* World = GetWorld();
	if (!World) return;

	const FTransform XForm = GetActorTransform();

	const float Width = PreviewWidth * CellSize;
	const float Height = PreviewHeight * CellSize;

	const FVector LocalMin = FVector::ZeroVector;
	const FVector LocalMax = FVector(Width, Height, 0.f);
	const FVector LocalCenter = (LocalMin + LocalMax) * 0.5f;
	const FVector LocalExtent = (LocalMax - LocalMin) * 0.5f;

	const FVector WorldCenter = XForm.TransformPosition(LocalCenter);

	DrawDebugBox(World, WorldCenter, FVector(LocalExtent.X, LocalExtent.Y, 50.f),
		FColor::Yellow, true, -1.f, 0, 4.f);

	for (int32 r = 0; r < PreviewHeight; ++r)
	{
		for (int32 c = 0; c < PreviewWidth; ++c)
		{
			const FVector CellLocal = LocalCellCenter(r, c, 0.f);
			const FVector CellWorld = XForm.TransformPosition(CellLocal);
			DrawDebugBox(World, CellWorld,
				FVector(CellSize * 0.5f, CellSize * 0.5f, CellSize * 0.5f),
				FColor::Green, true, -1.f, 0, 4.f);
		}
	}
}
#endif // WITH_EDITOR

FVector AMazeActor::GetCellCenter(int32 Row, int32 Col, float Z) const
{
	return GetActorTransform().TransformPosition(LocalCellCenter(Row, Col, Z));
}

FVector AMazeActor::GetVerticalBoundaryCenter(int32 Row, int32 BoundaryCol, float Z) const
{
	return GetActorTransform().TransformPosition(LocalVerticalBoundary(Row, BoundaryCol, Z));
}

FVector AMazeActor::GetHorizontalBoundaryCenter(int32 BoundaryRow, int32 Col, float Z) const
{
	return GetActorTransform().TransformPosition(LocalHorizontalBoundary(BoundaryRow, Col, Z));
}

TArray<FWallSpawnInfo> AMazeActor::CollectWallSpawnData(
	const TArray<FCellRow>& Grid, int32 Height, int32 Width) const
{
	TArray<FWallSpawnInfo> Result;
	const FTransform XForm = GetActorTransform();

	for (int32 r = 0; r < Height; ++r)
	{
		for (int32 c = 0; c < Width; ++c)
		{
			if (r == 0)
			{
				Result.Add({
					XForm.TransformPosition(LocalHorizontalBoundary(0, c, 0.f)),
					FRotator(0.f, 0.f, 0.f) });
			}

			if (c == 0)
			{
				Result.Add({
					XForm.TransformPosition(LocalVerticalBoundary(r, 0, 0.f)),
					FRotator(0.f, 90.f, 0.f) });
			}

			if (c == Width - 1)
			{
				Result.Add({
					XForm.TransformPosition(LocalVerticalBoundary(r, Width, 0.f)),
					FRotator(0.f, 90.f, 0.f) });
			}
			else if (Grid[r].Cells[c].RightWall)
			{
				Result.Add({
					XForm.TransformPosition(LocalVerticalBoundary(r, c + 1, 0.f)),
					FRotator(0.f, 90.f, 0.f) });
			}

			if (r == Height - 1)
			{
				Result.Add({
					XForm.TransformPosition(LocalHorizontalBoundary(Height, c, 0.f)),
					FRotator(0.f, 0.f, 0.f) });
			}
			else if (Grid[r].Cells[c].DownWall)
			{
				Result.Add({
					XForm.TransformPosition(LocalHorizontalBoundary(r + 1, c, 0.f)),
					FRotator(0.f, 0.f, 0.f) });
			}
		}
	}

	return Result;
}

int32 AMazeActor::SpawnWalls(const TArray<FCellRow>& Grid, int32 Height, int32 Width)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("MazeActor::SpawnWalls: World is null"));
		return 0;
	}

	if (!WallClass)
	{
		UE_LOG(LogTemp, Error, TEXT("MazeActor::SpawnWalls: WallClass is null"));
		return 0;
	}

	const TArray<FWallSpawnInfo> WallData = CollectWallSpawnData(Grid, Height, Width);

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Owner = this;

	for (const FWallSpawnInfo& Info : WallData)
	{
		World->SpawnActor<AActor>(WallClass, Info.Position, Info.Rotation, SpawnParams);
	}

	UE_LOG(LogTemp, Log, TEXT("MazeActor: Spawned %d walls"), WallData.Num());
	return WallData.Num();
}

void AMazeActor::SpawnWallsWithDelay(
	const TArray<FCellRow>& Grid, int32 Height, int32 Width,
	FSimpleDelegate OnComplete)
{
	UWorld* World = GetWorld();

	if (!World || !WallClass)
	{
		OnComplete.ExecuteIfBound();
		return;
	}

	TArray<FWallSpawnInfo> WallData = CollectWallSpawnData(Grid, Height, Width);
	const int32 Total = WallData.Num();

	if (Total == 0)
	{
		OnComplete.ExecuteIfBound();
		return;
	}

	struct FWallSpawnState
	{
		TArray<FWallSpawnInfo> WallData;
		TSubclassOf<AActor>    WallClass;
		TWeakObjectPtr<UWorld> World;
		TWeakObjectPtr<AActor> Owner;
		FSimpleDelegate        OnComplete;
	};

	TSharedPtr<FWallSpawnState> State = MakeShared<FWallSpawnState>();
	State->WallData   = MoveTemp(WallData);
	State->WallClass  = WallClass;
	State->World      = World;
	State->Owner      = this;
	State->OnComplete = MoveTemp(OnComplete);

	const float Interval = WallSpawnInterval;

	for (int32 i = 0; i < Total; ++i)
	{
		const bool bLast = (i == Total - 1);
		const float Delay = i * Interval;

		FTimerHandle Handle;
		World->GetTimerManager().SetTimer(
			Handle,
			[State, i, bLast]()
			{
				if (!State->World.IsValid()) return;

				FActorSpawnParameters SpawnParams;
				SpawnParams.SpawnCollisionHandlingOverride =
					ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				SpawnParams.Owner = State->Owner.Get();

				State->World->SpawnActor<AActor>(
					State->WallClass,
					State->WallData[i].Position,
					State->WallData[i].Rotation,
					SpawnParams);

				if (bLast)
				{
					State->OnComplete.ExecuteIfBound();
				}
			},
			FMath::Max(Delay, KINDA_SMALL_NUMBER),
			false
		);
	}
}

void AMazeActor::SpawnGameplayActors(
	const TArray<FCellRow>& Grid, int32 Height, int32 Width,
	int32 PlayerNum, TSubclassOf<AActor> GoalActorClass,
	TSubclassOf<APawn> BotClass, int32 BotCount)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("MazeActor::SpawnGameplayActors: World is null"));
		return;
	}

	if (!GoalActorClass)
	{
		UE_LOG(LogTemp, Error, TEXT("MazeActor::SpawnGameplayActors: GoalActorClass is null"));
		return;
	}

	(void)Grid;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Owner = this;

	{
		const int32 gr = Height / 2;
		const int32 gc = Width / 2;
		const FVector Pos = GetCellCenter(gr, gc, 0.f);
		World->SpawnActor<AActor>(GoalActorClass, Pos, FRotator::ZeroRotator, SpawnParams);
		UE_LOG(LogTemp, Log, TEXT("MazeActor: Goal spawned at %s"), *Pos.ToString());
	}

	const TArray<TPair<int32, int32>> PlayerStartNodes{
		{0, 0}, {0, Width - 1}, {Height - 1, 0}, {Height - 1, Width - 1}};
	for (int32 i = 0; i < PlayerNum; ++i)
	{
		auto [pr, pc] = PlayerStartNodes[i];
		const FVector Pos = GetCellCenter(pr, pc, 0.f);
		AMazeTargetPoint* TP = World->SpawnActor<AMazeTargetPoint>(
			AMazeTargetPoint::StaticClass(), Pos, FRotator::ZeroRotator, SpawnParams);
		if (TP)
		{
			TP->PlayerIndex = i;
		}
		UE_LOG(LogTemp, Log, TEXT("MazeActor: MazeTargetPoint[%d] spawned at %s"), i, *Pos.ToString());
	}

	if (BotClass && BotCount > 0)
	{
		const TArray<TPair<int32, int32>> BotStartNodes = {
			{Height / 4,              Width / 4},
			{Height - 1 - Height / 4, Width / 4},
			{Height / 4,              Width - 1 - Width / 4},
			{Height - 1 - Height / 4, Width - 1 - Width / 4},
		};

		const int32 ActualBotCount = FMath::Min(BotCount, BotStartNodes.Num());
		for (int32 i = 0; i < ActualBotCount; ++i)
		{
			auto [br, bc] = BotStartNodes[i];
			const FVector Pos = GetCellCenter(br, bc, 0.f);
			World->SpawnActor<APawn>(BotClass, Pos, FRotator::ZeroRotator, SpawnParams);
			UE_LOG(LogTemp, Log, TEXT("MazeActor: Bot[%d] spawned at %s"), i, *Pos.ToString());
		}
	}
}

void AMazeActor::BuildMazeGrid(int32 Height, int32 Width, int32 Seed, TArray<FCellRow>& Grid)
{
	const int32 NodeNum = Height * Width;

	TArray<int32> UF;
	UF.Init(-1, NodeNum);

	TArray<TPair<int32, int32>> Edges;
	Edges.Reserve((Height - 1) * Width + Height * (Width - 1));

	for (int32 i = 0; i < Height - 1; ++i)
		for (int32 j = 0; j < Width; ++j)
		{
			const int32 u = i * Width + j;
			const int32 v = (i + 1) * Width + j;
			Edges.Emplace(u, v);
		}

	for (int32 i = 0; i < Height; ++i)
		for (int32 j = 0; j < Width - 1; ++j)
		{
			const int32 u = i * Width + j;
			const int32 v = i * Width + (j + 1);
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
				Grid[Row].Cells[Col].RightWall = false;
			else
				Grid[Row].Cells[Col].DownWall = false;
			Count++;
		}
	}
}

int32 AMazeActor::FindRoot(int32 u, TArray<int32>& UF)
{
	if (UF[u] < 0) return u;
	return UF[u] = FindRoot(UF[u], UF);
}

bool AMazeActor::UnionSet(int32 u, int32 v, TArray<int32>& UF)
{
	u = FindRoot(u, UF);
	v = FindRoot(v, UF);

	if (u == v)
		return false;

	if (UF[u] < UF[v])
		std::swap(u, v);

	UF[v] += UF[u];
	UF[u] = v;
	return true;
}

#undef LOCTEXT_NAMESPACE
