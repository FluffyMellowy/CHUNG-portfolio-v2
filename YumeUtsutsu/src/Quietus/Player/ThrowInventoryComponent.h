#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ThrowInventoryComponent.generated.h"

class AThrowable;

/**
 *  拾った投擲物の所持と投擲。
 *  拾った物のクラスを覚えておき、投げるときに同じクラス（同じ見た目）をスポーンして飛ばす。
 *  どこから・どちらへ投げるか（視点）は持ち主が渡す。
 */
UCLASS(ClassGroup=(Quietus), meta=(BlueprintSpawnableComponent))
class UThrowInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

protected:

	/** 拾った物のクラスが分からないときに投げる投擲物クラス */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Throwable")
	TSubclassOf<AThrowable> ThrowableClass;

	/** 投擲初速 (cm/s) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Throwable", meta = (ClampMin = 0, Units = "cm/s"))
	float ThrowSpeed = 1100.0f;

	/** 視点から投擲物スポーン位置までの前方距離 (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Throwable", meta = (ClampMin = 0, Units = "cm"))
	float ThrowSpawnOffset = 60.0f;

	/** 投擲時に足す上向き初速 (cm/s)。山なりの軌道を作る */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Throwable", meta = (ClampMin = 0, Units = "cm/s"))
	float ThrowUpwardBoost = 150.0f;

	/** 所持数 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="Throwable")
	int32 ThrowableCount = 0;

	/** 最後に拾った投擲物のクラス（投げるとき同じクラスで出す） */
	UPROPERTY(Transient)
	TSubclassOf<AThrowable> CarriedClass;

public:
	UThrowInventoryComponent();

	/** 投擲物を拾う（クラスを記憶して所持数を増やす） */
	void Add(TSubclassOf<AThrowable> PickedClass);

	/**
	 *  1つ投げる。所持していなければ何もしない。
	 *  @param ViewLocation   視点（カメラ）位置。ここから ThrowSpawnOffset 前方にスポーンする
	 *  @param ViewDirection  視線方向（正規化済み）
	 *  @return 投げられたか
	 */
	bool Throw(const FVector& ViewLocation, const FVector& ViewDirection);

	UFUNCTION(BlueprintPure, Category="Throwable")
	int32 GetCount() const { return ThrowableCount; }

	/** 手元の投擲物のクラス（アイテム欄のアイコンと手に持った見た目に使う）。まだ拾っていなければ既定のクラス */
	TSubclassOf<AThrowable> GetCarriedClass() const;
};
