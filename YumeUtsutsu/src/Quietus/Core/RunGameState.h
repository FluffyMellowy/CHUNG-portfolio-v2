#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "RunGameState.generated.h"

/** 欠片の数・扉の開放・メモ数など、目標の進行値が変わった */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnObjectiveChanged);

/** 勝敗が確定した（bEscaped=true なら脱出成功、false なら捕獲） */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRoundFinished, bool, bEscaped);

/** プレイヤーが脱出扉の先の安全地帯に入った（以後、敵は追ってこない） */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSafeZoneEntered);

/** プレイヤーが虹色の扉で別の場所へ移った（From = 出発した扉の位置） */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerTeleported, FVector, From);

/** 捕まったが命が残っている（夢を作り直して続ける）。LivesRemaining は減った後の数 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLifeLost, int32, LivesRemaining);

/**
 *  ラン1回分のゲーム進行状態を管理する。
 *  夢の欠片（拾った数・扉にはめた数）・扉の開放・安全地帯・脱出（勝利）を一元的に持ち、
 *  欠片／脱出扉／階段／目覚まし時計の各アクターはここを参照する。
 *  流れ: 欠片を拾う → 脱出扉の溝にはめる → 必要数が揃うと扉が開く → 階段（安全地帯） → 目覚まし時計で脱出
 *
 *  状態の変化はデリゲートで通知する（Observer）。HUD・プレイヤーコントローラ・敵AIは
 *  毎フレーム問い合わせる代わりにこれを購読し、GameState は購読側を知らずに済む。
 */
UCLASS()
class ARunGameState : public AGameStateBase
{
	GENERATED_BODY()

protected:

	/** 扉を開けるのに必要な欠片の数。脱出扉が自分の溝の数で上書きする */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Objective", meta = (ClampMin = 1))
	int32 RequiredShards = 3;

	/** 拾ったがまだ扉にはめていない欠片の数 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="Objective")
	int32 HeldShards = 0;

	/** 扉にはめた欠片の数 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="Objective")
	int32 InsertedShards = 0;

	/** 必要数がはまって脱出扉が開いたか */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="Objective")
	bool bExitOpen = false;

	/** プレイヤーが安全地帯（扉の先の階段）に入ったか */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="Objective")
	bool bInSafeZone = false;

	/** 脱出済みか（勝利確定後の多重処理を防ぐ） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="Objective")
	bool bEscaped = false;

	/** 敵に捕まって命が尽きたか（敗北＝目覚めない） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="Objective")
	bool bGameOver = false;

	/** 捕まったが命が残っていて、夢の作り直しを待っているか */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="Objective")
	bool bLifeLost = false;

	/** 収集済みロアメモ数（進行には無関係の収集要素） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="Objective")
	int32 CollectedNotes = 0;

public:

	/** 目標の進行値（欠片・扉・メモ）が変わったときに通知する */
	UPROPERTY(BlueprintAssignable, Category="Objective")
	FOnObjectiveChanged OnObjectiveChanged;

	/** 勝敗が確定したときに1回だけ通知する */
	UPROPERTY(BlueprintAssignable, Category="Objective")
	FOnRoundFinished OnRoundFinished;

	/** 安全地帯に入ったときに1回だけ通知する */
	UPROPERTY(BlueprintAssignable, Category="Objective")
	FOnSafeZoneEntered OnSafeZoneEntered;

	/** プレイヤーが虹色の扉で移ったときに通知する（追っていたピエロは見失う） */
	UPROPERTY(BlueprintAssignable, Category="Objective")
	FOnPlayerTeleported OnPlayerTeleported;

	/** 捕まったが命が残っているときに1回だけ通知する（この夢はここで終わり） */
	UPROPERTY(BlueprintAssignable, Category="Objective")
	FOnLifeLost OnLifeLost;

	/** ワールドの ARunGameState を返す（このGameStateを使わないレベルでは null） */
	static ARunGameState* Get(const UObject* WorldContextObject);

	/** 欠片を1つ拾う */
	UFUNCTION(BlueprintCallable, Category="Objective")
	void AddShard();

	/** 必要な欠片の数を設定する（脱出扉が溝の数を登録する） */
	UFUNCTION(BlueprintCallable, Category="Objective")
	void SetRequiredShards(int32 Count);

	/**
	 *  手持ちの欠片を扉にはめる（足りない分まで）。必要数が揃ったら扉を開放状態にする。
	 *  @return 今回はめた数
	 */
	UFUNCTION(BlueprintCallable, Category="Objective")
	int32 InsertShards();

	/** プレイヤーが虹色の扉で移った */
	void NotifyPlayerTeleported(const FVector& From) { OnPlayerTeleported.Broadcast(From); }

	/** 安全地帯に入った */
	UFUNCTION(BlueprintCallable, Category="Objective")
	void EnterSafeZone();

	/** 脱出成功（勝利）を確定させる */
	UFUNCTION(BlueprintCallable, Category="Objective")
	void TriggerEscape();

	/** 敵に捕まった。命が残っていれば OnLifeLost、尽きていれば敗北を確定させる */
	UFUNCTION(BlueprintCallable, Category="Objective")
	void TriggerGameOver();

	/** ロアメモを1つ収集する */
	UFUNCTION(BlueprintCallable, Category="Objective")
	void AddNote();

	UFUNCTION(BlueprintPure, Category="Objective")
	int32 GetCollectedNotes() const { return CollectedNotes; }

	UFUNCTION(BlueprintPure, Category="Objective")
	int32 GetHeldShards() const { return HeldShards; }

	UFUNCTION(BlueprintPure, Category="Objective")
	int32 GetInsertedShards() const { return InsertedShards; }

	/** 拾った数とはめた数の合計（HUD 表示用） */
	UFUNCTION(BlueprintPure, Category="Objective")
	int32 GetFoundShards() const { return HeldShards + InsertedShards; }

	UFUNCTION(BlueprintPure, Category="Objective")
	int32 GetRequiredShards() const { return RequiredShards; }

	UFUNCTION(BlueprintPure, Category="Objective")
	bool IsExitOpen() const { return bExitOpen; }

	UFUNCTION(BlueprintPure, Category="Objective")
	bool IsInSafeZone() const { return bInSafeZone; }

	UFUNCTION(BlueprintPure, Category="Objective")
	bool HasEscaped() const { return bEscaped; }

	UFUNCTION(BlueprintPure, Category="Objective")
	bool IsGameOver() const { return bGameOver; }

	/** 勝敗いずれかが確定してラウンドが終了しているか */
	UFUNCTION(BlueprintPure, Category="Objective")
	bool IsRoundFinished() const { return bEscaped || bGameOver; }

	/** この夢での行動が終わったか（勝敗確定、または捕まって作り直し待ち） */
	UFUNCTION(BlueprintPure, Category="Objective")
	bool IsDreamOver() const { return IsRoundFinished() || bLifeLost; }

private:

	/** 勝敗を確定して通知する。確定済みなら何もしない（勝利後の捕獲・敗北後の脱出を無視） */
	void FinishRound(bool bWon);
};
