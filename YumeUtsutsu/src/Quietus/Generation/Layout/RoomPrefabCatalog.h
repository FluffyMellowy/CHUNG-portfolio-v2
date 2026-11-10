#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "Generation/RoomPrefab.h"

class UWorld;

namespace LevelGen
{
	/** プレハブ内の扉ソケット（アクター原点からの相対位置と外向き方向） */
	struct FPrefabSocket
	{
		FString Name;
		FVector LocalLocation = FVector::ZeroVector;
		FVector Forward = FVector::ForwardVector;
	};

	/** プレハブ1種の配置に必要な情報 */
	struct FPrefabEntry
	{
		TSubclassOf<ARoomPrefab> Class;
		FName ThemeName = NAME_None;
		FIntPoint Footprint = FIntPoint::ZeroValue;
		bool bCanBeStart = false;
		bool bCanBeExit = false;

		/** 1マップ内の最大配置数（0で無制限） */
		int32 MaxInstances = 0;

		TArray<FPrefabSocket> Sockets;

		bool IsValid() const { return Class != nullptr; }
	};

	/**
	 *  部屋プレハブの一覧と、各プレハブの寸法・フラグ・扉ソケット配置。
	 *  ソケットはBPで追加したコンポーネントのためCDOからは読めず、
	 *  生成前に1回だけ各プレハブを一時スポーンして読み取る。
	 *  null の要素も番号を保つため空エントリとして残す（抽選の再現性のため）。
	 */
	class FRoomPrefabCatalog
	{
	public:
		void Build(UWorld* World, const TArray<TSubclassOf<ARoomPrefab>>& Classes);

		int32 Num() const { return Entries.Num(); }
		const FPrefabEntry& Get(int32 Index) const { return Entries[Index]; }

		/** 指定プレハブを Origin（部屋の最小コーナー）にスポーンする */
		ARoomPrefab* SpawnRoom(UWorld* World, int32 Index, const FVector& Origin) const;

	private:
		TArray<FPrefabEntry> Entries;
	};
}
