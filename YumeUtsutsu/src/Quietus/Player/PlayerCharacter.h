#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "PlayerCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class AThrowable;
class UCameraComponent;
class UAudioComponent;
class UFlashlightComponent;
class UInteractionComponent;
class UStaminaComponent;
class UPointLightComponent;
class UPostProcessComponent;
class UThrowInventoryComponent;
class UInputAction;
struct FInputActionValue;

/**
 *  Quietus プレイヤーキャラクター（一人称ホラー）
 *  入力の受け口と、移動状態（歩行／スプリント／しゃがみ／ロッカー内）およびそれに応じたノイズ発生を担当する。
 *  それ以外の責務はコンポーネントに分けてあり、ここでは入力を委譲するだけ:
 *   - UFlashlightComponent     懐中電灯（バッテリー・歩行時の揺れ）
 *   - UInteractionComponent    視線インタラクション（トレース／フォーカス／実行）
 *   - UStaminaComponent        スプリント用スタミナ
 *   - UThrowInventoryComponent 投擲物の所持と投擲
 */
UCLASS(abstract)
class APlayerCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Pawn mesh: first person view (arms; seen only by self) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> FirstPersonMesh;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FirstPersonCameraComponent;

	/** 懐中電灯（バッテリー管理込み）。カメラに取り付けて視線に追従させる */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UFlashlightComponent> Flashlight;

	/** 懐中電灯を消しても周囲の形が分かるようにする影なしの弱い光。値は Dream Lighting */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPointLightComponent> DarkFillLight;

	/** 自動露出。レベルの PostProcessVolume より優先する。値は Dream Lighting */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPostProcessComponent> ExposureOverride;

	/** 視線インタラクション（トレース／フォーカス／実行） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInteractionComponent> Interaction;

	/** スプリント用スタミナ */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaminaComponent> StaminaComponent;

	/** 投擲物の所持と投擲 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UThrowInventoryComponent> ThrowInventory;

protected:

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	TObjectPtr<UInputAction> JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	TObjectPtr<UInputAction> MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	TObjectPtr<UInputAction> LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	TObjectPtr<UInputAction> MouseLookAction;

	/** スプリント入力アクション（ホールド式）。アセットはエディタで割り当てる */
	UPROPERTY(EditAnywhere, Category ="Input")
	TObjectPtr<UInputAction> SprintAction;

	/** しゃがみ入力アクション（トグル式） */
	UPROPERTY(EditAnywhere, Category ="Input")
	TObjectPtr<UInputAction> CrouchAction;

	/** 懐中電灯入力アクション（トグル式） */
	UPROPERTY(EditAnywhere, Category ="Input")
	TObjectPtr<UInputAction> FlashlightAction;

	/** インタラクト入力アクション（単発） */
	UPROPERTY(EditAnywhere, Category ="Input")
	TObjectPtr<UInputAction> InteractAction;

	/** 投擲入力アクション（単発） */
	UPROPERTY(EditAnywhere, Category ="Input")
	TObjectPtr<UInputAction> ThrowAction;

	// ---------------------------------------------------------------
	// 移動 (Movement)
	// ---------------------------------------------------------------

	/** 通常歩行速度 (cm/s) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Player|Movement", meta = (ClampMin = 0, Units = "cm/s"))
	float WalkSpeed = 250.0f;

	/** スプリント速度 (cm/s) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Player|Movement", meta = (ClampMin = 0, Units = "cm/s"))
	float SprintSpeed = 550.0f;

	/** スプリント中かどうか。変更は必ずDoSprintStart/DoSprintEnd経由で行う */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Player|Movement")
	bool bIsSprinting = false;

	/** ロッカー内に隠れているか（ロッカー側が設定。捕獲判定・ノイズが抑止される） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="Player|Movement")
	bool bHiddenInLocker = false;

	/** 隠れているロッカー。中からの E は視線トレースが箱の内側で空振りするため、
	 *  ここへ直接ルーティングして「出る」を成立させる */
	UPROPERTY(Transient)
	TObjectPtr<AActor> HiddenLocker = nullptr;

	// ---------------------------------------------------------------
	// ノイズ (Noise) — 敵AIの聴覚感知用
	// ---------------------------------------------------------------

	/** スプリント時のノイズの大きさ（AI聴覚の基準半径に対する倍率） */
	UPROPERTY(EditAnywhere, Category="Player|Noise", meta = (ClampMin = 0))
	float SprintLoudness = 1.0f;

	/** 歩行時のノイズの大きさ */
	UPROPERTY(EditAnywhere, Category="Player|Noise", meta = (ClampMin = 0))
	float WalkLoudness = 0.5f;

	/** しゃがみ歩行時のノイズの大きさ */
	UPROPERTY(EditAnywhere, Category="Player|Noise", meta = (ClampMin = 0))
	float CrouchLoudness = 0.15f;

	/** ノイズイベントの発生間隔。毎フレーム発行するとAI Perceptionに負荷がかかるため間引く */
	UPROPERTY(EditAnywhere, Category="Player|Noise", meta = (ClampMin = 0.05, Units = "s"))
	float NoiseInterval = 0.4f;

