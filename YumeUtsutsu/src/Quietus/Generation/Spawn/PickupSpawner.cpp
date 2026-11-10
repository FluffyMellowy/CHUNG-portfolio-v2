#include "Generation/Spawn/PickupSpawner.h"
#include "Generation/RoomPrefab.h"
#include "Generation/Spawn/SpawnSpotPicker.h"
#include "Interactables/Battery.h"
#include "Interactables/NoteItem.h"
#include "Interactables/Throwable.h"

namespace LevelGen
{
	namespace
	{
		/** マーカーが足りない時に床へ置く高さ */
		constexpr float PickupFloorZ = 30.0f;
	}

	int32 FPickupSpawner::SpawnBatteries() const
	{
		return SpawnBatchAtSpots(Context, Picker, Settings.BatteryCount, ESpawnMarkerType::Battery,
			Settings.BatteryClass, PickupFloorZ, [](AActor*, int32) {});
	}

	int32 FPickupSpawner::SpawnThrowables() const
	{
		return SpawnBatchAtSpots(Context, Picker, Settings.ThrowableCount, ESpawnMarkerType::Throwable,
			Settings.ThrowableClass, PickupFloorZ, [](AActor*, int32) {});
	}

	int32 FPickupSpawner::SpawnNotes() const
	{
		return SpawnBatchAtSpots(Context, Picker, Settings.NoteCount, ESpawnMarkerType::Note,
			Settings.NoteClass, PickupFloorZ,
			[this](AActor* Item, int32 Index)
			{
				if (ANoteItem* Note = Cast<ANoteItem>(Item))
				{
					const FText Title = FText::Format(NSLOCTEXT("Generator", "NoteTitle", "メモ #{0}"), Index + 1);
					Note->SetNoteText(Title, Settings.GetNoteBody(Index));
				}
			});
	}
}
