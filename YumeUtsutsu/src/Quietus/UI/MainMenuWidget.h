#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainMenuWidget.generated.h"

class UButton;
class UTextBlock;
class UWidget;
enum class EDreamDifficulty : uint8;

/**
 *  メインメニュー。レイアウトと文言は WBP_MainMenu が持ち、ここはボタンの処理だけを持つ。
 *  背景は3Dシーン（夜の主人公の部屋）を透過表示する。
 *  つづきから・オプションは仕様未定のため WBP 側で無効表示にしてある。
 *  はじめから → 吹き出しの先の夢の中で難易度（夢 / 悪夢 / 猿夢）を選ぶ → 生成レベルへ。
 */
UCLASS(Abstract)
class UMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	/** はじめからが押された。バインドされていれば難易度を出すのは受け取った側（ShowDifficulty を呼ぶ） */
	FSimpleDelegate OnStartRequested;

	/** 難易度の選択で戻るが押された。バインドされていなければ最初のメニューへ戻す */
	FSimpleDelegate OnBackRequested;

	/** 難易度の選択を出す */
	void ShowDifficulty();

protected:

	/** はじめから */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> StartButton;

	/** ゲームを終了 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> QuitButton;

	/** 「マップ生成中...」の表示（はじめからを押すまでは非表示） */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> StatusText;

	/** 最初のメニュー（はじめから・終了など）。難易度を選ぶ間は隠す */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> MenuBox = nullptr;

	/** 難易度の選択（はじめからを押すまでは非表示）。無ければ 悪夢 で即開始 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> DifficultyBox = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> YumeButton = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> AkumuButton = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> SarumeButton = nullptr;

	/** 各難易度の説明（欠片の数・命の数）。値は難易度設定から入れる */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> YumeDetail = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> AkumuDetail = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SarumeDetail = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BackButton = nullptr;

	/** ゲームタイトル。吹き出しへ入るときに隠す */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Title = nullptr;

	/** クレジットを開く */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CreditsButton = nullptr;

	/** クレジット画面（開くまでは非表示） */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> CreditsPanel = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CreditsCloseButton = nullptr;

	virtual void NativeOnInitialized() override;

	UFUNCTION()
	void OnCreditsClicked();

	UFUNCTION()
	void OnCreditsCloseClicked();

	UFUNCTION()
	void OnStartClicked();

	UFUNCTION()
	void OnYumeClicked();

	UFUNCTION()
	void OnAkumuClicked();

	UFUNCTION()
	void OnSarumeClicked();

	UFUNCTION()
	void OnBackClicked();

	void ChooseDifficulty(EDreamDifficulty Difficulty);

	UFUNCTION()
	void OnQuitClicked();

	/** 1フレーム置いてからレベル遷移する（「生成中...」を描画させるため） */
	void DoStartRun();

	EDreamDifficulty ChosenDifficulty;
};
