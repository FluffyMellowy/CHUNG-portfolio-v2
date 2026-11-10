#include "LevelGenerator.h"
#include "Quietus.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "NavigationSystem.h"
#include "TimerManager.h"
#include "Core/RunGameInstance.h"
#include "Generation/GenerationContext.h"
#include "Generation/GenerationDump.h"
#include "Generation/LevelGenPreset.h"
#include "Generation/RoomPrefab.h"
#include "Generation/Construction/DoorwayClearance.h"
#include "Generation/Construction/GeometryBuilder.h"
#include "Generation/Construction/InteriorDoorSpawner.h"
#include "Generation/Spawn/GimmickSpawner.h"
#include "Generation/Construction/LightingBuilder.h"
#include "Generation/Layout/GreyboxLayoutBuilder.h"
#include "Generation/Layout/LevelLayout.h"
#include "Generation/Layout/PrefabLayoutBuilder.h"
#include "Generation/Layout/RoomPrefabCatalog.h"
#include "Generation/Spawn/EnemySpawner.h"
#include "Generation/Spawn/FurnitureSpawner.h"
#include "Generation/Spawn/ObjectiveSpawner.h"
#include "Generation/Spawn/PickupSpawner.h"
#include "Generation/Spawn/SpawnSpotPicker.h"

namespace
{
	UInstancedStaticMeshComponent* CreateGeometryISM(AActor* Owner, const FName Name)
	{
		// 実行時にインスタンスを流し込むためMovable。Staticだとナビ側が後からの追加を追跡しない
		UInstancedStaticMeshComponent* ISM = Owner->CreateDefaultSubobject<UInstancedStaticMeshComponent>(Name);
		ISM->SetupAttachment(Owner->GetRootComponent());
		ISM->SetMobility(EComponentMobility::Movable);
		ISM->SetCollisionProfileName(FName("BlockAll"));
		return ISM;
	}
}

ALevelGenerator::ALevelGenerator()
{
	// 生成はロード時1回だけ。Tickは使わない
	PrimaryActorTick.bCanEverTick = false;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

	FloorISM = CreateGeometryISM(this, TEXT("FloorISM"));
	WallISM = CreateGeometryISM(this, TEXT("WallISM"));
	KitFloorISM = CreateGeometryISM(this, TEXT("KitFloorISM"));
	KitWallISM = CreateGeometryISM(this, TEXT("KitWallISM"));
	KitWallDoorISM = CreateGeometryISM(this, TEXT("KitWallDoorISM"));
	KitCeilingISM = CreateGeometryISM(this, TEXT("KitCeilingISM"));
}

void ALevelGenerator::PreInitializeComponents()
{
	Super::PreInitializeComponents();

	// PlayerStartの検索（プレイヤースポーン）より前に済ませたいのでここで生成する。
	// エディタプレビューワールドでは何もしない
	if (GetWorld() && GetWorld()->IsGameWorld())
	{
		Generate();
	}
}

void ALevelGenerator::BeginPlay()
{
	Super::BeginPlay();

	// PreInitializeComponents時点ではナビシステムが未初期化で、ジオメトリ追加が
	// ダーティ扱いにならず動的NavMeshが張られない。1フレーム待ってから明示的に張り直す
	if (GetWorld() && GetWorld()->IsGameWorld())
	{
		GetWorldTimerManager().SetTimerForNextTick(this, &ALevelGenerator::RefreshNavigation);
	}
}

