#pragma once

#include "CoreMinimal.h"
#include "Generation/GenerationSettings.h"

namespace LevelGen
{
	struct FContext;
	class FSpawnSpotPicker;

	/** 調べられる家具（中身入りキャビネット／隠れロッカー）を配置する */
	class FFurnitureSpawner
	{
	public:
		FFurnitureSpawner(const FContext& InContext, FSpawnSpotPicker& InPicker, const FGenItemSettings& InSettings)
			: Context(InContext), Picker(InPicker), Settings(InSettings)
		{
		}

		/** Cabinetマーカー優先。中身（拾得アイテム／メモ）は開けた時にキャビネット側で抽選する */
		int32 SpawnCabinets() const;

		/** HidingSpotマーカー優先 */
		int32 SpawnLockers() const;

	private:
		const FContext& Context;
		FSpawnSpotPicker& Picker;
		const FGenItemSettings& Settings;
	};
}
