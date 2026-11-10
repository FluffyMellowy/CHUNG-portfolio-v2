#pragma once

#include "CoreMinimal.h"
#include "Generation/GenerationSettings.h"

namespace LevelGen
{
	struct FContext;
	struct FDoorway;
}

class ADoor;

namespace LevelGen
{

	/** 部屋-廊下の扉モジュールに開閉ドアを立てる（脱出扉の辺は脱出扉が担当するので除外） */
	class FInteriorDoorSpawner
	{
	public:
		FInteriorDoorSpawner(const FContext& InContext, const FGenDoorSettings& InSettings)
			: Context(InContext), Settings(InSettings)
		{
		}

		/** 立てたドアは OutDoors に足す。脱出部屋に面したドアには印を付ける（虹色の扉の行き先から外すため）
		 *  @return 立てたドアの数 */
		int32 Spawn(const TArray<FDoorway>& Doorways, TArray<ADoor*>* OutDoors = nullptr) const;

	private:
		const FContext& Context;
		const FGenDoorSettings& Settings;
	};
}
