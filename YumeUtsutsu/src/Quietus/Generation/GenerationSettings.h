#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "Interactables/CabinetLootTable.h"
#include "GenerationSettings.generated.h"

class UStaticMesh;
class UMaterialInterface;
class ARoomPrefab;
class ADreamShard;
class AExitDoor;
class AWakeStairs;
class ADoor;
class AEnemyCharacter;
class ABattery;
class AThrowable;
class ANoteItem;
class ACabinet;
class ALocker;
class ABalloon;

/** 部屋の配置と接続（タイルグリッド） */
USTRUCT(BlueprintType)
struct FGenLayoutSettings
{
	GENERATED_BODY()

	/** タイル1辺の長さ (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 100, Units = "cm"))
	float CellSize = 400.0f;

	/** グリッドの大きさ（タイル数） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FIntPoint GridSize = FIntPoint(24, 24);

	/** 部屋数の下限 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 2))
	int32 RoomCountMin = 3;

	/** 部屋数の上限 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 2))
	int32 RoomCountMax = 6;

	/** 部屋の最小サイズ（タイル数、グレイボックスのみ） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FIntPoint RoomSizeMin = FIntPoint(2, 2);

	/** 部屋の最大サイズ（タイル数、グレイボックスのみ） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FIntPoint RoomSizeMax = FIntPoint(4, 3);

	/** 部屋同士の最低間隔（タイル数） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 1))
	int32 RoomGap = 1;

	/** MSTに追加するループ接続の本数 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0))
	int32 ExtraLoops = 1;

	/** 脱出扉の外側に空けておくタイル数（扉の先の階段セットが通る分） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0))
	int32 ExitClearCells = 5;

	/**
	 *  配置候補の部屋プレハブ。1つ以上設定するとプレハブモードになり、
	 *  部屋はプレハブ（footprint＋扉ソケット＋マーカー）で配置される。
	 *  空ならグレイボックスの矩形部屋にフォールバックする。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TSubclassOf<ARoomPrefab>> RoomPrefabClasses;
};

/** 床・壁・天井の構造メッシュ */
USTRUCT(BlueprintType)
struct FGenGeometrySettings
{
	GENERATED_BODY()

	/** キット床ピース（タイルサイズ・ピボット中央・床面Z=0）。未設定ならキューブ床 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UStaticMesh> KitFloorMesh = nullptr;

	/** キット壁ピース（幅KitModuleSize、ピボット下端中央、+Xが長手方向・+Yが室内側）。未設定ならキューブ壁 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UStaticMesh> KitWallMesh = nullptr;

	/** キット扉付き壁ピース（規約は壁ピースと同じ、中央に開口） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UStaticMesh> KitWallDoorMesh = nullptr;

	/** キット天井ピース（タイルサイズ・ピボット中央・下向き面）。未設定なら天井なし */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UStaticMesh> KitCeilingMesh = nullptr;

	/** キットモジュールの1辺 (cm)。タイル1辺はこの整数倍であること */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 50, Units = "cm"))
	float KitModuleSize = 200.0f;

	/** 壁の高さ (cm)。キット規格に合わせて300 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 100, Units = "cm"))
	float WallHeight = 300.0f;

	/** キット未設定時の床・壁に使うメッシュ（100cm基準キューブ想定） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UStaticMesh> TileMesh = nullptr;

	/** キューブ床のマテリアル（未指定ならメッシュのまま） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UMaterialInterface> FloorMaterial = nullptr;

	/** キューブ壁のマテリアル（未指定ならメッシュのまま） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UMaterialInterface> WallMaterial = nullptr;
};

/** 天井下の室内照明（主光源は懐中電灯なので最低限） */
USTRUCT(BlueprintType)
struct FGenLightingSettings
{
	GENERATED_BODY()

