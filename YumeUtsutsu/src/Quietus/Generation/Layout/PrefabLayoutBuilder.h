#pragma once

#include "CoreMinimal.h"
#include "Generation/Layout/LayoutBuilder.h"

namespace LevelGen
{
	class FRoomPrefabCatalog;

	/**
	 *  部屋プレハブ用レイアウト。
	 *  プレハブの footprint で部屋を置き、扉ソケット同士を廊下（BFS最短路）で結ぶ。
	 *  部屋と廊下は壁で仕切られ、接続に使った扉辺だけが開口になる。
	 */
	class FPrefabLayoutBuilder : public FLayoutBuilder
	{
	public:
		FPrefabLayoutBuilder(const FGenLayoutSettings& InSettings, const FString& InLogOwner, const FRoomPrefabCatalog& InCatalog)
			: FLayoutBuilder(InSettings, InLogOwner), Catalog(InCatalog)
		{
		}

		virtual bool TryBuild(FLayout& OutLayout, FRandomStream& Rand) const override;
		virtual const TCHAR* GetModeName() const override { return TEXT("prefab"); }

	private:
		/** 開始・脱出候補を1部屋ずつ必ず確保してから、残りをランダムなプレハブで埋める */
		bool PlaceRooms(FLayout& Layout, FRandomStream& Rand) const;

		/** 指定プレハブを1部屋、ランダムな位置に置けたら true（配置上限も見る） */
		bool TryPlacePrefab(FLayout& Layout, int32 PrefabIdx, TArray<int32>& PlacedPerPrefab, FRandomStream& Rand) const;

		/** 各部屋の扉ソケットをタイル辺に変換する。使える扉が無い部屋があれば不成立 */
		bool CollectDoorEdges(FLayout& Layout) const;

		/** 接続グラフの各辺について、両部屋の扉同士を廊下で結ぶ */
		bool ConnectRooms(FLayout& Layout) const;

		/** 開始部屋（候補からランダム）と脱出部屋（開始から最遠）を決め、脱出扉と、その先の階段セット用の空きを決める */
		bool PickStartAndExit(FLayout& Layout, FRandomStream& Rand) const;

		/** 部屋セルを避けて空き・廊下セル上をBFSで最短接続する */
		static bool FindCorridorPath(const FCellGrid& Grid, const FIntPoint& From, const FIntPoint& To, TArray<FIntPoint>& OutPath);

		const FRoomPrefabCatalog& Catalog;
	};
}
