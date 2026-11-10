#include "UI/EndScreenWidget.h"
#include "Components/Button.h"
#include "Core/GameConstants.h"
#include "Core/RunGameInstance.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

#define LOCTEXT_NAMESPACE "EndScreen"

void UEndScreenWidget::SetResult(bool bWon)
{
	if (TitleText)
	{
		// 脱出＝目覚め。命が尽きた＝目覚めない（バッドエンド）
		TitleText->SetText(bWon
			? LOCTEXT("Escaped", "目が覚めた")
			: LOCTEXT("Died", "夢から醒めなかった"));
	}
}

void UEndScreenWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (RestartButton)
	{
		RestartButton->OnClicked.AddDynamic(this, &UEndScreenWidget::OnRestartClicked);
	}
	if (QuitButton)
	{
		QuitButton->OnClicked.AddDynamic(this, &UEndScreenWidget::OnQuitClicked);
	}
	if (MenuButton)
	{
		MenuButton->OnClicked.AddDynamic(this, &UEndScreenWidget::OnMenuClicked);
	}
}

void UEndScreenWidget::OnRestartClicked()
{
	// 生成レベルと目覚めの部屋では新しいランダムシードで作り直す。それ以外は同じレベルを開き直すだけ
	const FName Level(*UGameplayStatics::GetCurrentLevelName(this));
	if (Level == GameNames::GeneratedLevel || Level == GameNames::MainMenuLevel)
	{
		if (URunGameInstance* GI = GetGameInstance<URunGameInstance>())
		{
			GI->StartNewRun();
			return;
		}
	}
	UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this)));
}

void UEndScreenWidget::OnMenuClicked()
{
	UGameplayStatics::OpenLevel(this, GameNames::MainMenuLevel);
}

void UEndScreenWidget::OnQuitClicked()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

#undef LOCTEXT_NAMESPACE