void ALevelGenerator::Generate()
{
	if (!Preset)
	{
		UE_LOG(LogQuietus, Error, TEXT("%s: Preset is not set — nothing will be generated"), *GetName());
		return;
	}
	const ULevelGenPreset& Settings = *Preset;
	const int32 BaseSeed = ResolveSeed();

	// プレハブの扉ソケット配置を先に読んでおき、レイアウト計算はワールドに触れずに行う
	LevelGen::FRoomPrefabCatalog Catalog;
	if (Settings.IsPrefabMode())
	{
		Catalog.Build(GetWorld(), Settings.Layout.RoomPrefabClasses);
	}
	const TUniquePtr<LevelGen::FLayoutBuilder> Builder = MakeLayoutBuilder(Settings, Catalog);

	LevelGen::FLayout MapLayout;
	FRandomStream Rand;
	if (!BuildLayout(*Builder, BaseSeed, MapLayout, Rand))
	{
		return;
	}

	const TArray<ARoomPrefab*> RoomActors = SpawnRoomActors(Settings, Catalog, MapLayout);
	const LevelGen::FContext Context(GetWorld(), GetActorLocation(), Settings.Layout.CellSize, MapLayout, RoomActors, Rand, GetName());

	TArray<LevelGen::FDoorway> Doorways;
	if (!LevelGen::FGeometryBuilder(Context, Settings.Geometry, GetGeometryComponents()).Build(Doorways))
	{
		return;
	}
	if (Settings.Geometry.KitCeilingMesh)
	{
		LevelGen::FLightingBuilder(Context, Settings.Lighting, Settings.Geometry.WallHeight).Build();
	}
	SpawnGameplayActors(Settings, Context, Doorways);

	if (URunGameInstance* GI = GetWorld()->GetGameInstance<URunGameInstance>())
	{
		GI->SetCurrentRunSeed(BaseSeed);
	}

	UE_LOG(LogQuietus, Log, TEXT("%s: generated map ok (mode=%s seed=%d rooms=%d connections=%d start=%d exit=%d floor=%d walls=%d doors=%d)"),
		*GetName(), Builder->GetModeName(), BaseSeed, MapLayout.Rooms.Num(), MapLayout.Connections.Num(),
		MapLayout.StartRoom, MapLayout.ExitRoom,
		FloorISM->GetInstanceCount() + KitFloorISM->GetInstanceCount(),
		WallISM->GetInstanceCount() + KitWallISM->GetInstanceCount(),
		KitWallDoorISM->GetInstanceCount());

#if !UE_BUILD_SHIPPING
	// 回帰確認用: -GenDump=<ファイル> で生成直後の配置を書き出す
	FString DumpPath;
	if (FParse::Value(FCommandLine::Get(), TEXT("GenDump="), DumpPath))
	{
		LevelGen::DumpWorldLayout(GetWorld(), DumpPath);
	}
#endif
}

int32 ALevelGenerator::ResolveSeed() const
{
	int32 BaseSeed = bRandomizeSeedOnPlay ? FMath::Rand() : Seed;

	// メニューから発行されたシードがあれば最優先
	if (URunGameInstance* GI = GetWorld()->GetGameInstance<URunGameInstance>())
	{
		int32 PendingSeed = 0;
		if (GI->ConsumePendingSeed(PendingSeed))
		{
			BaseSeed = PendingSeed;
		}
	}
	return BaseSeed;
}

TUniquePtr<LevelGen::FLayoutBuilder> ALevelGenerator::MakeLayoutBuilder(const ULevelGenPreset& Settings, const LevelGen::FRoomPrefabCatalog& Catalog) const
{
	if (Settings.IsPrefabMode())
	{
		return MakeUnique<LevelGen::FPrefabLayoutBuilder>(Settings.Layout, GetName(), Catalog);
	}
	return MakeUnique<LevelGen::FGreyboxLayoutBuilder>(Settings.Layout, GetName());
}

bool ALevelGenerator::BuildLayout(const LevelGen::FLayoutBuilder& Builder, int32 BaseSeed, LevelGen::FLayout& OutLayout, FRandomStream& OutRand,
	int32* OutAttempts, bool bLog) const
{
	// 出口の外に階段セット分の空きを求めるので、500 シードで最大 6 回かかった。余裕を持たせる
	constexpr int32 MaxAttempts = 16;
	for (int32 Attempt = 0; Attempt < MaxAttempts; ++Attempt)
	{
		OutRand.Initialize(BaseSeed + Attempt * 7919);
		if (Builder.TryBuild(OutLayout, OutRand))
		{
			if (Attempt > 0 && bLog)
			{
				UE_LOG(LogQuietus, Warning, TEXT("%s: layout succeeded on retry %d (seed %d)"), *GetName(), Attempt, BaseSeed);
			}
			if (OutAttempts)
			{
				*OutAttempts = Attempt + 1;
			}
			return true;
		}
	}
	if (OutAttempts)
	{
		*OutAttempts = MaxAttempts;
	}
	if (bLog)
	{
		UE_LOG(LogQuietus, Error, TEXT("%s: failed to build layout after %d attempts (seed %d)"), *GetName(), MaxAttempts, BaseSeed);
	}
	return false;
}

