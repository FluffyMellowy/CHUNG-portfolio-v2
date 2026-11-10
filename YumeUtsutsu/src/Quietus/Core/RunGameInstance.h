#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Core/DreamDifficulty.h"
#include "RunGameInstance.generated.h"

/**
 *  ラン（1プレイ）をまたいで状態を受け渡すGameInstance。
 *  - 難易度と残りの命。捕まると命が 1 減り、残っていれば新しいシードで夢（レベル）を作り直して続ける
 *  - 次に生成するシードと、開始前の「キー入力待ち」を出すか
 */
UCLASS()
class URunGameInstance : public UGameInstance
{
	GENERATED_BODY()

	/** 次のランで使うシード。0以下なら未設定（レベル側の設定値を使う） */
	UPROPERTY(Transient)
	int32 PendingSeed = 0;

	/** 現在のランで実際に使われたシード（HUD表示用）。0以下なら未生成 */
	UPROPERTY(Transient)
	int32 CurrentRunSeed = 0;

	/** メニューから開始したランで、開始前の「キー入力待ち」をまだ出していないか */
	UPROPERTY(Transient)
	bool bPendingRunIntro = false;

	/** 捕まって作り直した夢で、真っ暗から明ける演出をまだ出していないか */
	UPROPERTY(Transient)
	bool bPendingDreamFadeIn = false;

	/** 目覚まし時計で夢から覚めて部屋を開くところか（タイトルではなく朝の場面を出す） */
	UPROPERTY(Transient)
	bool bPendingWakeUp = false;

	UPROPERTY(Transient)
	EDreamDifficulty Difficulty = EDreamDifficulty::Akumu;

	/** 残りの命（今の夢の分を含む） */
	UPROPERTY(Transient)
	int32 LivesRemaining = 0;

	/** このランで何回目の夢か（1 始まり） */
	UPROPERTY(Transient)
	int32 DreamNumber = 0;

public:
	virtual void Init() override;

	/** 難易度を決めて新しいランを始める（命を満タンにして生成レベルへ） */
	UFUNCTION(BlueprintCallable, Category="Run")
	void StartRun(EDreamDifficulty InDifficulty);

	/** 今の難易度で新しいランを始め直す（終了画面のリスタート） */
	UFUNCTION(BlueprintCallable, Category="Run")
	void StartNewRun();

	/**
	 *  捕まったときに呼ぶ。命を 1 減らす。
	 *  @return まだ命が残っていれば true（夢を作り直して続ける）、尽きたら false（目覚めない＝バッドエンド）
	 */
	bool LoseLife();

	/** 命が残っているとき: 新しいシードで今のレベルを開き直す（生成レベルなら別の構造になる） */
	void ContinueDream();

	/** 目覚まし時計で夢から覚めた: 主人公の部屋の朝を開く */
	void WakeUp();

	/** 夢から覚めて開いたところか。呼ぶと消費される */
	bool ConsumeWakeUpFlag();

	/** 確認用: 表示中の画面のボタンを押したことにする（Name はウィジェット名。WBP_MainMenu の StartButton など。AnyKey でキー待ちを閉じる） */
	UFUNCTION(Exec)
	void SimButton(const FString& Name);

	/** 紹介動画の撮影: Folder に連番 PNG と音（audio.wav）を録り始める。Every フレームごとに 1 枚（段取りの確認用に間引ける） */
	UFUNCTION(Exec)
	void CaptureStart(const FString& Folder, int32 Every = 1);

	/** 紹介動画の撮影: 録画を止めて音を書き出す */
	UFUNCTION(Exec)
	void CaptureStop();

	/** 生成器が呼ぶ: 予約シードがあれば取り出す。@return 予約があったか */
	bool ConsumePendingSeed(int32& OutSeed);

	/** 生成器が呼ぶ: このランで実際に使ったシードを記録する */
	void SetCurrentRunSeed(int32 InSeed) { CurrentRunSeed = InSeed; }

	UFUNCTION(BlueprintPure, Category="Run")
	int32 GetCurrentRunSeed() const { return CurrentRunSeed; }

	/** 開始前オーバーレイを出すべきか。呼ぶと消費される（1回だけ出す） */
	bool ConsumeRunIntroFlag();

	/** 捕まって作り直した夢か（暗転から明ける）。呼ぶと消費される */
	bool ConsumeDreamFadeInFlag();

	UFUNCTION(BlueprintPure, Category="Run")
	EDreamDifficulty GetDifficulty() const { return Difficulty; }

	const FDreamDifficultyParams& GetDifficultyParams() const { return UDreamDifficultySettings::GetParams(Difficulty); }

	UFUNCTION(BlueprintPure, Category="Run")
	int32 GetLivesRemaining() const { return LivesRemaining; }

	UFUNCTION(BlueprintPure, Category="Run")
	int32 GetDreamNumber() const { return DreamNumber; }

	/** 難易度から引いた値を返す（UObject から GameInstance を探す手間を省く）。GameInstance が無ければ既定（悪夢） */
	static const FDreamDifficultyParams& GetParams(const UObject* WorldContextObject);

private:
	/** bShowIntro: 開いた先で「キーを押して開始」を出すか */
	void IssueSeedAndOpen(FName LevelName, bool bShowIntro = true);
};
