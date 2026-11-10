#include "Generation/Spawn/ObjectiveSpawner.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "Generation/GenerationContext.h"
#include "Generation/RoomPrefab.h"
#include "Generation/Spawn/SpawnSpotPicker.h"
#include "Environment/WakeStairs.h"
#include "Interactables/DreamShard.h"
#include "Interactables/ExitDoor.h"

namespace LevelGen
{
	namespace
	{
		/** PlayerStart はカプセル中心の高さに置く */
		constexpr float PlayerStartHeight = 92.0f;
	}

	void FObjectiveSpawner::PlacePlayerStart() const
	{
		const int32 StartRoom = Context.Layout.StartRoom;

		// PlayerStartマーカーがあればその位置と向き、無ければ部屋中心
		FVector Location = Context.CellCenter(Context.Layout.Rooms[StartRoom].Center(), PlayerStartHeight);
		FRotator Rotation = FRotator::ZeroRotator;
		const TArray<USpawnMarkerComponent*> Markers = Picker.GetMarkers(StartRoom, ESpawnMarkerType::PlayerStart);
		if (Markers.Num() > 0)
		{
			Location = Markers[0]->GetComponentLocation() + FVector(0, 0, PlayerStartHeight);
			Rotation = FRotator(0, Markers[0]->GetComponentRotation().Yaw, 0);
		}

		TActorIterator<APlayerStart> It(Context.World);
		if (It)
		{
			It->SetActorLocationAndRotation(Location, Rotation);
		}
		else
		{
			Context.SpawnActor(APlayerStart::StaticClass(), Location, Rotation);
		}
	}

	void FObjectiveSpawner::SpawnExitDoor() const
	{
		// 扉のローカル +X（溝のある正面）を部屋の内側へ向ける。開くと外側の階段へ抜ける
		const FIntPoint& Dir = Context.Layout.ExitDoorDir;
		const FVector Outward((float)Dir.X, (float)Dir.Y, 0.0f);
		const FRotator DoorRot = (-Outward).Rotation();
		const FVector EdgeCenter = Context.EdgeCenter(Context.Layout.ExitDoorCell, Dir);

		if (AExitDoor* Door = Context.SpawnActor<AExitDoor>(DoorSettings.ExitDoorClass, EdgeCenter, DoorRot))
		{
			Door->SetSlotCount(Settings.ShardCount);
		}
	}

	void FObjectiveSpawner::SpawnExitStairs() const
	{
		if (!Settings.ExitStairsClass)
		{
			return;
		}
		const FIntPoint& Dir = Context.Layout.ExitDoorDir;
		const FRotator StairsRot = FVector((float)Dir.X, (float)Dir.Y, 0.0f).Rotation();
		Context.SpawnActor(Settings.ExitStairsClass, Context.EdgeCenter(Context.Layout.ExitDoorCell, Dir), StairsRot);
	}

	int32 FObjectiveSpawner::SpawnShards() const
	{
		const int32 N = Context.Layout.Rooms.Num();
		const int32 StartRoom = Context.Layout.StartRoom;

		const TArray<FSpawnSpot> Spots = Picker.CollectShuffledMarkerSpots(ESpawnMarkerType::Shard,
			[StartRoom](int32 RoomIdx) { return RoomIdx != StartRoom; });

		int32 NumSpawned = 0;
		for (int32 f = 0; f < Settings.ShardCount; ++f)
		{
			FSpawnSpot Spot;
			if (Spots.IsValidIndex(f))
			{
				Spot = Spots[f];
			}
			else
			{
				// マーカー不足: 開始部屋以外のランダムな部屋の中心へ
				const int32 RoomIdx = (StartRoom + 1 + Context.Rand.RandRange(0, N - 2)) % N;
				Spot.Location = Context.CellCenter(Context.Layout.Rooms[RoomIdx].Center(), 40.0f);
			}
			if (Context.SpawnActor(Settings.ShardClass, Spot.Location, Spot.Rotation))
			{
				++NumSpawned;
			}
		}
		return NumSpawned;
	}
}
