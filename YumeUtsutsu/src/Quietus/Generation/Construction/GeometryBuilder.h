#pragma once

#include "CoreMinimal.h"
#include "Generation/GenerationSettings.h"

class UInstancedStaticMeshComponent;

namespace LevelGen
{
	struct FContext;

	/** 扉付き壁モジュールの配置（開口中心・壁沿いの向き）。室内ドアとクリアランス確保が使う */
	struct FDoorway
	{
		uint64 EdgeKey = 0;
		FTransform Transform;
	};

	/** ジオメトリを流し込む先の ISM 一式（生成器アクターが所有） */
	struct FGeometryComponents
	{
		UInstancedStaticMeshComponent* CubeFloor = nullptr;
		UInstancedStaticMeshComponent* CubeWall = nullptr;
		UInstancedStaticMeshComponent* KitFloor = nullptr;
		UInstancedStaticMeshComponent* KitWall = nullptr;
		UInstancedStaticMeshComponent* KitWallDoor = nullptr;
		UInstancedStaticMeshComponent* KitCeiling = nullptr;

		TArray<UInstancedStaticMeshComponent*> All() const
		{
			return { CubeFloor, CubeWall, KitFloor, KitWall, KitWallDoor, KitCeiling };
		}
	};

	/**
	 *  レイアウトから床・壁・天井を ISM に積む。
	 *  キットピースが設定されていればそれを、無ければ100cmキューブを伸縮して使う。
	 */
	class FGeometryBuilder
	{
	public:
		FGeometryBuilder(const FContext& InContext, const FGenGeometrySettings& InSettings, const FGeometryComponents& InComponents)
			: Context(InContext), Settings(InSettings), Components(InComponents)
		{
		}

		/** @param OutDoorways 置いた扉モジュールの一覧 */
		bool Build(TArray<FDoorway>& OutDoorways) const;

	private:
		void AssignMeshes() const;
		void AddFloorAndCeiling(const FIntPoint& Cell) const;

		/** 各辺を一度ずつ処理する（境界辺は埋まっている側から、内部辺は+X/+Y側から） */
		void AddWallsAround(const FIntPoint& Cell, TArray<FDoorway>& OutDoorways) const;
		void AddWallEdge(const FIntPoint& Cell, const FIntPoint& Dir, bool bDoorEdge, TArray<FDoorway>& OutDoorways) const;

		/** 壁の向こうの空きセル上空が見えないよう、隣接1タイル分まで天井を張り出す */
		void AddCeilingSkirt() const;

		const FContext& Context;
		const FGenGeometrySettings& Settings;
		FGeometryComponents Components;
	};
}
