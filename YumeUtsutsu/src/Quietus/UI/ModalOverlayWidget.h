#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ModalOverlayWidget.generated.h"

/**
 *  全画面の幕の上に内容を出し、表示中はゲームを一時停止する「何か押すまで閉じない」オーバーレイの基底。
 *  幕と内容のレイアウトは各 WBP（WBP_PressAnyKey・WBP_Note）が持ち、
 *  ここは一時停止・入力モード・キー/クリックで閉じる流れだけを持つ。
 */
UCLASS(Abstract)
class UModalOverlayWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 一時停止を解いてオーバーレイを閉じ、入力をゲームへ戻す */
	void Dismiss();

protected:
	virtual void NativeConstruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
};
