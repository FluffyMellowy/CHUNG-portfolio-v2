#include "Generation/Construction/InteriorDoorSpawner.h"
#include "Generation/Construction/GeometryBuilder.h"
#include "Generation/GenerationContext.h"
#include "Interactables/Door.h"

namespace LevelGen
{
	int32 FInteriorDoorSpawner::Spawn(const TArray<FDoorway>& Doorways, TArray<ADoor*>* OutDoors) const
	{
		if (!Settings.bSpawnInteriorDoors || !Settings.InteriorDoorClass)
		{
			return 0;
		}

		const uint64 ExitKey = Context.Layout.GetExitDoorKey();
		int32 NumDoors = 0;
		for (const FDoorway& Doorway : Doorways)
		{
			if (Doorway.EdgeKey == ExitKey)
			{
				continue;
			}
			ADoor* Door = Context.SpawnActor<ADoor>(Settings.InteriorDoorClass,
				Doorway.Transform.GetLocation(), Doorway.Transform.Rotator());
			if (!Door)
			{
				continue;
			}
			Door->SetStartOpenRatio(Context.Rand.FRandRange(Settings.DoorStartOpenMin, Settings.DoorStartOpenMax));

			// 扉の両側のセルのどちらかが脱出部屋なら印を付ける（通り抜け方向はローカル Y）
			const FVector Through = Doorway.Transform.GetRotation().RotateVector(FVector::YAxisVector) * (Context.CellSize * 0.25f);
			for (const FVector& Side : { Doorway.Transform.GetLocation() + Through, Doorway.Transform.GetLocation() - Through })
			{
				if (Context.Layout.Grid.GetRoomAt(Context.WorldToCell(Side)) == Context.Layout.ExitRoom)
				{
					Door->SetInExitRoom(true);
				}
			}
			if (OutDoors)
			{
				OutDoors->Add(Door);
			}
			++NumDoors;
		}
		return NumDoors;
	}
}
