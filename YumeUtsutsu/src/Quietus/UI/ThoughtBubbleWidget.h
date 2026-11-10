#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ThoughtBubbleWidget.generated.h"

class UBorder;
class UCanvasPanelSlot;
class UWidget;

/**
 *  タイトルの部屋に重ねる吹き出し。「はじめから」で中へズームして夢に入る。
 *  見た目は WBP_ThoughtBubble が持つ（デザイン未定の仮枠）。
 *  Bubble は全画面キャンバス上にアンカーで置く。ズームは Bubble の Render Transform だけを動かすので、中身を差し替えても動く。
 */
UCLASS(Abstract)
class UThoughtBubbleWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Alpha 0 = 定位置、1 = 吹き出しの中が画面全体を覆う */
	void SetZoom(float Alpha);

	/** そのときの吹き出しの拡大率（カメラのズームを揃える用） */
	float GetZoomScale(float Alpha) const;

	/** 吹き出しの中心（画面に対する割合。0.5, 0.5 が画面中央） */
	FVector2D GetCenter() const;

protected:
	/** 吹き出し全体。ここにズームを掛ける */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> Bubble;

	/** 中身の下地。ズームが進むほど不透明にする */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> Fill = nullptr;

	/** ズームし切ったときの余裕（1 でちょうど画面を覆う） */
	UPROPERTY(EditAnywhere, Category="Zoom", meta = (ClampMin = 1))
	float CoverMargin = 1.2f;

	virtual void NativeOnInitialized() override;

private:
	const UCanvasPanelSlot* GetBubbleSlot() const;

	FLinearColor FillRestColor = FLinearColor::Black;
};
