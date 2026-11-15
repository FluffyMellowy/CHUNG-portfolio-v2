#pragma once

#include "CoreMinimal.h"
#include "Generation/GenerationSettings.h"

namespace LevelGen
{
	struct FContext;

	/**
	 *  迷路の仕掛けを置く: 廊下のセルに風船を浮かべる（出口の外の階段セットの上は除く）。
	 *  個数は難易度で決まるので呼ぶ側が渡す。
	 */
	class FGimmickSpawner
	{
	public:
		FGimmickSpawner(const FContext& InContext, const FGenItemSettings& InSettings)
			: Context(InContext), Settings(InSettings)
		{
		}

		/** @return 置いた風船の数 */
		int32 SpawnBalloons(int32 Count) const;

	private:
		template <typename T>
		void Shuffle(TArray<T>& Items) const;

		const FContext& Context;
		const FGenItemSettings& Settings;
	};
}