FString ALevelGenerator::SweepLayouts(int32 FirstSeed, int32 Count) const
{
	if (!Preset)
	{
		return TEXT("no preset");
	}
	const ULevelGenPreset& Settings = *Preset;
	LevelGen::FRoomPrefabCatalog Catalog;
	if (Settings.IsPrefabMode())
	{
		Catalog.Build(GetWorld(), Settings.Layout.RoomPrefabClasses);
	}
	const TUniquePtr<LevelGen::FLayoutBuilder> Builder = MakeLayoutBuilder(Settings, Catalog);

	int32 NumOk = 0;
	TMap<int32, int32> AttemptHistogram;
	TArray<int32> FailedSeeds;
	double TotalMs = 0.0;
	double MaxMs = 0.0;
	for (int32 i = 0; i < Count; ++i)
	{
		const int32 SweepSeed = FirstSeed + i;
		LevelGen::FLayout Layout;
		FRandomStream Rand;
		int32 Attempts = 0;
		const double Start = FPlatformTime::Seconds();
		const bool bOk = BuildLayout(*Builder, SweepSeed, Layout, Rand, &Attempts, /*bLog=*/ false);
		const double Ms = (FPlatformTime::Seconds() - Start) * 1000.0;
		TotalMs += Ms;
		MaxMs = FMath::Max(MaxMs, Ms);
		if (bOk)
		{
			++NumOk;
			AttemptHistogram.FindOrAdd(Attempts)++;
		}
		else
		{
			FailedSeeds.Add(SweepSeed);
		}
	}

	AttemptHistogram.KeySort(TLess<int32>());
	FString Histogram;
	for (const TPair<int32, int32>& Pair : AttemptHistogram)
	{
		Histogram += FString::Printf(TEXT(" %d:%d"), Pair.Key, Pair.Value);
	}
	FString Failed;
	for (int32 i = 0; i < FMath::Min(FailedSeeds.Num(), 20); ++i)
	{
		Failed += FString::Printf(TEXT(" %d"), FailedSeeds[i]);
	}
	const FString Summary = FString::Printf(
		TEXT("mode=%s seeds=%d..%d ok=%d/%d attempts{%s } avg=%.1fms max=%.1fms failed=[%s ]"),
		Builder->GetModeName(), FirstSeed, FirstSeed + Count - 1, NumOk, Count, *Histogram,
		Count > 0 ? TotalMs / Count : 0.0, MaxMs, *Failed);
	UE_LOG(LogQuietus, Log, TEXT("%s: layout sweep %s"), *GetName(), *Summary);
	return Summary;
}

TArray<ARoomPrefab*> ALevelGenerator::SpawnRoomActors(const ULevelGenPreset& Settings, const LevelGen::FRoomPrefabCatalog& Catalog, const LevelGen::FLayout& MapLayout) const
{
	TArray<ARoomPrefab*> RoomActors;
	if (!Settings.IsPrefabMode())
	{
		return RoomActors;
	}

	const float CellSize = Settings.Layout.CellSize;
	for (const LevelGen::FRoom& Room : MapLayout.Rooms)
	{
		const FVector Origin = GetActorLocation() + FVector(Room.X * CellSize, Room.Y * CellSize, 0.0f);
		ARoomPrefab* Actor = Catalog.SpawnRoom(GetWorld(), Room.PrefabIndex, Origin);
		if (!Actor)
		{
			UE_LOG(LogQuietus, Warning, TEXT("%s: failed to spawn room prefab %d"), *GetName(), Room.PrefabIndex);
		}
		RoomActors.Add(Actor);
	}
	return RoomActors;
}

