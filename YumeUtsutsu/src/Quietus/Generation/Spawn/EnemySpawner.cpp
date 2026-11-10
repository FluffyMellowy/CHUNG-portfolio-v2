#include "Generation/Spawn/EnemySpawner.h"
#include "Components/CapsuleComponent.h"
#include "Core/GameConstants.h"
#include "Engine/TargetPoint.h"
#include "Enemies/EnemyCharacter.h"
#include "Generation/GenerationContext.h"
#include "Generation/RoomPrefab.h"
#include "Generation/Spawn/SpawnSpotPicker.h"

namespace LevelGen
{
	TArray<int32> FEnemySpawner::GetRoomsFarthestFirst() const
	{
		const TArray<FRoom>& Rooms = Context.Layout.Rooms;
		const FIntPoint StartCenter = Rooms[Context.Layout.StartRoom].Center();

		TArray<int32> Result;
		for (int32 i = 0; i < Rooms.Num(); ++i)
		{
			if (i != Context.Layout.StartRoom)
			{
				Result.Add(i);
			}
		}
		Result.Sort([&Rooms, &StartCenter](int32 A, int32 B)
		{
			const FIntPoint DA = Rooms[A].Center() - StartCenter;
			const FIntPoint DB = Rooms[B].Center() - StartCenter;
			return DA.X * DA.X + DA.Y * DA.Y > DB.X * DB.X + DB.Y * DB.Y;
		});
		return Result;
	}

	int32 FEnemySpawner::SpawnEnemies() const
	{
		const TArray<int32> EnemyRooms = GetRoomsFarthestFirst();

		// 床に足が着く高さ（カプセル半高＋少し）に置く
		const float HalfHeight = Settings.EnemyClass
			? Settings.EnemyClass->GetDefaultObject<AEnemyCharacter>()->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()
			: 88.0f;
		const float SpawnZ = HalfHeight + 2.0f;

		int32 NumSpawned = 0;
		for (int32 e = 0; e < FMath::Min(Settings.EnemyCount, EnemyRooms.Num()); ++e)
		{
			const int32 RoomIdx = EnemyRooms[e];
			FVector Location = Context.CellCenter(Context.Layout.Rooms[RoomIdx].Center(), SpawnZ);
			FRotator Rotation(0, Context.Rand.FRandRange(0.0f, 360.0f), 0);

			const TArray<USpawnMarkerComponent*> Markers = Picker.GetMarkers(RoomIdx, ESpawnMarkerType::Enemy);
			if (Markers.Num() > 0)
			{
				const USpawnMarkerComponent* Marker = Markers[Context.Rand.RandRange(0, Markers.Num() - 1)];
				Location = Marker->GetComponentLocation() + FVector(0, 0, SpawnZ);
				Rotation = FRotator(0, Marker->GetComponentRotation().Yaw, 0);
			}

			if (Context.SpawnActor<AEnemyCharacter>(Settings.EnemyClass, Location, Rotation))
			{
				++NumSpawned;
			}
		}
		return NumSpawned;
	}

	int32 FEnemySpawner::SpawnPatrolPoints() const
	{
		TArray<FVector> Points;
		for (int32 i = 0; i < Context.Layout.Rooms.Num(); ++i)
		{
			const TArray<USpawnMarkerComponent*> Markers = Picker.GetMarkers(i, ESpawnMarkerType::PatrolPoint);
			if (Markers.Num() == 0)
			{
				Points.Add(Context.CellCenter(Context.Layout.Rooms[i].Center()));
				continue;
			}
			for (const USpawnMarkerComponent* Marker : Markers)
			{
				Points.Add(Marker->GetComponentLocation());
			}
		}
		ShuffleArray(Points, Context.Rand);

		const int32 NumPatrol = FMath::Min(Settings.PatrolPointCount, Points.Num());
		for (int32 p = 0; p < NumPatrol; ++p)
		{
			if (AActor* Point = Context.SpawnActor(ATargetPoint::StaticClass(), Points[p], FRotator::ZeroRotator))
			{
				// 敵AIはこのタグで巡回先を収集する
				Point->Tags.Add(GameNames::PatrolPointTag);
			}
		}
		return NumPatrol;
	}
}
