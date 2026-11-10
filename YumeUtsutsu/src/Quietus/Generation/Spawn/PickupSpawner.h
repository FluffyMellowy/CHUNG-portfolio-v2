#pragma once

#include "CoreMinimal.h"
#include "Generation/GenerationSettings.h"

namespace LevelGen
{
	struct FContext;
	class FSpawnSpotPicker;

	/** 床や棚に置く拾得アイテム（バッテリー／投擲物／ロアメモ）を配置する */
	class FPickupSpawner
	{
	public:
		FPickupSpawner(const FContext& InContext, FSpawnSpotPicker& InPicker, const FGenItemSettings& InSettings)
			: Context(InContext), Picker(InPicker), Settings(InSettings)
		{
		}

		int32 SpawnBatteries() const;
		int32 SpawnThrowables() const;
		int32 SpawnNotes() const;

	private:
		const FContext& Context;
		FSpawnSpotPicker& Picker;
		const FGenItemSettings& Settings;
	};
}
