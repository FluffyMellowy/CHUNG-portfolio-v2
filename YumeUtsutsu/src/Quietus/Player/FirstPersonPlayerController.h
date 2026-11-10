#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "FirstPersonPlayerController.generated.h"

class UInputMappingContext;
class UUserWidget;
class UHUDWidget;
class UEndScreenWidget;
class UModalOverlayWidget;
class UNoteWidget;
class USeedOverlayWidget;
class UKillCamComponent;
class AEnemyCharacter;

/**
 *  Simple first person Player Controller
 *  Manages the input mapping context.
 *  Overrides the Player Camera Manager class.
 */
UCLASS(abstract, config="Game")
class QUIETUS_API AFirstPersonPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:

	/** Constructor */
	AFirstPersonPlayerController();

protected:

	/** HUDウィジェットクラス（エディタで割り当て） */
	UPROPERTY(EditAnywhere, Category="UI")
	TSubclassOf<UHUDWidget> HUDWidgetClass;

	/** 終了画面ウィジェットクラス（エディタで割り当て） */
	UPROPERTY(EditAnywhere, Category="UI")
	TSubclassOf<UEndScreenWidget> EndScreenWidgetClass;

	/** ロアメモの本文表示（WBP_Note） */
	UPROPERTY(EditAnywhere, Category="UI")
	TSubclassOf<UNoteWidget> NoteWidgetClass;

	/** メニューから始めたランの開始前「何かキーを押して開始」（WBP_PressAnyKey） */
	UPROPERTY(EditAnywhere, Category="UI")
	TSubclassOf<UModalOverlayWidget> RunIntroWidgetClass;

	/** 生成シードの表示（WBP_SeedOverlay） */
	UPROPERTY(EditAnywhere, Category="UI")
	TSubclassOf<USeedOverlayWidget> SeedOverlayWidgetClass;

	/** 捕まってから暗転し切るまで（秒） */
	UPROPERTY(EditAnywhere, Category="Run", meta = (ClampMin = 0, Units = "s"))
	float CaughtFadeTime = 1.5f;

	/** 捕まってから夢を作り直すまで（秒）。暗転のあと残りの命を読ませる間を取る */
	UPROPERTY(EditAnywhere, Category="Run", meta = (ClampMin = 0, Units = "s"))
	float DreamResetDelay = 3.5f;

	FTimerHandle DreamResetTimer;

	/** 捕獲演出（敵の演出データに沿って視点を動かす） */
	UPROPERTY(VisibleAnywhere, Category="Run")
	TObjectPtr<UKillCamComponent> KillCam;

	/** 捕獲演出を流す敵を探す範囲（cm）。これより遠い敵しかいなければ暗転だけ */
	UPROPERTY(EditAnywhere, Category="Run", meta = (ClampMin = 0, Units = "cm"))
	float KillCamRange = 400.0f;

	/** 捕獲演出の暗転のあと、残りの命を読ませてから夢を作り直すまで（秒） */
	UPROPERTY(EditAnywhere, Category="Run", meta = (ClampMin = 0, Units = "s"))
	float LifeLostHoldTime = 2.5f;

	/** 作り直した夢で、真っ暗のまま待つ時間（秒）。生成直後のちらつきを見せない */
	UPROPERTY(EditAnywhere, Category="Run", meta = (ClampMin = 0, Units = "s"))
	float DreamFadeInDelay = 0.3f;

	/** 作り直した夢が明けるまで（秒） */
	UPROPERTY(EditAnywhere, Category="Run", meta = (ClampMin = 0, Units = "s"))
	float DreamFadeInTime = 1.5f;

	FTimerHandle DreamFadeInTimer;

	/** 目覚まし時計が鳴り終わってから暗転し切るまで（秒）。暗転し切ったら部屋の朝を開く */
	UPROPERTY(EditAnywhere, Category="Run", meta = (ClampMin = 0, Units = "s"))
	float WakeFadeTime = 1.0f;

	FTimerHandle WakeTimer;

	/** 生成済みHUD */
	UPROPERTY(Transient)
	TObjectPtr<UHUDWidget> HUDWidget;

	/** 生成済み終了画面（生成済みなら切り替えは完了している） */
	UPROPERTY(Transient)
	TObjectPtr<UEndScreenWidget> EndScreenWidget;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<TObjectPtr<UInputMappingContext>> DefaultMappingContexts;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<TObjectPtr<UInputMappingContext>> MobileExcludedMappingContexts;

	/** Mobile controls widget to spawn */
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** Pointer to the mobile controls widget */
	UPROPERTY()
	TObjectPtr<UUserWidget> MobileControlsWidget;

	/** If true, the player will use UMG touch controls even if not playing on mobile platforms */
	UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

	/** Returns true if the player should use UMG touch controls */
	bool ShouldUseTouchControls() const;

	/** 勝敗確定の通知を受けて、HUDを外し終了画面へ切り替える（GameState::OnRoundFinished を購読） */
	UFUNCTION()
	void HandleRoundFinished(bool bEscaped);

	/** 捕まったが命が残っている（GameState::OnLifeLost を購読）。暗転して残りの命を出し、夢を作り直す */
	UFUNCTION()
	void HandleLifeLost(int32 LivesRemaining);

	void ResetDream();

	/** HUDを外して終了画面を出し、ゲームを止める */
	void ShowEndScreen(bool bEscaped);

	/** 近くの敵に捕獲演出を流させる。流せたら true（暗転し切ると OnBlackout を呼ぶ） */
	bool StartKillCam(TFunction<void()> OnBlackout);

	/** 目覚まし時計で夢から覚めた。暗転して部屋の朝へ */
	void WakeUp();

	/** 捕まって作り直した夢なら、真っ暗から明ける */
	void FadeInContinuedDream();

	/** 生成レベルならシードを、メニューから始めたランなら開始前オーバーレイを出す */
	void ShowRunOverlays();

