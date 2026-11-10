#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SceneComponent.h"
#include "RoomPrefab.generated.h"

/** スポーンマーカーの種別 */
UENUM(BlueprintType)
enum class ESpawnMarkerType : uint8
{
	Shard          UMETA(DisplayName = "DreamShard"),
	Enemy          UMETA(DisplayName = "Enemy"),
	PlayerStart    UMETA(DisplayName = "PlayerStart"),
	PatrolPoint    UMETA(DisplayName = "PatrolPoint"),
	HidingSpot     UMETA(DisplayName = "HidingSpot"),
	DecorMannequin UMETA(DisplayName = "DecorMannequin"),
	Battery        UMETA(DisplayName = "Battery"),
	Throwable      UMETA(DisplayName = "Throwable"),
	Note           UMETA(DisplayName = "Note"),
	Cabinet        UMETA(DisplayName = "Cabinet"),
};

/**
 *  部屋プレハブ内のスポーン位置マーカー。
 *  生成器がこの位置に夢の欠片・敵などを配置する。向きも使われる（敵の初期向きなど）。
 */
UCLASS(ClassGroup = (Quietus), meta = (BlueprintSpawnableComponent))
class USpawnMarkerComponent : public USceneComponent
{
	GENERATED_BODY()

public:

	/** このマーカーに何をスポーンするか */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Marker")
	ESpawnMarkerType MarkerType = ESpawnMarkerType::Shard;
};

/**
 *  部屋プレハブの扉ソケット＝「廊下を接続してよいタイル辺」の宣言。
 *  部屋境界のタイル辺の中央（床面）に置き、ローカル+Xを部屋の外向きにする。
 *  壁・床の実体は生成器がキットピースでスポーンするため、このコンポーネントは位置宣言のみ:
 *  生成器は接続に使う辺には扉付き壁ピース、使わない辺には通常壁ピースを置く。
 */
UCLASS(ClassGroup = (Quietus), meta = (BlueprintSpawnableComponent))
class UDoorSocketComponent : public USceneComponent
{
	GENERATED_BODY()
};

/**
 *  手続き生成用の部屋プレハブ基底クラス。モデリング側はこれをBlueprint継承して
 *  内装ドレッシング（小物・装飾メッシュ）とマーカー・扉ソケットを配置する。
 *  構造体（床・壁・扉開口）は生成器がキットピースでスポーンするため、ここには置かない。
 *
 *  座標規約:
 *  - アクター原点 = 部屋の最小コーナー（-X/-Y角）、部屋は +X/+Y 方向へ広がる
 *  - 1タイル = 400cm（生成器の CellSize と一致させる）、床の歩行面は Z=0
 *  - FootprintTiles にタイル数 W×H を設定（例: 3×2 の部屋なら (3,2)）
 */
UCLASS(Abstract, Blueprintable)
class ARoomPrefab : public AActor
{
	GENERATED_BODY()

protected:

	/** 部屋の占有タイル数（W×H）。配置と衝突判定に使う */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Room")
	FIntPoint FootprintTiles = FIntPoint(3, 2);

	/** テーマ名（ログ・デバッグ表示用） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Room")
	FName ThemeName = NAME_None;

	/** 開始部屋（PlayerStart配置先）候補にしてよいか */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Room")
	bool bCanBeStartRoom = true;

	/** 脱出部屋（脱出扉の配置先）候補にしてよいか */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Room")
	bool bCanBeExitRoom = true;

	/** 1マップ内の最大配置数。0で無制限。開始・脱出テーマは1を推奨 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Room", meta = (ClampMin = 0))
	int32 MaxInstancesPerMap = 0;

public:
	ARoomPrefab();

	FIntPoint GetFootprintTiles() const { return FootprintTiles; }
	FName GetThemeName() const { return ThemeName; }
	bool CanBeStartRoom() const { return bCanBeStartRoom; }
	bool CanBeExitRoom() const { return bCanBeExitRoom; }
	int32 GetMaxInstancesPerMap() const { return MaxInstancesPerMap; }

	/** 扉ソケットを列挙する */
	void GetDoorSockets(TArray<UDoorSocketComponent*>& OutSockets) const;

	/** 指定種別のマーカーを列挙する */
	void GetMarkersOfType(ESpawnMarkerType Type, TArray<USpawnMarkerComponent*>& OutMarkers) const;
};
