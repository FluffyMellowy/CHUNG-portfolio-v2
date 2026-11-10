#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LevelGenerator.generated.h"

class UInstancedStaticMeshComponent;
class ARoomPrefab;
class ULevelGenPreset;

namespace LevelGen
{
	struct FLayout;
	struct FContext;
	struct FDoorway;
	struct FGeometryComponents;
	class FLayoutBuilder;
	class FRoomPrefabCatalog;
}

/**
 *  手続き生成マップの生成器。レベルに1つ置くと、ロード時にシード基準で「部屋＋廊下」マップを作る。
 *  設定値は Data Asset（ULevelGenPreset）が持ち、このクラスはシードの決定とパイプラインの進行だけを担う。
 *  各段の処理は LevelGen 名前空間のクラスに委ねる。
 *
 *  パイプライン:
 *   1. レイアウト   部屋配置・接続・廊下・開始/脱出部屋を決める（不成立ならシードを派生させて再試行）
 *   2. 部屋        確定した位置に部屋プレハブをスポーン
 *   3. ジオメトリ   床・壁・天井を ISM に積む
 *   4. 照明        天井がある時だけ室内灯を置く
 *   5. ゲームプレイ  目標・敵・アイテム・家具を配置
 *   6. 扉          扉前の装飾を撤去し、室内ドアを立てる
 *  NavMesh は RuntimeGeneration=Dynamic で、BeginPlay の次フレームに張り直す。
 */
UCLASS()
class ALevelGenerator : public AActor
{
	GENERATED_BODY()

	/** 床タイル用 ISM（キューブ） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInstancedStaticMeshComponent> FloorISM;

	/** 壁セグメント用 ISM（キューブ） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInstancedStaticMeshComponent> WallISM;

	/** キット床ピース用 ISM */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInstancedStaticMeshComponent> KitFloorISM;

	/** キット壁ピース用 ISM */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInstancedStaticMeshComponent> KitWallISM;

	/** キット扉付き壁ピース用 ISM */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInstancedStaticMeshComponent> KitWallDoorISM;

	/** キット天井ピース用 ISM */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInstancedStaticMeshComponent> KitCeilingISM;

protected:

	/** 生成シード。同じ値なら同じマップになる */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Generator|Seed")
	int32 Seed = 12345;

	/** trueならプレイ毎にシードをランダム化する（検証時はfalse推奨） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Generator|Seed")
	bool bRandomizeSeedOnPlay = false;

	/** 生成設定一式。テーマや難易度はこのアセットを差し替えて変える */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Generator")
	TObjectPtr<ULevelGenPreset> Preset;

public:
	ALevelGenerator();

	/** PlayerStart検索より前にマップを作る必要があるため、BeginPlayではなくここで生成する */
	virtual void PreInitializeComponents() override;

	/** ナビシステム初期化後にNavMeshを張り直すため、生成本体とは別にここでも処理する */
	virtual void BeginPlay() override;

	/**
	 *  レイアウトだけを多数のシードで試し、成功率・再試行回数・所要時間をまとめる（生成の安定性確認用）。
	 *  ワールドには部屋の下見用プレハブ以外何も置かない。Python からも呼べる。
	 */
	UFUNCTION(BlueprintCallable, Category="Generator|Debug")
	FString SweepLayouts(int32 FirstSeed = 1, int32 Count = 200) const;

private:
	void Generate();

	/** メニューから渡された予約シード > ランダム > 設定値 の順で決める */
	int32 ResolveSeed() const;

	TUniquePtr<LevelGen::FLayoutBuilder> MakeLayoutBuilder(const ULevelGenPreset& Settings, const LevelGen::FRoomPrefabCatalog& Catalog) const;

	/** シードを派生させながら最大数回レイアウトを試す。成功時の乱数ストリームは後段でも使い続ける */
	bool BuildLayout(const LevelGen::FLayoutBuilder& Builder, int32 BaseSeed, LevelGen::FLayout& OutLayout, FRandomStream& OutRand,
		int32* OutAttempts = nullptr, bool bLog = true) const;

	/** 部屋ごとのプレハブをスポーンする（グレイボックスでは空） */
	TArray<ARoomPrefab*> SpawnRoomActors(const ULevelGenPreset& Settings, const LevelGen::FRoomPrefabCatalog& Catalog, const LevelGen::FLayout& MapLayout) const;

	/** 目標・敵・アイテム・家具・扉を配置する */
	void SpawnGameplayActors(const ULevelGenPreset& Settings, const LevelGen::FContext& Context, const TArray<LevelGen::FDoorway>& Doorways) const;

	LevelGen::FGeometryComponents GetGeometryComponents() const;

	void RefreshNavigation();
};
