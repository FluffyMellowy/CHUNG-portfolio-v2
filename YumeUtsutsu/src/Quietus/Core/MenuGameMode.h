#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MenuGameMode.generated.h"

class ABedroomSet;
class ACameraActor;
class UEndScreenWidget;
class UMainMenuWidget;
class UThoughtBubbleWidget;

/** 部屋で台詞を挟む場所 */
UENUM(BlueprintType)
enum class EBedroomDialogue : uint8
{
	/** 「はじめから」のあと、吹き出しへズームする前 */
	BeforeDream,
	/** 目覚めのあと、クリア画面の前 */
	AfterWaking,
};

/**
 *  主人公の部屋で進むタイトルと目覚めの GameMode。ポーンは出さない。
 *  タイトル: 夜の部屋 + メニュー → 台詞 → 吹き出しへズーム → 夢の背景 + 難易度の選択 → 生成レベル。
 *  目覚め（時計で夢から覚めて開いたとき）: 朝の部屋 → 台詞 → クリア画面 → タイトル。
 *  レベルに ABedroomSet が無ければ、メニューだけを出して直接ランを始める。
 *  ウィジェットは BP_MenuGameMode で指定する。
 */
UCLASS(Abstract)
class AMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMenuGameMode();

	virtual void Tick(float DeltaSeconds) override;

	/** 台詞が終わった。次の場面へ進む（PlayDialogue を上書きしたら、読み終わりで呼ぶ） */
	UFUNCTION(BlueprintCallable, Category="Bedroom")
	void FinishDialogue();

protected:

	/** 表示するメニュー（WBP_MainMenu） */
	UPROPERTY(EditDefaultsOnly, Category="UI")
	TSubclassOf<UMainMenuWidget> MainMenuWidgetClass;

	/** タイトルの吹き出し（WBP_ThoughtBubble） */
	UPROPERTY(EditDefaultsOnly, Category="UI")
	TSubclassOf<UThoughtBubbleWidget> ThoughtBubbleWidgetClass;

	/** 目覚めのあとのクリア画面（WBP_EndScreen） */
	UPROPERTY(EditDefaultsOnly, Category="UI")
	TSubclassOf<UEndScreenWidget> EndScreenWidgetClass;

	/** 部屋を開いたあと、真っ暗のまま待つ時間（秒）。読み込み直後のちらつきを見せない */
	UPROPERTY(EditDefaultsOnly, Category="Bedroom", meta = (ClampMin = 0, Units = "s"))
	float FadeInDelay = 0.5f;

	/** 部屋が明けるまで（秒） */
	UPROPERTY(EditDefaultsOnly, Category="Bedroom", meta = (ClampMin = 0, Units = "s"))
	float FadeInTime = 1.5f;

	/** 「はじめから」の台詞の待ち時間（秒）。台詞が入るまではこれだけ待ってズームする */
	UPROPERTY(EditDefaultsOnly, Category="Bedroom", meta = (ClampMin = 0, Units = "s"))
	float BeforeDreamDialogueWait = 1.0f;

	/** 目覚めの台詞の待ち時間（秒）。部屋が明け切ってから数える */
	UPROPERTY(EditDefaultsOnly, Category="Bedroom", meta = (ClampMin = 0, Units = "s"))
	float AfterWakingDialogueWait = 2.0f;

	/** 吹き出しへズームする時間（秒） */
	UPROPERTY(EditDefaultsOnly, Category="Bedroom", meta = (ClampMin = 0, Units = "s"))
	float ZoomTime = 2.5f;

	virtual void BeginPlay() override;

	/**
	 *  台詞の差し込み口。既定は待ち時間だけ待って FinishDialogue を呼ぶ。
	 *  台詞ウィンドウができたらここを上書きし、読み終わりで FinishDialogue を呼ぶ。
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Bedroom")
	void PlayDialogue(EBedroomDialogue Point);

private:
	void StartTitle(APlayerController* PC);
	void StartMorning(APlayerController* PC);

	/** カメラを切り替え、真っ暗から明ける。明け切ったら OnShown を呼ぶ */
	void ShowScene(APlayerController* PC, ACameraActor* Camera, TFunction<void()> OnShown);

	/** メニューを操作できる状態にする */
	void FocusMenu(APlayerController* PC);

	void HandleStartRequested();
	void HandleBackRequested();
	void BeginDialogue(EBedroomDialogue Point);
	void StartZoom();
	void ShowDreamBackdrop();
	void ShowEndScreen();

	UPROPERTY(Transient)
	TObjectPtr<ABedroomSet> Bedroom;

	UPROPERTY(Transient)
	TObjectPtr<UMainMenuWidget> Menu;

	UPROPERTY(Transient)
	TObjectPtr<UThoughtBubbleWidget> ThoughtBubble;

	UPROPERTY(Transient)
	TObjectPtr<ACameraActor> ZoomCamera;

	EBedroomDialogue ActiveDialogue = EBedroomDialogue::BeforeDream;
	bool bDialogueActive = false;

	float ZoomElapsed = 0.0f;
	float ZoomStartFOV = 90.0f;
	FQuat ZoomStartRotation = FQuat::Identity;
	FQuat ZoomEndRotation = FQuat::Identity;

	FTimerHandle FadeTimer;
	FTimerHandle ShownTimer;
	FTimerHandle DialogueTimer;
};
