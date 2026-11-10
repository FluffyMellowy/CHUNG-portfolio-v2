#include "UI/ThoughtBubbleWidget.h"
#include "Components/Border.h"
#include "Components/CanvasPanelSlot.h"

void UThoughtBubbleWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (Fill)
	{
		FillRestColor = Fill->GetBrushColor();
	}
}

const UCanvasPanelSlot* UThoughtBubbleWidget::GetBubbleSlot() const
{
	return Bubble ? Cast<UCanvasPanelSlot>(Bubble->Slot) : nullptr;
}

FVector2D UThoughtBubbleWidget::GetCenter() const
{
	const UCanvasPanelSlot* BubbleSlot = GetBubbleSlot();
	if (!BubbleSlot)
	{
		return FVector2D(0.5, 0.5);
	}
	const FAnchors Anchors = BubbleSlot->GetAnchors();
	return (Anchors.Minimum + Anchors.Maximum) * 0.5;
}

float UThoughtBubbleWidget::GetZoomScale(float Alpha) const
{
	const UCanvasPanelSlot* BubbleSlot = GetBubbleSlot();
	if (!BubbleSlot)
	{
		return 1.0f;
	}
	const FAnchors Anchors = BubbleSlot->GetAnchors();
	const FVector2D Share = Anchors.Maximum - Anchors.Minimum;
	const float Cover = CoverMargin / FMath::Max(FMath::Min(Share.X, Share.Y), 0.01);
	// 倍率は等比で増やす（一定の速さで近づいて見える）
	return FMath::Pow(Cover, Alpha);
}

void UThoughtBubbleWidget::SetZoom(float Alpha)
{
	if (!Bubble)
	{
		return;
	}
	const FVector2D Size = GetCachedGeometry().GetLocalSize();
	FWidgetTransform Transform;
	Transform.Scale = FVector2D(GetZoomScale(Alpha));
	Transform.Translation = (FVector2D(0.5, 0.5) - GetCenter()) * Size * Alpha;
	Bubble->SetRenderTransform(Transform);

	if (Fill)
	{
		FLinearColor Color = FillRestColor;
		Color.A = FMath::Lerp(FillRestColor.A, 1.0f, Alpha);
		Fill->SetBrushColor(Color);
	}
}
