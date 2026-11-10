#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnemyCharacter.generated.h"

class UStaticMeshComponent;
class UKillSequence;

/** 足音の鳴らし方 */
UENUM()
enum class EEnemyFootsteps : uint8
{
	None,
	/** 靴音だけ */
	StepOnly,
	/** 靴音＋鈴（＋ときどき靴のきしみ）。赤いピエロ */
	ClownBells,
};

/**
 *  徘徊する敵のポーン。
 *  判断は AEnemyController（ビヘイビアツリー）、アニメーションは AnimBP（UEnemyAnimInstance 派生）が担当し、
 *  このクラスは移動特性だけを持つ。
 *  メッシュと AnimBP は BP_Enemy で設定し、見た目の違う敵は BP_Enemy を派生させて作る（例: BP_Enemy_ClownRed）。
 *  武器（赤いピエロの斧など）は本体メッシュとは別のスタティックメッシュとして、メッシュのソケットに付ける。
 */
UCLASS()
class AEnemyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AEnemyCharacter();

	virtual void OnConstruction(const FTransform& Transform) override;

	/** 武器を持っているか（AnimBP が持ち手のポーズを重ねるかどうかに使う） */
	bool HasWeapon() const;

	UStaticMeshComponent* GetWeaponMesh() const { return WeaponMesh; }

	/** 足が床に着いた（歩行・走行アニメのノティファイから呼ばれる） */
	void PlayFootstep();

	UKillSequence* GetKillSequence() const { return KillSequence; }

	/**
	 *  捕まえた相手（視点 VictimEye）の正面に立ち、捕獲演出のアニメを流す。判断と移動はここで止める。
	 *  FromDirection は相手から見て自分が立つ向き（ゼロなら今いる向き）。そこが壁の中なら空いている向きへずらす。
	 *  演出データがなければ何もせず false
	 */
	bool StartKill(const AActor* Victim, const FVector& VictimEye, const FVector& FromDirection = FVector::ZeroVector);

protected:
	/** 手に持たせる武器。メッシュが空なら何も持たない。移動の邪魔にならないよう当たり判定なし */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon")
	TObjectPtr<UStaticMeshComponent> WeaponMesh;

	/** 武器を付ける本体メッシュのソケット */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	FName WeaponSocket = TEXT("SOCKET_Axe_R");

	/** 足音の種類。音の決まっていない敵は None（無音） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Sound")
	EEnemyFootsteps Footsteps = EEnemyFootsteps::None;

	/** プレイヤーを捕まえたときの演出。空なら暗転だけ */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Kill")
	TObjectPtr<UKillSequence> KillSequence;
};
