#pragma once

#include "CoreMinimal.h"
#include "Generation/GenerationSettings.h"

class ADoor;

namespace LevelGen
{
	struct FContext;

	/**
	 *  迷路の仕掛けを置く:
	 *  - 生成済みの部屋の扉からいくつかを選んで虹色の扉にする（脱出部屋に面した扉は除く）
	 *  - 廊下のセルに風船を浮かべる（出口の外の階段セットの上は除く）
	 *  個数は難易度で決まるので呼ぶ側が渡す。
	 */
	class FGimmickSpawner
	{
	public:
		FGimmickSpawner(const FContext& InContext, const FGenItemSettings& InSettings)
			: Context(InContext), Settings(InSettings)
		{
		}

		/** @return 虹色にした扉の数 */
		int32 SpawnRainbowDoors(const TArray<ADoor*>& Doors, int32 Count) const;

		/** @return 置いた風船の数 */
		int32 SpawnBalloons(int32 Count) const;

	private:
		template <typename T>
		void Shuffle(TArray<T>& Items) const;

		const FContext& Context;
		const FGenItemSettings& Settings;
	};
}
