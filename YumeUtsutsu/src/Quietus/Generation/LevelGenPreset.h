#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Generation/GenerationSettings.h"
#include "LevelGenPreset.generated.h"

/**
 *  手続き生成マップの設定一式（部屋・構造キット・照明・目標・扉・敵・アイテム）。
 *  生成器（ALevelGenerator）はこのアセットを参照して生成する。
 *  テーマ（構造キットや部屋プレハブ）や難易度を変えるときは、生成器ではなくこのアセットを差し替える。
 */
UCLASS(BlueprintType)
class ULevelGenPreset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	ULevelGenPreset();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Generator")
	FGenLayoutSettings Layout;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Generator")
	FGenGeometrySettings Geometry;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Generator")
	FGenLightingSettings Lighting;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Generator")
	FGenObjectiveSettings Objectives;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Generator")
	FGenDoorSettings Doors;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Generator")
	FGenEnemySettings Enemies;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Generator")
	FGenItemSettings Items;

	/** 部屋プレハブが設定されていればプレハブモード、空ならグレイボックスの矩形部屋 */
	bool IsPrefabMode() const { return Layout.RoomPrefabClasses.Num() > 0; }
};
