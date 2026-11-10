#pragma once

#include "CoreMinimal.h"
#include "UI/ModalOverlayWidget.h"
#include "NoteWidget.generated.h"

class UTextBlock;

/**
 *  ロアメモの本文表示オーバーレイ（レイアウトは WBP_Note）。表示中はポーズし、キーかクリックで閉じる。
 */
UCLASS(Abstract)
class UNoteWidget : public UModalOverlayWidget
{
	GENERATED_BODY()

public:

	/** 表示するメモの内容を設定する */
	void SetNote(const FText& InTitle, const FText& InBody);

protected:

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> BodyText;
};
