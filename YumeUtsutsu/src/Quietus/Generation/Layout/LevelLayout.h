#pragma once

#include "CoreMinimal.h"

/**
 *  手続き生成の中間データ（UObject非依存の純データ）。
 *  レイアウト段で作られ、以降のジオメトリ・スポーン段は読むだけ。
 */
namespace LevelGen
{
	/** 4近傍（+X, -X, +Y, -Y）。この順序は経路探索の結果に影響するので変えないこと */
	inline const FIntPoint NeighborDirs[4] = { FIntPoint(1, 0), FIntPoint(-1, 0), FIntPoint(0, 1), FIntPoint(0, -1) };

	/** セルの種別 */
	enum class ECell : uint8
	{
		Empty,
		Room,
		Corridor,
	};

	/** タイル辺（Cell と Cell+Dir の境界）を向きに依存しない64bitキーにする */
	uint64 EdgeKey(const FIntPoint& Cell, const FIntPoint& Dir);

	/** 配置済みの部屋（タイル座標の矩形） */
	struct FRoom
	{
		int32 X = 0, Y = 0, W = 0, H = 0;

		/** プレハブモード時: カタログ内のプレハブ番号 */
		int32 PrefabIndex = INDEX_NONE;

		FIntPoint Center() const { return FIntPoint(X + W / 2, Y + H / 2); }
		bool Contains(const FIntPoint& Cell) const { return Cell.X >= X && Cell.X < X + W && Cell.Y >= Y && Cell.Y < Y + H; }

		/** 間隔 Gap タイルを含めて他の部屋と重なるか */
		bool OverlapsWithGap(const FRoom& Other, int32 Gap) const;
	};

	/** 部屋の扉ソケットをタイル辺に対応づけたもの（プレハブモード） */
	struct FDoorEdge
	{
		FIntPoint Inside = FIntPoint::ZeroValue;  // 部屋側のセル
		FIntPoint Outside = FIntPoint::ZeroValue; // 廊下側のセル
		FIntPoint Dir = FIntPoint::ZeroValue;     // Inside→Outside の向き
		int32 RoomIdx = INDEX_NONE;
		bool bUsed = false;

		uint64 Key() const { return EdgeKey(Inside, Dir); }
	};

	/** タイルグリッド（セル種別＋どの部屋に属するか） */
	class FCellGrid
	{
	public:
		void Reset(const FIntPoint& InSize);

		const FIntPoint& GetSize() const { return Size; }
		bool IsInBounds(const FIntPoint& Cell) const;

		/** 範囲外は Empty 扱い */
		ECell Get(const FIntPoint& Cell) const;
		void Set(const FIntPoint& Cell, ECell Value);
		bool IsFilled(const FIntPoint& Cell) const { return Get(Cell) != ECell::Empty; }

		/** 部屋セルなら部屋番号、それ以外は INDEX_NONE */
		int32 GetRoomAt(const FIntPoint& Cell) const;

		/** 部屋の矩形を Room セルとして書き込む */
		void StampRoom(const FRoom& Room, int32 RoomIdx);

		/** 空きセルだけを廊下にする（部屋・既存廊下は上書きしない） */
		void CarveCorridor(const FIntPoint& Cell);

		/** From から埋まったセルを辿ったBFS距離（到達不能は -1）。添字は ToIndex で引く */
		TArray<int32> ComputeDistances(const FIntPoint& From) const;
		int32 ToIndex(const FIntPoint& Cell) const { return Cell.Y * Size.X + Cell.X; }

	private:
		FIntPoint Size = FIntPoint::ZeroValue;
		TArray<ECell> Cells;
		TArray<int16> CellRoom;
	};

	/** レイアウト段の成果物 */
	struct FLayout
	{
		FCellGrid Grid;
		TArray<FRoom> Rooms;

		/** 部屋間の接続（部屋番号のペア） */
		TArray<TPair<int32, int32>> Connections;

		int32 StartRoom = 0;
		int32 ExitRoom = 0;

		/** 脱出扉の辺（脱出部屋側セルと外向き方向） */
		FIntPoint ExitDoorCell = FIntPoint::ZeroValue;
		FIntPoint ExitDoorDir = FIntPoint::ZeroValue;

		/** 脱出扉の先の階段セットが通るタイル（空きのまま、天井の縁取りも置かない） */
		TArray<FIntPoint> ExitStairCells;

		/** 部屋と廊下の境界を壁で仕切るか（プレハブ）。false なら連続した空間として扱う（グレイボックス） */
		bool bSeparateRooms = false;

		/** プレハブの扉ソケット一覧と、接続に使われた扉辺のキー集合 */
		TArray<FDoorEdge> DoorEdges;
		TSet<uint64> UsedDoorKeys;

		void Reset(const FIntPoint& GridSize);
		uint64 GetExitDoorKey() const { return EdgeKey(ExitDoorCell, ExitDoorDir); }
	};
}
