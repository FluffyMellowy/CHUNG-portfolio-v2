#include "UI/ModalOverlayWidget.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

void UModalOverlayWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetIsFocusable(true);
	UGameplayStatics::SetGamePaused(GetWorld(), true);

	if (APlayerController* PC = GetOwningPlayer())
	{
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(TakeWidget());
		PC->SetInputMode(InputMode);
	}
}

FReply UModalOverlayWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	Dismiss();
	return FReply::Handled();
}

FReply UModalOverlayWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Dismiss();
	return FReply::Handled();
}

void UModalOverlayWidget::Dismiss()
{
	UGameplayStatics::SetGamePaused(GetWorld(), false);

	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->bShowMouseCursor = false;
	}
	RemoveFromParent();
}
