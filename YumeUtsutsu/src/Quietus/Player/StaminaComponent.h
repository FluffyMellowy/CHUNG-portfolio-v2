#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StaminaComponent.generated.h"

/**
 *  スプリント用スタミナ。消費・回復と「枯渇ロック」（枯渇したら満タンまで再スプリント不可）を持つ。
 *  いつ消費するかは持ち主が決めて Update に渡す。枯渇は OnExhausted で通知する。
 */
UCLASS(ClassGroup=(Quietus), meta=(BlueprintSpawnableComponent))
class UStaminaComponent : public UActorComponent
{
	GENERATED_BODY()

protected:

	/** スタミナ最大値 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stamina", meta = (ClampMin = 0))
	float MaxStamina = 100.0f;

	/** 現在のスタミナ (0〜MaxStamina)。BeginPlayで初期化。HUDから参照する */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="Stamina")
	float Stamina = 0.0f;

	/** 消費中の毎秒スタミナ消費量 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stamina", meta = (ClampMin = 0))
	float DrainRate = 20.0f;

	/** 非消費時の毎秒スタミナ回復量 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stamina", meta = (ClampMin = 0))
	float RecoveryRate = 15.0f;

	/** スプリント開始に必要な最低スタミナ（枯渇直後の連発防止） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stamina", meta = (ClampMin = 0))
	float MinToStartSprint = 20.0f;

	/** 枯渇リカバリ中か。trueの間は満タンまでスプリント不可（HUDは赤表示） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="Stamina")
	bool bExhausted = false;

public:
	UStaminaComponent();

	/** 枯渇した瞬間に1回通知する（持ち主はここでスプリントを止める） */
	FSimpleMulticastDelegate OnExhausted;

	/**
	 *  1フレーム分の消費・回復を行う。
	 *  @param bDraining  この間スタミナを使う行動（実際に走っている等）をしていたか
	 */
	void Update(float DeltaSeconds, bool bDraining);

	/** 枯渇ロック中でなく、開始に必要な量が残っているか */
	bool CanStartSprint() const { return !bExhausted && Stamina >= MinToStartSprint; }

	UFUNCTION(BlueprintPure, Category="Stamina")
	float GetStamina() const { return Stamina; }

	UFUNCTION(BlueprintPure, Category="Stamina")
	float GetMaxStamina() const { return MaxStamina; }

	UFUNCTION(BlueprintPure, Category="Stamina")
	bool IsExhausted() const { return bExhausted; }

protected:
	virtual void BeginPlay() override;
};
