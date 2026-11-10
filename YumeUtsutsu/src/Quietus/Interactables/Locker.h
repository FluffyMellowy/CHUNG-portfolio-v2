#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interactables/HidingSpot.h"
#include "Interactables/Interactable.h"
#include "Locker.generated.h"

class UStaticMeshComponent;
class APlayerCharacter;

/**
 *  隠れ用ロッカー（GDD §3 Hiding / §13-③）。
 *  E で中に入り、もう一度 E で出る。中にいる間は移動不可・扉が視界を塞ぐ。
 *  敵に目撃されたまま隠れると、捜索中の敵がロッカーへ歩み寄り扉をこじ開けて捕獲する
 *  （こじ開け判定は敵AIコントローラ側が IHidingSpot::BustOpen を呼ぶ。敵は UHidingSpotSubsystem 経由で探す）。
 *  メッシュ規約: ボディ=前面下端中央ピボット・開口はローカル+Y面、
 *  扉=蝶番下端ピボット・幅+Y・厚みX（ヒンジはボディ開口の左端に置く）。
 */
UCLASS()
class ALocker : public AActor, public IInteractable, public IHidingSpot
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	/** 扉の蝶番。開閉はこの Yaw を回す */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> DoorPivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> DoorMesh;

protected:

	/** 全開時の扉 Yaw（度）。正で外側へ開く */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Locker", meta = (ClampMin = -180, ClampMax = 180))
	float OpenAngle = 110.0f;

	/** 入っている間の半開き角（わずかな隙間から外が見える） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Locker", meta = (ClampMin = 0, ClampMax = 45))
	float AjarAngle = 8.0f;

	/** 扉の開閉速度（度/秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Locker", meta = (ClampMin = 1))
	float DoorSpeed = 240.0f;

	/** 中に隠れているプレイヤー（無人ならnull） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="Locker")
	TObjectPtr<APlayerCharacter> Occupant = nullptr;

	/** 扉の目標 Yaw（出入り・こじ開けの演出はここを書き換えるだけ） */
	float TargetDoorYaw = 0.0f;

public:
	ALocker();

	virtual void Tick(float DeltaSeconds) override;

	//~ Begin IHidingSpot
	virtual void BustOpen() override;
	virtual bool IsOccupied() const override { return Occupant != nullptr; }
	//~ End IHidingSpot

	//~ Begin IInteractable
	virtual void Interact_Implementation(APlayerCharacter* Interactor) override;
	virtual FText GetInteractionPrompt_Implementation() const override;
	//~ End IInteractable

	/** 中の人を扉の前へ引きずり出す（捕獲演出の前）。扉はこじ開けられたまま、音は鳴らさない */
	void DragOut();

protected:
	/** 敵から見つけてもらえるよう隠れ場所の登録簿へ登録・解除する */
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void Enter(APlayerCharacter* Player);
	void Exit();

	/** 外へ出るときの立ち位置（足元）。正面に床が無ければロッカーのすぐ前 */
	FVector GetExitSpot() const;

	/** 出入りの音（開ける→少し後に閉める） */
	void PlayOpenThenClose();

	FTimerHandle CloseSoundTimer;
};
