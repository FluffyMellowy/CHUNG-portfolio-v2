#include "MainMenuWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Core/DreamDifficulty.h"
#include "Core/RunGameInstance.h"
#include "Kismet/KismetSystemLibrary.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "MainMenu"

void UMainMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	ChosenDifficulty = EDreamDifficulty::Akumu;
	StartButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnStartClicked);
	QuitButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnQuitClicked);
	if (YumeButton)   { YumeButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnYumeClicked); }
	if (AkumuButton)  { AkumuButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnAkumuClicked); }
	if (SarumeButton) { SarumeButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnSarumeClicked); }
	if (BackButton)   { BackButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnBackClicked); }
	if (CreditsButton)      { CreditsButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnCreditsClicked); }
	if (CreditsCloseButton) { CreditsCloseButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnCreditsCloseClicked); }
	if (CreditsPanel)       { CreditsPanel->SetVisibility(ESlateVisibility::Collapsed); }

	// 説明文は難易度設定の値から作る（設定を変えても表示がずれない）
	auto Describe = [](UTextBlock* Text, EDreamDifficulty Difficulty)
	{
		if (Text)
		{
			const FDreamDifficultyParams& P = UDreamDifficultySettings::GetParams(Difficulty);
			Text->SetText(FText::Format(LOCTEXT("DifficultyDetail", "欠片 {0}　・　命 {1}"), P.ShardCount, P.Lives));
		}
	};
	Describe(YumeDetail, EDreamDifficulty::Yume);
	Describe(AkumuDetail, EDreamDifficulty::Akumu);
	Describe(SarumeDetail, EDreamDifficulty::Sarume);

	if (DifficultyBox)
	{
		DifficultyBox->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UMainMenuWidget::OnCreditsClicked()
{
	if (CreditsPanel)
	{
		CreditsPanel->SetVisibility(ESlateVisibility::Visible);
	}
}

void UMainMenuWidget::OnCreditsCloseClicked()
{
	if (CreditsPanel)
	{
		CreditsPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UMainMenuWidget::OnStartClicked()
{
	if (!DifficultyBox)
	{
		ChooseDifficulty(EDreamDifficulty::Akumu);
		return;
	}
	if (MenuBox)
	{
		MenuBox->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (OnStartRequested.IsBound())
	{
		if (Title)
		{
			Title->SetVisibility(ESlateVisibility::Hidden);
		}
		OnStartRequested.Execute();
		return;
	}
	ShowDifficulty();
}

void UMainMenuWidget::ShowDifficulty()
{
	if (DifficultyBox)
	{
		DifficultyBox->SetVisibility(ESlateVisibility::Visible);
	}
}

void UMainMenuWidget::OnYumeClicked()   { ChooseDifficulty(EDreamDifficulty::Yume); }
void UMainMenuWidget::OnAkumuClicked()  { ChooseDifficulty(EDreamDifficulty::Akumu); }
void UMainMenuWidget::OnSarumeClicked() { ChooseDifficulty(EDreamDifficulty::Sarume); }

void UMainMenuWidget::OnBackClicked()
{
	if (OnBackRequested.IsBound())
	{
		OnBackRequested.Execute();
		return;
	}
	if (DifficultyBox)
	{
		DifficultyBox->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (MenuBox)
	{
		MenuBox->SetVisibility(ESlateVisibility::Visible);
	}
}

void UMainMenuWidget::ChooseDifficulty(EDreamDifficulty Difficulty)
{
	ChosenDifficulty = Difficulty;

	// 選択肢を生成中表示に差し替えてから、描画1フレーム置いて遷移する
	if (DifficultyBox)
	{
		DifficultyBox->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (MenuBox)
	{
		MenuBox->SetVisibility(ESlateVisibility::Hidden);
	}
	StartButton->SetVisibility(ESlateVisibility::Hidden);
	StatusText->SetVisibility(ESlateVisibility::Visible);

	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UMainMenuWidget::DoStartRun);
}

void UMainMenuWidget::OnQuitClicked()
{
	UKismetSystemLibrary::QuitGame(GetWorld(), GetOwningPlayer(), EQuitPreference::Quit, /*bIgnorePlatformRestrictions=*/ false);
}

void UMainMenuWidget::DoStartRun()
{
	if (URunGameInstance* GI = GetGameInstance<URunGameInstance>())
	{
		GI->StartRun(ChosenDifficulty);
	}
}

#undef LOCTEXT_NAMESPACE
