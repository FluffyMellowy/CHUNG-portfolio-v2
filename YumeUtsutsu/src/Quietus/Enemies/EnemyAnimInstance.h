#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "EnemyAnimInstance.generated.h"

/**
 *  敵の AnimBP（ABP_Enemy）の親クラス。
 *  ポーンの移動速度を毎フレーム取り出し、AnimGraph のロコモーション用ブレンドスペースへ渡す値として公開する。
 *  武器を持つ敵では、ロコモーションの上に腕と指の持ち手ポーズを重ねる（WeaponCarryAlpha）。
 */
UCLASS()
class UEnemyAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
#if !UE_BUILD_SHIPPING
	/** 確認用: 0 以上なら実際の速度の代わりにこの速度でロコモーションを再生する（負で解除） */
	static float DebugSpeedOverride;
#endif

protected:

	/** 水平方向の移動速度 (cm/s)。待機・歩行・走行のブレンドに使う */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="Locomotion", meta = (Units = "cm/s"))
	float GroundSpeed = 0.0f;

	/** 武器の持ち手ポーズ（右腕の補正ループ＋指）の重み。武器を持たない敵は 0 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="Weapon")
	float WeaponCarryAlpha = 0.0f;

	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
};