void ALevelGenerator::SpawnGameplayActors(const ULevelGenPreset& Settings, const LevelGen::FContext& Context, const TArray<LevelGen::FDoorway>& Doorways) const
{
	LevelGen::FSpawnSpotPicker Picker(Context);
	// 開始地点の床には何も置かない
	Picker.ReserveCell(Context.Layout.Rooms[Context.Layout.StartRoom].Center());

	// 欠片の数（＝扉の溝の数）は難易度で決まる
	FGenObjectiveSettings Objectives = Settings.Objectives;
	Objectives.ShardCount = URunGameInstance::GetParams(this).ShardCount;
	const LevelGen::FObjectiveSpawner ObjectiveSpawner(Context, Picker, Objectives, Settings.Doors);
	const LevelGen::FEnemySpawner EnemySpawner(Context, Picker, Settings.Enemies);
	const LevelGen::FPickupSpawner PickupSpawner(Context, Picker, Settings.Items);
	const LevelGen::FFurnitureSpawner FurnitureSpawner(Context, Picker, Settings.Items);

	// 乱数の消費順が同じシードから同じマップを作れるかを決めるので、呼び出し順は変えないこと
	ObjectiveSpawner.PlacePlayerStart();
	ObjectiveSpawner.SpawnExitDoor();
	ObjectiveSpawner.SpawnExitStairs();
	const int32 NumShards = ObjectiveSpawner.SpawnShards();
	const int32 NumEnemies = EnemySpawner.SpawnEnemies();
	PickupSpawner.SpawnBatteries();
	PickupSpawner.SpawnThrowables();
	PickupSpawner.SpawnNotes();
	FurnitureSpawner.SpawnCabinets();
	FurnitureSpawner.SpawnLockers();
	const int32 NumPatrol = EnemySpawner.SpawnPatrolPoints();

	UE_LOG(LogQuietus, Log, TEXT("%s: placed gameplay actors (shards=%d enemies=%d patrol=%d)"),
		*GetName(), NumShards, NumEnemies, NumPatrol);

	const int32 NumCulled = LevelGen::ClearDressingAtDoorways(Doorways, Context.RoomActors, Settings.Doors.DoorClearanceDepth);
	if (NumCulled > 0)
	{
		UE_LOG(LogQuietus, Log, TEXT("%s: culled %d dressing meshes blocking doorways"), *GetName(), NumCulled);
	}

	TArray<ADoor*> Doors;
	const int32 NumDoors = LevelGen::FInteriorDoorSpawner(Context, Settings.Doors).Spawn(Doorways, &Doors);
	if (NumDoors > 0)
	{
		UE_LOG(LogQuietus, Log, TEXT("%s: spawned %d interior doors"), *GetName(), NumDoors);
	}

	// 仕掛けは最後に置く（既存の乱数の消費順を変えないため）
	const FDreamDifficultyParams& Difficulty = URunGameInstance::GetParams(this);
	const LevelGen::FGimmickSpawner GimmickSpawner(Context, Settings.Items);
	const int32 NumRainbow = GimmickSpawner.SpawnRainbowDoors(Doors, Difficulty.RainbowDoorCount);
	const int32 NumBalloons = GimmickSpawner.SpawnBalloons(Difficulty.BalloonCount);
	UE_LOG(LogQuietus, Log, TEXT("%s: gimmicks (rainbow doors=%d balloons=%d)"), *GetName(), NumRainbow, NumBalloons);
}

LevelGen::FGeometryComponents ALevelGenerator::GetGeometryComponents() const
{
	LevelGen::FGeometryComponents Components;
	Components.CubeFloor = FloorISM;
	Components.CubeWall = WallISM;
	Components.KitFloor = KitFloorISM;
	Components.KitWall = KitWallISM;
	Components.KitWallDoor = KitWallDoorISM;
	Components.KitCeiling = KitCeilingISM;
	return Components;
}

void ALevelGenerator::RefreshNavigation()
{
	UNavigationSystemV1::UpdateComponentInNavOctree(*FloorISM);
	UNavigationSystemV1::UpdateComponentInNavOctree(*WallISM);
	if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		NavSys->Build();
		UE_LOG(LogQuietus, Log, TEXT("%s: navigation rebuilt"), *GetName());
	}
}
