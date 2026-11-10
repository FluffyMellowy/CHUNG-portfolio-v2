#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SeedOverlayWidget.generated.h"

class UTextBlock;

/**
 *  ラン中に画面右上へ生成シードを表示するオーバーレイ（レイアウトは WBP_SeedOverlay）。
 */
UCLASS(Abstract)
class USeedOverlayWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SeedText;

	virtual void NativeConstruct() override;
};
