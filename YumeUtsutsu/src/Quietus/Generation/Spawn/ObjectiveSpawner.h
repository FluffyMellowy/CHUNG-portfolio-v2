#pragma once

#include "CoreMinimal.h"
#include "Generation/GenerationSettings.h"

namespace LevelGen
{
	struct FContext;
	class FSpawnSpotPicker;

	/**
	 *  1ランの目標の流れを配置する:
	 *  開始地点 → 夢の欠片（開始部屋以外に分散） → 脱出扉（脱出部屋の外周） → 扉の先の階段セット
	 */
	class FObjectiveSpawner
	{
	public:
		FObjectiveSpawner(const FContext& InContext, FSpawnSpotPicker& InPicker,
			const FGenObjectiveSettings& InSettings, const FGenDoorSettings& InDoorSettings)
			: Context(InContext), Picker(InPicker), Settings(InSettings), DoorSettings(InDoorSettings)
		{
		}

		/** レベルの PlayerStart を開始部屋へ移す（無ければスポーン） */
		void PlacePlayerStart() const;

		/** 脱出扉を選ばれた扉辺に立てる（溝のある正面を部屋側へ向け、溝の数＝欠片の数） */
		void SpawnExitDoor() const;

		/** 脱出扉の外側に階段セットを置く */
		void SpawnExitStairs() const;

		/** @return 置いた欠片の数 */
		int32 SpawnShards() const;

	private:
		const FContext& Context;
		FSpawnSpotPicker& Picker;
		const FGenObjectiveSettings& Settings;
		const FGenDoorSettings& DoorSettings;
	};
}
