#pragma once

#include "CoreMinimal.h"
#include "Generation/GenerationSettings.h"

namespace LevelGen
{
	struct FContext;
	class FSpawnSpotPicker;

	/** 徘徊敵と、敵AIが巡回に使うパトロールポイントを配置する */
	class FEnemySpawner
	{
	public:
		FEnemySpawner(const FContext& InContext, FSpawnSpotPicker& InPicker, const FGenEnemySettings& InSettings)
			: Context(InContext), Picker(InPicker), Settings(InSettings)
		{
		}

		/** 開始部屋から遠い部屋から順に1体ずつ（Enemyマーカー優先、無ければ部屋中心）。@return 置いた数 */
		int32 SpawnEnemies() const;

		/** PatrolPointマーカー（無い部屋は中心）から PatrolPointCount 個を選ぶ。@return 置いた数 */
		int32 SpawnPatrolPoints() const;

	private:
		/** 開始部屋以外を、開始部屋からの距離が遠い順に並べる */
		TArray<int32> GetRoomsFarthestFirst() const;

		const FContext& Context;
		FSpawnSpotPicker& Picker;
		const FGenEnemySettings& Settings;
	};
}