#if WITH_EDITORONLY_DATA
	/** 自動テスト用フック: trueの間、静止していてもスプリント相当のノイズを発し続ける（エディタ専用） */
	UPROPERTY(EditAnywhere, Transient, Category="Debug")
	bool bDebugEmitNoise = false;

	/** 自動テスト用フック: trueにすると1回だけ投擲を実行する（エディタ専用） */
	UPROPERTY(EditAnywhere, Transient, Category="Debug")
	bool bDebugThrow = false;
#endif

private:

	/** ノイズ発生用の内部タイマー */
	float NoiseTimer = 0.0f;

	/** 前の足音から進んだ距離 (cm) */
	float StepDistance = 0.0f;

	/** 走っている間の息と、息切れの息（どちらもループ） */
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> BreathRunLoop;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> BreathExhaustedLoop;

	bool bBreathRun = false;
	bool bBreathExhausted = false;
	float BreathRunHold = 0.0f;

#if !UE_BUILD_SHIPPING
	/** 確認用の自動歩行（残り秒数） */
	float DebugMoveLeft = 0.0f;
	float DebugStuckTime = 0.0f;

	/** 紹介動画の撮影用: 残りの経由点と、歩き終えたら向く位置 */
	TArray<FVector> DebugPath;
	FVector DebugPathLookAt = FVector::ZeroVector;
	float DebugPathStuckTotal = 0.0f;

	/** 紹介動画の撮影用: ゆっくり向く先 */
	TOptional<FRotator> DebugLookTarget;
	float DebugLookSpeed = 2.0f;

	void TickDebugPath(float DeltaSeconds);
#endif

public:
	APlayerCharacter();

	/** スタミナとノイズを毎フレーム更新する */
	virtual void Tick(float DeltaSeconds) override;

#if !UE_BUILD_SHIPPING
	/** 確認用: 前へ Seconds 秒歩かせる。Mode 0 = 歩き / 1 = 走り / 2 = しゃがみ */
	void DebugMove(int32 Mode, float Seconds);

	/** 紹介動画の撮影用: 経由点を順に歩き、終わったら LookAt を向く */
	void DebugWalkPath(const TArray<FVector>& Points, int32 Mode, const FVector& LookAt);

	/** 紹介動画の撮影用: 指定の向きへゆっくり向く */
	void DebugLook(const FRotator& Rotation, float Speed);

	/** 紹介動画の撮影用: 経路を歩いている途中か */
	bool IsDebugWalking() const { return DebugPath.Num() > 0; }
#endif

protected:

	/** ランタイム状態（移動速度）の初期化とコンポーネントの接続 */
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Dream Lighting の値を懐中電灯・補助光・露出に反映する。PIE 中の変更でも呼ばれる */
	void ApplyLightingSettings();

#if WITH_EDITOR
	void HandleLightingChanged(UObject* Settings, struct FPropertyChangedEvent& Event) { ApplyLightingSettings(); }
	FDelegateHandle LightingChangedHandle;