	/** 部屋・廊下にライトを置くか（天井がある時のみ有効） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bSpawnRoomLights = true;

	/** 室内ライトの明るさ */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0))
	float RoomLightIntensity = 1500.0f;

	/** 室内・廊下ライトの色（薄暗い電球色） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor RoomLightColor = FLinearColor(1.0f, 0.85f, 0.6f);

	/** 室内ライトの減衰半径 (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0, Units = "cm"))
	float RoomLightRadius = 900.0f;

	/** 廊下ライトの設置間隔（タイル数） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 1))
	int32 CorridorLightInterval = 3;

	/** 廊下ライトの明るさ（部屋より弱め。0で廊下ライトなし） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0))
	float CorridorLightIntensity = 250.0f;

	/** 廊下ライトの減衰半径 (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0, Units = "cm"))
	float CorridorLightRadius = 600.0f;

	/** ライトを天井面から下げる距離 (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0, Units = "cm"))
	float LightDropFromCeiling = 40.0f;
};

/** 脱出目標（夢の欠片 → 脱出扉の溝 → 扉の先の階段）。見た目は各クラスのBPで設定する */
USTRUCT(BlueprintType)
struct FGenObjectiveSettings
{
	GENERATED_BODY()

	/** 欠片の数＝脱出扉の溝の数（1/3/5。夢 / 悪夢 / 猿夢） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 1, ClampMax = 5))
	int32 ShardCount = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<ADreamShard> ShardClass;

	/** 脱出扉の先に置く階段セット */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<AWakeStairs> ExitStairsClass;
};

/** 扉（脱出扉・部屋の出入口ドア・扉前の通行確保）。見た目は各クラスのBPで設定する */
USTRUCT(BlueprintType)
struct FGenDoorSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<AExitDoor> ExitDoorClass;

	/** 部屋-廊下の開口部にドアを立てるか */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bSpawnInteriorDoors = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<ADoor> InteriorDoorClass;

	/** スポーン時の開き具合の範囲（シードでばらつかせる） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0, ClampMax = 1))
	float DoorStartOpenMin = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0, ClampMax = 1))
	float DoorStartOpenMax = 1.0f;

	/** 扉クリアランスの奥行き (cm)。使用扉の内外この距離までにある遮蔽装飾を撤去する */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0, Units = "cm"))
	float DoorClearanceDepth = 140.0f;
};

/** 徘徊敵と巡回ポイント */
USTRUCT(BlueprintType)
struct FGenEnemySettings
{
	GENERATED_BODY()

	/** 配置する徘徊敵の数 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0))
	int32 EnemyCount = 1;

	/** 巡回ポイントの数（タグ PatrolPoint で敵AIが収集する） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 2))
	int32 PatrolPointCount = 4;

	/** スポーンする敵。見た目（メッシュ・AnimBP）はクラス側の BP で決まる */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<AEnemyCharacter> EnemyClass;
};

/** 拾得アイテム（バッテリー／投擲物／ロアメモ）と調べられる家具（キャビネット／ロッカー）。見た目は各クラスのBPで設定する */
USTRUCT(BlueprintType)
struct FGenItemSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0))
	int32 BatteryCount = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<ABattery> BatteryClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0))
	int32 ThrowableCount = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<AThrowable> ThrowableClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0))
	int32 NoteCount = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<ANoteItem> NoteClass;

	/** ロアメモ本文プール（配置順に使い回す） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (MultiLine = true))
	TArray<FText> NoteTexts;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0))
	int32 CabinetCount = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<ACabinet> CabinetClass;

	/** キャビネットを開けたときに出るもの（上の拾得アイテムのクラスを使う） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FCabinetLootTable CabinetLoot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0))
	int32 LockerCount = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<ALocker> LockerClass;

	/** 廊下に浮かべる風船（数は難易度で決まる） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<ABalloon> BalloonClass;

	/** 風船の紐の下端の高さ (cm)。胴体がだいたい目の高さに来るように */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0, Units = "cm"))
	float BalloonFloatHeight = 80.0f;

	/** 本文プールから Index 番目の本文を返す（プールが空なら判読不能の定型文） */
	FText GetNoteBody(int32 Index) const;
};
