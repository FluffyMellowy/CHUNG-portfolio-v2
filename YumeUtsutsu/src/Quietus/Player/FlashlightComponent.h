#pragma once

#include "CoreMinimal.h"
#include "Components/SpotLightComponent.h"
#include "FlashlightComponent.generated.h"

/**
 *  バッテリー管理付きの懐中電灯。
 *  SpotLightComponent を継承し、ライト自身が点灯状態とバッテリー消費を持つ。
 *  明るさ・コーン角・減衰はプロジェクト設定の Dream Lighting が正（BP の値は BeginPlay で上書きされる）。
 *  持ち主が地上を移動している間は、歩調に合わせて光源を揺らす（ボブ）。
 */
UCLASS(ClassGroup=(Quietus), meta=(BlueprintSpawnableComponent))
class UFlashlightComponent : public USpotLightComponent
{
	GENERATED_BODY()

protected:

	/** バッテリー最大値 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Flashlight", meta = (ClampMin = 0))
	float MaxBattery = 100.0f;

	/** 現在のバッテリー (0〜MaxBattery)。BeginPlayで初期化。HUDから参照する */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="Flashlight")
	float Battery = 0.0f;

	/** 点灯中の毎秒バッテリー消費量 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flashlight", meta = (ClampMin = 0))
	float BatteryDrainRate = 0.5f;

	/** 点灯しているか */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="Flashlight")
	bool bIsOn = false;

	/** スポーン時に点灯した状態で始めるか（屋内照明が暗いため既定でON） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flashlight")
	bool bStartOn = true;

	// ---------------------------------------------------------------
	// ボブ (Bob) — 歩行/走行時に光源を揺らす
	// ---------------------------------------------------------------

	/** 最大振幅 (cm)。実際の振幅は速度比（BobReferenceSpeedで最大）でスケールする */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flashlight|Bob", meta = (ClampMin = 0, Units = "cm"))
	float BobAmplitude = 2.5f;

	/** 縦の首振り角の最大値（度）。ビームの見た目の揺れはほぼこれで決まる */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flashlight|Bob", meta = (ClampMin = 0, ClampMax = 10))
	float BobPitchAmplitude = 1.4f;

	/** 横の首振り角の最大値（度） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flashlight|Bob", meta = (ClampMin = 0, ClampMax = 10))
	float BobYawAmplitude = 0.7f;

	/** 歩行時の揺れ周波数 (Hz) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flashlight|Bob", meta = (ClampMin = 0))
	float BobWalkFrequency = 1.6f;

	/** 最高速時の揺れ周波数 (Hz) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flashlight|Bob", meta = (ClampMin = 0))
	float BobSprintFrequency = 2.6f;

	/** 揺れの立ち上がり/減衰の速さ */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flashlight|Bob", meta = (ClampMin = 0.1))
	float BobBlendSpeed = 6.0f;

private:
	/** 振幅・周波数が最大になる移動速度 (cm/s)。持ち主が SetBobReferenceSpeed でスプリント速度を渡す */
	float BobReferenceSpeed = 550.0f;

	/** 位相（sin入力）と現在の振幅エンベロープ */
	float BobPhase = 0.0f;
	float BobEnvelope = 0.0f;

	/** ボブの基準となる初期相対位置・回転（BeginPlayで取得。BPで調整された取り付けオフセットを尊重する） */
	FVector BobBaseOffset = FVector::ZeroVector;
	FRotator BobBaseRotation = FRotator::ZeroRotator;

public:
	UFlashlightComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** on/off切り替え。バッテリー切れの場合は点灯しない */
	UFUNCTION(BlueprintCallable, Category="Flashlight")
	void Toggle();

	/** バッテリーを回復する（MaxBatteryでクランプ）。電池アイテムが呼ぶ */
	UFUNCTION(BlueprintCallable, Category="Flashlight")
	void Recharge(float Amount);

	UFUNCTION(BlueprintPure, Category="Flashlight")
	bool IsOn() const { return bIsOn; }

	UFUNCTION(BlueprintPure, Category="Flashlight")
	float GetBattery() const { return Battery; }

	UFUNCTION(BlueprintPure, Category="Flashlight")
	float GetMaxBattery() const { return MaxBattery; }

	void SetBobReferenceSpeed(float Speed) { BobReferenceSpeed = FMath::Max(Speed, 1.0f); }

	/** Dream Lighting の値を光に反映する */
	void ApplyLightingSettings();

protected:
	virtual void BeginPlay() override;

	/** 点灯中はバッテリーを消費し、切れたら強制消灯する */
	void UpdateBattery(float DeltaTime);

	/** 持ち主の移動速度に応じて光源を揺らす */
	void UpdateBob(float DeltaTime);
};
