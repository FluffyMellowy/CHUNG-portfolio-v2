#pragma once

#include "CoreMinimal.h"
#include "Generation/GenerationSettings.h"

namespace LevelGen
{
	struct FContext;
	class FSpawnSpotPicker;

	/** 床や棚に置く拾得アイテム（バッテリー／投擲物／ロアメモ／鍵）を配置する */
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

		/** 鍵を置く。数は難易度で決まるので呼ぶ側が渡す */
		int32 SpawnKeys(int32 Count) const;

	private:
		const FContext& Context;
		FSpawnSpotPicker& Picker;
		const FGenItemSettings& Settings;
	};
}