#endif

	/** Called from Input Actions for movement input */
	void MoveInput(const FInputActionValue& Value);

	/** Called from Input Actions for looking input */
	void LookInput(const FInputActionValue& Value);

	/** Handles aim inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoAim(float Yaw, float Pitch);

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles jump start inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump end inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

	/** スプリント開始。スタミナが開始に足りるときのみ成功する */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoSprintStart();

	/** スプリント終了（キーを離した／スタミナが枯渇した） */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoSprintEnd();

	/** しゃがみ切り替え */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoCrouchToggle();

	/** 懐中電灯のon/off切り替え（Flashlightコンポーネントへ委譲） */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoFlashlightToggle();

public:
	/** 視線の先のインタラクト対象に作用する（Interactionコンポーネントへ委譲）。
	 *  デバッグフックが実キー入力と同じ経路を踏むため public */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoInteract();

protected:
	/** 所持している投擲物を視線方向へ投げる（ThrowInventoryコンポーネントへ委譲） */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoThrow();

	/** Set up input action bindings */
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;

	/** Tick内サブ処理: 実際に走っている間だけスタミナを消費させる */
	void UpdateStamina(float DeltaSeconds);

	/** Tick内サブ処理: 移動状態に応じたノイズイベントの定期発行 */
	void UpdateNoise(float DeltaSeconds);

	/** Tick内サブ処理: 足音（ノイズと同じ条件で、進んだ距離ごとに鳴らす）と息 */
	void UpdateFootsteps(float DeltaSeconds);
	void UpdateBreath();

public:

	/** 現在の移動状態のノイズの大きさを返す（静止=0、しゃがみ<歩行<スプリント） */
	UFUNCTION(BlueprintPure, Category="Player|Noise")
	float GetCurrentNoiseLevel() const;

	// HUD用のゲッター群（コンポーネント分の値はファサードとして委譲する）
	UFUNCTION(BlueprintPure, Category="Player|Stamina")
	float GetStamina() const;

	UFUNCTION(BlueprintPure, Category="Player|Stamina")
	float GetMaxStamina() const;

	UFUNCTION(BlueprintPure, Category="Player|Stamina")
	bool IsStaminaExhausted() const;

	/** ロッカー隠れ状態の設定・照会（ALocker が呼ぶ） */
	void SetHiddenInLocker(bool bInHidden, AActor* InLocker = nullptr)
	{
		bHiddenInLocker = bInHidden;
		HiddenLocker = bInHidden ? InLocker : nullptr;
	}

	UFUNCTION(BlueprintPure, Category="Player|Movement")
	bool IsHiddenInLocker() const { return bHiddenInLocker; }

	/** 隠れているロッカー（隠れていなければ null） */
	AActor* GetHiddenLocker() const { return HiddenLocker; }

	UFUNCTION(BlueprintPure, Category="Player|Flashlight")
	float GetBattery() const;

	UFUNCTION(BlueprintPure, Category="Player|Flashlight")
	float GetMaxBattery() const;

	/** 現在のインタラクト対象のプロンプト文字列を返す（対象がなければ空）。HUD用 */
	UFUNCTION(BlueprintPure, Category="Player|Interaction")
	FText GetInteractionPrompt() const;

	/** Returns the first person mesh **/
	USkeletalMeshComponent* GetFirstPersonMesh() const { return FirstPersonMesh; }

	/** Returns first person camera component **/
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

	/** 懐中電灯コンポーネントを返す */
	UFlashlightComponent* GetFlashlight() const { return Flashlight; }

	/** インタラクションコンポーネントを返す */
	UInteractionComponent* GetInteraction() const { return Interaction; }

	/** 投擲物を拾う（クラスを記憶して所持数を増やす） */
	void AddThrowable(TSubclassOf<AThrowable> PickedClass);

	UFUNCTION(BlueprintPure, Category="Player|Throwable")
	int32 GetThrowableCount() const;
};
