#include "UI/SeedOverlayWidget.h"
#include "Components/TextBlock.h"
#include "Core/RunGameInstance.h"

void USeedOverlayWidget::NativeConstruct()
{
	Super::NativeConstruct();

	int32 SeedValue = 0;
	if (const URunGameInstance* GI = GetGameInstance<URunGameInstance>())
	{
		SeedValue = GI->GetCurrentRunSeed();
	}
	SeedText->SetText(FText::FromString(FString::Printf(TEXT("Seed: %d"), SeedValue)));
}