public:

	// ---- 確認用コンソールコマンド（ハーネスや手元の検証用。出荷ビルドでは何もしない） ----

	/** いちばん近い虹色の扉の前へ移る */
	UFUNCTION(Exec)
	void GotoRainbowDoor();

	/** いちばん近い虹色の扉を使う */
	UFUNCTION(Exec)
	void UseRainbowDoor();

	/** いちばん近い風船の前へ移る */
	UFUNCTION(Exec)
	void GotoBalloon();

	/** いちばん近い風船を割る（自分が割ったことにする） */
	UFUNCTION(Exec)
	void PopBalloon();

	/** いちばん近い敵の前（正面から Yaw 度回った位置、Distance cm）に立って敵を向く */
	UFUNCTION(Exec)
	void GotoClown(float Distance = 350.0f, float Yaw = 0.0f);

	/** いちばん近い敵を、敵に付いて回るカメラで見る（懐中電灯と同じ灯り付き）。高さは足元から、LookHeight は注視する高さ。
	 *  Distance が 0 以下なら自分の視点に戻す */
	UFUNCTION(Exec)
	void WatchClown(float Distance = 300.0f, float Yaw = 90.0f, float Height = 120.0f, float LookHeight = 100.0f);

	/** 敵をその場に止め、指定の速度のロコモーションを再生させる。負の値で元に戻す */
	UFUNCTION(Exec)
	void PoseClowns(float Speed = 0.0f);

	// ---- 効果音の確認用（ハーネスから順に呼ぶ） ----

	/** 前へ歩かせる。Mode 0 = 歩き / 1 = 走り / 2 = しゃがみ */
	UFUNCTION(Exec)
	void SimMove(int32 Mode = 0, float Seconds = 2.0f);

	UFUNCTION(Exec)
	void SimFlashlight();

	/** 足元に出して拾う。Kind = Battery / Note / Throwable */
	UFUNCTION(Exec)
	void SimPickup(const FString& Kind);

	/** いちばん近いものを操作する。Kind = Door / Rainbow / Locker / Cabinet / Clock / Shard / Balloon / Exit */
	UFUNCTION(Exec)
	void SimInteract(const FString& Kind);

	// ---- 紹介動画の撮影用 ----

	/** いちばん近いものへナビの経路で歩き、StopDistance cm 手前で止まってそちらを向く。Mode は SimMove と同じ */
	UFUNCTION(Exec)
	void SimWalkTo(const FString& Kind, int32 Mode = 0, float StopDistance = 180.0f);

	/** 今の向きから Yaw 度回り、上下は Pitch 度へゆっくり向く（Speed は補間の速さ） */
	UFUNCTION(Exec)
	void SimLook(float Yaw, float Pitch = 0.0f, float Speed = 2.0f);

	/** いちばん近いものを Distance cm 離れて見る位置へ移る（LookHeight は注視点の高さのオフセット） */
	UFUNCTION(Exec)
	void GotoObject(const FString& Kind, float Distance = 200.0f, float LookHeight = 0.0f);

	/** 見通しの長い方向の先（最大 Distance cm）へ敵を移して追跡させ、そちらを向く */
	UFUNCTION(Exec)
	void SummonClown(float Distance = 1200.0f);

	/** いちばん近い空のロッカーを敵がこじ開けたことにする */
	UFUNCTION(Exec)
	void SimBustLocker();

	/** 瓶を前へ投げる */
	UFUNCTION(Exec)
	void SimThrow();

	/** いちばん近い敵の正面 Distance cm に立ち、その場で捕まったことにする */
	UFUNCTION(Exec)
	void SimCaught(float Distance = 110.0f);

	/** メモの本文を表示する（閉じるまでポーズ）。ANoteItem が拾われたときに呼ぶ */
	void ShowNote(const FText& Title, const FText& Body);
};
