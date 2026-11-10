#pragma once

#include "CoreMinimal.h"
#include "Templates/Casts.h"
#include "Generation/Layout/LevelLayout.h"

class UWorld;
class AActor;
class ARoomPrefab;

namespace LevelGen
{
	/**
	 *  レイアウト確定後の各段（ジオメトリ・照明・スポーン）が共有する参照一式。
	 *  タイル座標→ワールド座標の変換とアクターのスポーンをここに集約する。
	 */
	struct FContext
	{
		UWorld* World = nullptr;

		/** 生成器アクターの位置（グリッド原点 = タイル(0,0)の最小コーナー） */
		FVector Origin = FVector::ZeroVector;

		float CellSize = 400.0f;

		const FLayout& Layout;

		/** 部屋ごとのプレハブアクター（Layout.Rooms と同順、グレイボックスでは空） */
		const TArray<ARoomPrefab*>& RoomActors;

		/** 乱数は全段で1本を共有する（消費順がマップの再現性を決める） */
		FRandomStream& Rand;

		/** ログの先頭に付ける名前（生成器アクター名） */
		FString LogOwner;

		FContext(UWorld* InWorld, const FVector& InOrigin, float InCellSize, const FLayout& InLayout,
			const TArray<ARoomPrefab*>& InRoomActors, FRandomStream& InRand, const FString& InLogOwner)
			: World(InWorld), Origin(InOrigin), CellSize(InCellSize), Layout(InLayout)
			, RoomActors(InRoomActors), Rand(InRand), LogOwner(InLogOwner)
		{
		}

		/** セル中心のワールド座標（Z は床面からの高さ） */
		FVector CellCenter(const FIntPoint& Cell, float Z = 0.0f) const;

		/** ワールド座標を含むセル */
		FIntPoint WorldToCell(const FVector& Location) const;

		/** Cell の Dir 側の辺の中点（床面） */
		FVector EdgeCenter(const FIntPoint& Cell, const FIntPoint& Dir) const;

		/** 部屋のプレハブアクター（無ければ nullptr） */
		ARoomPrefab* GetRoomActor(int32 RoomIdx) const;

		/** 重なりを無視して必ずスポーンする */
		AActor* SpawnActor(UClass* Class, const FVector& Location, const FRotator& Rotation) const;

		template <typename T>
		T* SpawnActor(UClass* Class, const FVector& Location, const FRotator& Rotation) const
		{
			return Cast<T>(SpawnActor(Class, Location, Rotation));
		}
	};
}
