#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ItemSlotWidget.generated.h"

class UBorder;
class UImage;
class UTextBlock;
class UTexture2D;

/**
 *  アイテム欄の 1 マス（WBP_ItemSlot）。番号・アイコン・持っている数を出し、手に持っている欄は枠を明るくする。
 *  UMG 側は同名ウィジェット（BindWidget）を置くだけでよい。色の調整値は WBP のクラス既定値で変えられる。
 */
UCLASS(abstract)
class UItemSlotWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	/** 外枠（ブラシの色が枠の色になる） */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> Frame;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Icon;

	/** 欄の番号（1, 2…） */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> NumberText;

	/** 持っている数（×2）。持っていなければ空 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CountText;

	/** 枠の色（普段 / 手に持っているとき） */
	UPROPERTY(EditAnywhere, Category="Item Slot")
	FLinearColor FrameColor = FLinearColor(0.85f, 0.82f, 0.75f, 0.18f);

	UPROPERTY(EditAnywhere, Category="Item Slot")
	FLinearColor SelectedFrameColor = FLinearColor(0.95f, 0.92f, 0.85f, 0.85f);

	/** アイコンの色（持っているとき） */
	UPROPERTY(EditAnywhere, Category="Item Slot")
	FLinearColor IconColor = FLinearColor(0.85f, 0.82f, 0.75f, 0.95f);

	/** 番号の色（持っているとき / 空のとき） */
	UPROPERTY(EditAnywhere, Category="Item Slot")
	FLinearColor NumberColor = FLinearColor(0.85f, 0.82f, 0.75f, 0.9f);

	UPROPERTY(EditAnywhere, Category="Item Slot")
	FLinearColor EmptyNumberColor = FLinearColor(0.85f, 0.82f, 0.75f, 0.35f);

	virtual void NativeConstruct() override;

public:
	void SetNumber(int32 Number);

	/** 中身を出す。Count が 0 なら空の欄にする */
	void SetItem(UTexture2D* InIcon, int32 Count, bool bSelected);

private:
	TWeakObjectPtr<UTexture2D> ShownIcon;
};
