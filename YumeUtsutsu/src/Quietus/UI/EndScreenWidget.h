#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EndScreenWidget.generated.h"

class UButton;
class UTextBlock;

/**
 *  ラウンド終了画面（勝利／敗北 共用）。
 *  タイトル文言を差し替え、リスタート／終了ボタンを提供する。
 */
UCLASS(abstract)
class UEndScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	/** 勝敗に応じてタイトル文言を設定する */
	void SetResult(bool bWon);

protected:

	/** 結果表示（目が覚めた / 夢から醒めなかった） */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TitleText;

	/** リスタートボタン */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> RestartButton;

	/** ゲーム終了ボタン */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> QuitButton;

	/** メインメニューへ戻るボタン（WBP側に未配置でも動くようOptional） */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> MenuButton = nullptr;

	virtual void NativeOnInitialized() override;

	UFUNCTION()
	void OnRestartClicked();

	UFUNCTION()
	void OnQuitClicked();

	UFUNCTION()
	void OnMenuClicked();
};
