#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interactables/Interactable.h"
#include "ExitDoor.generated.h"

class UMaterialInstanceDynamic;
class UStaticMesh;
class UStaticMeshComponent;

/** 溝の数ごとの扉板（1/3/5 穴）。メッシュは溝の位置に Shard_00..04 のソケットを持つ */
USTRUCT(BlueprintType)
struct FExitDoorVariant
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 SlotCount = 5;

	/** 左扉（正面 +X から見て左。原点 = 外側の蝶番・床・厚み中央） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UStaticMesh> LeafL;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UStaticMesh> LeafR;
};

/**
 *  脱出扉（夢の欠片の溝が彫られた石の観音開き）。壁・枠込みでタイル辺1本（400cm）を占める。
 *  - E で手持ちの欠片を溝にはめる。はめた溝は欠片が収まり、縁が光る
 *  - 必要数がはまると、溝の無い裏側（ローカル -X、階段側）へ両扉が開く
 *  - 中央の溝 00 は両扉の継ぎ目にあり、欠片は半分ずつ各扉に付いて一緒に動く
 *
 *  座標: 原点 = 開口中央の床、+X = 溝のある正面（プレイヤー側）。
 *  FBX の Y が反転して入るため、左扉の蝶番はローカル +Y=100、右扉は -Y=100。
 *  メッシュ類は BP で設定する。
 */
UCLASS()
class AExitDoor : public AActor, public IInteractable
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> Root;

	/** 開口付きの壁（幅 400cm。メッシュは幅 +X で作られているので 90° 回して置く） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> WallMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> FrameMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> LeafL;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> LeafR;

protected:

	/** 溝の数（1/3/5）。生成器が難易度に合わせて設定する */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ExitDoor", meta = (ClampMin = 1, ClampMax = 5))
	int32 SlotCount = 3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ExitDoor|Appearance")
	TArray<FExitDoorVariant> Variants;

	/** 溝 01..04 にはめる欠片 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ExitDoor|Appearance")
	TObjectPtr<UStaticMesh> ShardMesh;

	/** 中央の溝 00 の左右の半分 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ExitDoor|Appearance")
	TObjectPtr<UStaticMesh> ShardHalfL;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ExitDoor|Appearance")
	TObjectPtr<UStaticMesh> ShardHalfR;

	/** 溝の縁のマテリアルスロット名（Fill00..04 で溝ごとに点灯） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ExitDoor|Appearance")
	FName RimMaterialSlot = TEXT("M_ExitDoor_SocketRim");

	/** 全開の角度（度） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ExitDoor|Open", meta = (ClampMin = 1, ClampMax = 120))
	float OpenAngle = 90.0f;

	/** 最後の欠片がはまってから開き始めるまで（秒）。縁が光り切るのを見せる間 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ExitDoor|Open", meta = (ClampMin = 0, Units = "s"))
	float OpenDelay = 1.2f;

	/** 欠片がはまったとき、溝の縁が灯り切るまで（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ExitDoor|Appearance", meta = (ClampMin = 0.01, Units = "s"))
	float RimLightUpTime = 0.7f;

	/** 灯る途中の一瞬の明るさの上乗せ（1 = 定常の 2 倍まで跳ねる） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ExitDoor|Appearance", meta = (ClampMin = 0))
	float RimFlash = 1.5f;

	/** 開き切るまで（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ExitDoor|Open", meta = (ClampMin = 0.1, Units = "s"))
	float OpenDuration = 3.0f;

	/** 開き始めに各扉を外側へずらす量 (cm)。中央の欠片の断面同士が当たらないようにする */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ExitDoor|Open", meta = (ClampMin = 0, Units = "cm"))
	float LeafOutset = 0.15f;

	/** この角度までにずらし終える（度） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ExitDoor|Open", meta = (ClampMin = 0.01))
	float OutsetAngle = 1.0f;

public:
	AExitDoor();

	/** 溝の数を決める（扉板の差し替え・欠片の用意・GameState への必要数の登録） */
	void SetSlotCount(int32 InSlotCount);

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

	//~ Begin IInteractable
	virtual void Interact_Implementation(APlayerCharacter* Interactor) override;
	virtual FText GetInteractionPrompt_Implementation() const override;
	//~ End IInteractable

protected:
	virtual void BeginPlay() override;

private:
	/** 溝1つ分の見た目（中央は左右2つの欠片） */
	struct FSlotVisual
	{
		int32 Id = 0;
		TArray<TObjectPtr<UStaticMeshComponent>> Shards;
	};

	const FExitDoorVariant* FindVariant() const;
	void ApplyLeafMeshes();
	void BuildSlots();
	UStaticMeshComponent* AddShard(UStaticMeshComponent* Leaf, int32 SlotId, UStaticMesh* Mesh);
	void FillSlot(const FSlotVisual& Slot);
	void SetLeafAngle(float Angle);

	/** 灯っている途中の溝（Fill パラメータ名と経過時間） */
	struct FRimFade
	{
		FName Param;
		float Elapsed = 0.0f;
	};

	void TickRimFades(float DeltaSeconds);

	TArray<FSlotVisual> Slots;
	TArray<FRimFade> RimFades;
	int32 NumFilled = 0;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> RimMaterials;

	bool bSlotsBuilt = false;
	bool bOpening = false;
	float OpenElapsed = 0.0f;
};
