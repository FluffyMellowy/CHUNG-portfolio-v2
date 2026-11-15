#include "UI/ItemSlotWidget.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "UI/HUDWidget.h"

#define LOCTEXT_NAMESPACE "HUD"

void UItemSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// HUD の他の文字と同じ縁取りと影
	UHUDWidget::ApplyHudTextStyle(WidgetTree);
	SetItem(nullptr, 0, false);
}

void UItemSlotWidget::SetNumber(int32 Number)
{
	if (NumberText)
	{
		NumberText->SetText(FText::AsNumber(Number));
	}
}

void UItemSlotWidget::SetItem(UTexture2D* InIcon, int32 Count, bool bSelected)
{
	const bool bHas = Count > 0 && InIcon != nullptr;
	if (Frame)
	{
		Frame->SetBrushColor(bSelected && bHas ? SelectedFrameColor : FrameColor);
	}
	if (Icon)
	{
		if (bHas && ShownIcon.Get() != InIcon)
		{
			Icon->SetBrushFromTexture(InIcon);
			ShownIcon = InIcon;
		}
		Icon->SetColorAndOpacity(IconColor);
		Icon->SetVisibility(bHas ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
	if (NumberText)
	{
		NumberText->SetColorAndOpacity(FSlateColor(bHas ? NumberColor : EmptyNumberColor));
	}
	if (CountText)
	{
		CountText->SetText(bHas ? FText::Format(LOCTEXT("ItemCount", "×{0}"), Count) : FText::GetEmpty());
	}
}

#undef LOCTEXT_NAMESPACE
