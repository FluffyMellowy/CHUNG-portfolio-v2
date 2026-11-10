#include "Generation/Spawn/SpawnSpotPicker.h"
#include "Generation/GenerationContext.h"
#include "Generation/RoomPrefab.h"

namespace LevelGen
{
	TArray<USpawnMarkerComponent*> FSpawnSpotPicker::GetMarkers(int32 RoomIdx, ESpawnMarkerType Type) const
	{
		TArray<USpawnMarkerComponent*> Markers;
		if (const ARoomPrefab* Room = Context.GetRoomActor(RoomIdx))
		{
			Room->GetMarkersOfType(Type, Markers);
		}
		return Markers;
	}

	TArray<FSpawnSpot> FSpawnSpotPicker::CollectShuffledMarkerSpots(ESpawnMarkerType Type, TFunctionRef<bool(int32)> RoomFilter) const
	{
		TArray<FSpawnSpot> Spots;
		for (int32 i = 0; i < Context.Layout.Rooms.Num(); ++i)
		{
			if (!RoomFilter(i))
			{
				continue;
			}
			for (const USpawnMarkerComponent* Marker : GetMarkers(i, Type))
			{
				Spots.Add({ Marker->GetComponentLocation(), Marker->GetComponentRotation() });
			}
		}
		ShuffleArray(Spots, Context.Rand);
		return Spots;
	}

	FSpawnSpot FSpawnSpotPicker::PickFreeFloorSpot(float Z)
	{
		const TArray<FRoom>& Rooms = Context.Layout.Rooms;
		FRandomStream& Rand = Context.Rand;

		// 数回だけ引き直して未使用セルを探す（見つからなければ重ねて置く）
		FIntPoint Cell(0, 0);
		for (int32 Try = 0; Try < 10; ++Try)
		{
			const FRoom& Room = Rooms[Rand.RandRange(0, Rooms.Num() - 1)];
			Cell = FIntPoint(Rand.RandRange(Room.X, Room.X + Room.W - 1), Rand.RandRange(Room.Y, Room.Y + Room.H - 1));
			if (!UsedCells.Contains(Cell))
			{
				break;
			}
		}
		UsedCells.Add(Cell);
		return { Context.CellCenter(Cell, Z), FRotator(0, Rand.FRandRange(0.0f, 360.0f), 0) };
	}

	int32 SpawnBatchAtSpots(const FContext& Context, FSpawnSpotPicker& Picker, int32 Count, ESpawnMarkerType MarkerType,
		UClass* Class, float FallbackZ, TFunctionRef<void(AActor*, int32)> Configure)
	{
		if (Count <= 0 || !Class)
		{
			return 0;
		}

		const TArray<FSpawnSpot> MarkerSpots = Picker.CollectShuffledMarkerSpots(MarkerType, [](int32) { return true; });
		int32 NumSpawned = 0;
		for (int32 i = 0; i < Count; ++i)
		{
			// 乱数の消費順を保つため、フォールバック位置は1個ずつスポーン直前に引く
			const FSpawnSpot Spot = MarkerSpots.IsValidIndex(i) ? MarkerSpots[i] : Picker.PickFreeFloorSpot(FallbackZ);
			AActor* Actor = Context.SpawnActor(Class, Spot.Location, Spot.Rotation);
			if (!Actor)
			{
				continue;
			}
			Configure(Actor, i);
			++NumSpawned;
		}
		return NumSpawned;
	}
}
