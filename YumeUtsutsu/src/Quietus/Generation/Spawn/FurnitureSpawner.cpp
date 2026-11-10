#include "Generation/Spawn/FurnitureSpawner.h"
#include "Generation/GenerationContext.h"
#include "Generation/RoomPrefab.h"
#include "Generation/Spawn/SpawnSpotPicker.h"
#include "Interactables/Battery.h"
#include "Interactables/Cabinet.h"
#include "Interactables/Locker.h"
#include "Interactables/NoteItem.h"
#include "Interactables/Throwable.h"

namespace LevelGen
{
	int32 FFurnitureSpawner::SpawnCabinets() const
	{
		return SpawnBatchAtSpots(Context, Picker, Settings.CabinetCount, ESpawnMarkerType::Cabinet,
			Settings.CabinetClass, 0.0f,
			[this](AActor* Item, int32 Index)
			{
				ACabinet* Cabinet = Cast<ACabinet>(Item);
				if (!Cabinet)
				{
					return;
				}
				// 床のロアメモと同じ本文が続かないよう1つずらす
				Cabinet->ConfigureLoot(Settings.CabinetLoot, Settings.BatteryClass, Settings.ThrowableClass, Settings.NoteClass,
					NSLOCTEXT("Generator", "CabinetNoteTitle", "作業指示書"), Settings.GetNoteBody(Index + 1),
					Context.Rand.RandRange(0, MAX_int32 - 1));
			});
	}

	int32 FFurnitureSpawner::SpawnLockers() const
	{
		return SpawnBatchAtSpots(Context, Picker, Settings.LockerCount, ESpawnMarkerType::HidingSpot,
			Settings.LockerClass, 0.0f, [](AActor*, int32) {});
	}
}
