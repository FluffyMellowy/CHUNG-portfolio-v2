#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interactables/Interactable.h"
#include "Door.generated.h"

class UStaticMesh;
class UStaticMeshComponent;
class UBoxComponent;

/**
 *  部屋と廊下の開口部に立つ観音開きのドア。
 *  - 生成時は開いた状態（開き具合は生成器がシードでばらつかせる）で自由通行
 *  - プレイヤーは E で閉められる（追跡を遅らせる防御手段）
 *  - 敵は閉まったドアの前で EnemyOpenDelay 待ってから開けて通る
 *  - NavMesh には影響させない（経路は常に開口部を通る。閉扉は物理と開閉遅延で表現）
 *  蝶番ピボットを回転させ、パネルはそこにぶら下げる構造。
 *
 *  見た目はBPで決める。片開きは SingleLeafMesh を設定すれば蝶番位置をメッシュのバウンドから自動で合わせる。
 *  観音開きなどそれ以外の構成は、BPで DoorMesh / DoorMesh2 のメッシュと配置を直接設定する。
 */
UCLASS()
class ADoor : public AActor, public IInteractable
{
	GENERATED_BODY()

	/** ルート。開口部の中心（床面）を表す。+Xが壁の走る方向 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> Root;

	/** 左蝶番（ローカル -X 側）。開閉時はこのコンポーネントの Yaw を回す */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> DoorPivot;

	/** 左パネルのメッシュ */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> DoorMesh;

	/** 右蝶番（ローカル +X 側）。片開き運用時は使わない */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> DoorPivot2;

	/** 右パネルのメッシュ */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> DoorMesh2;

	/** 敵の接近を検知して自動開扉するゾーン（開口部の両側をまたぐ） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> EnemyOpenZone;

protected:

	/** 開いたときの Yaw 角度（度）。右パネルは符号反転で同方向に開く */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Door", meta = (ClampMin = -180, ClampMax = 180))
	float OpenAngle = 105.0f;

	/** 開閉速度（度/秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Door", meta = (ClampMin = 1))
	float OpenSpeed = 180.0f;

	/** 現在開いているか（開閉アニメ中も含め、目標状態を表す） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Door")
	bool bIsOpen = true;

	/** スポーン時の開き具合 (0=閉, 1=全開)。生成器がシードでばらつかせる */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Door", meta = (ClampMin = 0, ClampMax = 1))
	float StartOpenRatio = 1.0f;

	/** 敵が閉扉を開けるまでの待ち時間（秒）。プレイヤーが閉めて稼げる時間 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Door", meta = (ClampMin = 0, Units = "s"))
	float EnemyOpenDelay = 1.2f;

	FTimerHandle EnemyOpenTimer;

	/**
	 *  片開きのドア板。設定すると DoorMesh に取り付け、DoorMesh2 は使わない。
	 *  リーフ規約: 原点=蝶番・床・厚み中央、+X=正面、ドア板は-Yへ展開（ピボットが焼かれていなくても可）
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Door|Appearance")
	TObjectPtr<UStaticMesh> SingleLeafMesh;

	/** 片開きドア板のスケール（開口150x210に合わせる係数） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Door|Appearance")
	FVector SingleLeafScale = FVector::OneVector;

public:
	ADoor();

	/** 蝶番の Yaw を目標角度へ毎フレーム補間する */
	virtual void Tick(float DeltaSeconds) override;

	/** 片開き設定があればドア板を取り付ける（BPエディタのプレビューにも反映される） */
	virtual void OnConstruction(const FTransform& Transform) override;

	/** 生成器用: スポーン時の開き具合を設定（BeginPlay前に呼ぶ） */
	void SetStartOpenRatio(float Ratio) { StartOpenRatio = Ratio; }

	/** 補間せずに閉じた状態にする（虹色の扉にするとき） */
	void ForceClose();

	/** 虹色の扉か（URainbowDoorComponent が付いていて未使用） */
	bool IsRainbow() const;

	/** 脱出部屋に面した扉か（虹色の扉の行き先から外す） */
	bool IsInExitRoom() const { return bInExitRoom; }
	void SetInExitRoom(bool bIn) { bInExitRoom = bIn; }

	//~ Begin IInteractable
	virtual void Interact_Implementation(APlayerCharacter* Interactor) override;
	virtual FText GetInteractionPrompt_Implementation() const override;
	//~ End IInteractable

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnZoneBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** 敵の待ち時間経過後に開く */
	void OpenForEnemy();

	/** 開け閉めの音（ForceClose は生成中にしか呼ばれないので鳴らさない） */
	void PlayDoorSound(bool bOpening) const;

	/** ゾーン内に敵がいれば開扉タイマーを起動する（重複起動はしない） */
	void ArmEnemyTimerIfNeeded();

	/** ドア板がPawnを遮るかを切り替える。閉じ切った時だけ遮り、
	 *  開放中・開閉アニメ中は素通し（回転スイープがPawnを巻き込んで
	 *  ドア板内に閉じ込める事故と、開いた板が経路を塞ぐ事故の両方を防ぐ） */
	void SetLeafPawnBlocking(bool bBlock);

private:
	/** 片開きドア板を蝶番位置に合わせて DoorMesh に取り付ける */
	void ApplySingleLeaf();

	bool bInExitRoom = false;
};
