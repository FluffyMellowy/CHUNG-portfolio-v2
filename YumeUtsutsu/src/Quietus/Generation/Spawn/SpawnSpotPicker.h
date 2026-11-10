#pragma once

#include "CoreMinimal.h"
#include "Templates/Function.h"

class AActor;
class USpawnMarkerComponent;
enum class ESpawnMarkerType : uint8;

namespace LevelGen
{
	struct FContext;

	/** スポーン位置と向き */
	struct FSpawnSpot
	{
		FVector Location = FVector::ZeroVector;
		FRotator Rotation = FRotator::ZeroRotator;
	};

	/** フィッシャー–イェーツでシャッフルする（Rand の消費順は固定） */
	template <typename T>
	void ShuffleArray(TArray<T>& Array, FRandomStream& Rand)
	{
		for (int32 i = Array.Num() - 1; i > 0; --i)
		{
			Array.Swap(i, Rand.RandRange(0, i));
		}
	}

	/**
	 *  「どこに置くか」を決める。部屋プレハブのマーカーを優先し、
	 *  足りなければ部屋の床からランダムに選ぶ（既に何か置いたセルは避ける）。
	 */
	class FSpawnSpotPicker
	{
	public:
		explicit FSpawnSpotPicker(const FContext& InContext) : Context(InContext) {}

		/** 部屋内の指定種別マーカー（プレハブが無い部屋は空） */
		TArray<USpawnMarkerComponent*> GetMarkers(int32 RoomIdx, ESpawnMarkerType Type) const;

		/** 条件を満たす部屋のマーカー位置を部屋順に集め、シャッフルして返す */
		TArray<FSpawnSpot> CollectShuffledMarkerSpots(ESpawnMarkerType Type, TFunctionRef<bool(int32)> RoomFilter) const;

		/** まだ何も置いていない床セルをランダムに選んで使用済みにする（向きもランダム） */
		FSpawnSpot PickFreeFloorSpot(float Z);

		/** このセルには床置きの物を置かない */
		void ReserveCell(const FIntPoint& Cell) { UsedCells.Add(Cell); }

	private:
		const FContext& Context;
		TSet<FIntPoint> UsedCells;
	};

	/**
	 *  Class を Count 個、マーカー優先（全部屋）・不足分は空き床（高さ FallbackZ）にスポーンし、
	 *  それぞれ Configure(Actor, 通し番号) を呼ぶ。
	 *  @return スポーンできた数
	 */
	int32 SpawnBatchAtSpots(const FContext& Context, FSpawnSpotPicker& Picker, int32 Count, ESpawnMarkerType MarkerType,
		UClass* Class, float FallbackZ, TFunctionRef<void(AActor*, int32)> Configure);
}
