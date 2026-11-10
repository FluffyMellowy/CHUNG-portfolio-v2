#include "UI/HUDWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Player/PlayerCharacter.h"
#include "Core/DreamLightingSettings.h"
#include "Core/RunGameInstance.h"
#include "Core/RunGameState.h"

#define LOCTEXT_NAMESPACE "HUD"

void UHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ARunGameState* GameState = ARunGameState::Get(this))
	{
		GameState->OnObjectiveChanged.AddDynamic(this, &UHUDWidget::RefreshObjective);
	}
	if (MessageText)
	{
		MessageText->SetVisibility(ESlateVisibility::Collapsed);
	}
	RefreshObjective();

	ApplyTextStyle();
#if WITH_EDITOR
	LightingChangedHandle = GetMutableDefault<UDreamLightingSettings>()->OnSettingChanged().AddUObject(this, &UHUDWidget::HandleLightingChanged);
#endif
}

void UHUDWidget::ApplyTextStyle()
{
	const UDreamLightingSettings& S = UDreamLightingSettings::Get();
	WidgetTree->ForEachWidget([&S](UWidget* Widget)
	{
		if (UTextBlock* Text = Cast<UTextBlock>(Widget))
		{
			FSlateFontInfo Font = Text->GetFont();
			Font.OutlineSettings.OutlineSize = S.HudOutlineSize;
			Font.OutlineSettings.OutlineColor = S.HudOutlineColor;
			Text->SetFont(Font);
			Text->SetShadowOffset(S.HudShadowOffset);
			Text->SetShadowColorAndOpacity(S.HudShadowColor);
		}
	});
}

void UHUDWidget::NativeDestruct()
{
#if WITH_EDITOR
	GetMutableDefault<UDreamLightingSettings>()->OnSettingChanged().Remove(LightingChangedHandle);
#endif
	if (ARunGameState* GameState = ARunGameState::Get(this))
	{
		GameState->OnObjectiveChanged.RemoveDynamic(this, &UHUDWidget::RefreshObjective);
	}
	Super::NativeDestruct();
}

void UHUDWidget::RefreshObjective()
{
	const ARunGameState* GameState = ARunGameState::Get(this);
	if (!GameState)
	{
		return;
	}
	if (ShardText)
	{
		ShardText->SetText(FText::Format(LOCTEXT("ShardFormat", "夢の欠片 {0}/{1}"),
			GameState->GetFoundShards(), GameState->GetRequiredShards()));
	}
	if (ObjectiveText)
	{
		FText Objective;
		if (GameState->IsInSafeZone())
		{
			Objective = LOCTEXT("ObjectiveClock", "目覚まし時計へ");
		}
		else if (GameState->IsExitOpen())
		{
			Objective = LOCTEXT("ObjectiveStairs", "扉の先へ");
		}
		else if (GameState->GetFoundShards() >= GameState->GetRequiredShards())
		{
			Objective = LOCTEXT("ObjectiveDoor", "扉に夢の欠片をはめる");
		}
		else
		{
			Objective = LOCTEXT("ObjectiveShards", "夢の欠片を探す");
		}
		ObjectiveText->SetText(Objective);
	}
	if (LivesText)
	{
		if (const URunGameInstance* GI = GetGameInstance<URunGameInstance>())
		{
			LivesText->SetText(FText::Format(LOCTEXT("LivesFormat", "{0}　命 {1}"),
				UDreamDifficultySettings::GetDisplayName(GI->GetDifficulty()), GI->GetLivesRemaining()));
		}
	}
}

void UHUDWidget::ShowLifeLost(int32 LivesRemaining)
{
	RefreshObjective();
	if (MessageText)
	{
		MessageText->SetText(FText::Format(LOCTEXT("LifeLost", "まだ目覚めない……\n残りの命 {0}"), LivesRemaining));
		MessageText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const APlayerCharacter* Player = Cast<APlayerCharacter>(GetOwningPlayerPawn());
	if (Player)
	{
		if (StaminaBar && Player->GetMaxStamina() > 0.0f)
		{
			const float Ratio = Player->GetStamina() / Player->GetMaxStamina();
			StaminaBar->SetPercent(Ratio);

			// 枯渇リカバリ中=赤（スプリント不可の警告）、低残量=橙、通常=緑
			FLinearColor Color(0.25f, 0.85f, 0.35f);
			if (Player->IsStaminaExhausted())
			{
				Color = FLinearColor(0.9f, 0.15f, 0.1f);
			}
			else if (Ratio < LowStaminaThreshold)
			{
				Color = FLinearColor(0.95f, 0.55f, 0.1f);
			}
			StaminaBar->SetFillColorAndOpacity(Color);
		}
		if (BatteryBar && Player->GetMaxBattery() > 0.0f)
		{
			BatteryBar->SetPercent(Player->GetBattery() / Player->GetMaxBattery());
		}
		if (ThrowableText)
		{
			ThrowableText->SetText(Player->GetThrowableCount() > 0
				? FText::Format(LOCTEXT("ThrowableFormat", "ビン ×{0}"), Player->GetThrowableCount())
				: FText::GetEmpty());
		}
		if (PromptText)
		{
			const FText Prompt = Player->GetInteractionPrompt();
			if (Prompt.IsEmpty())
			{
				PromptText->SetText(FText::GetEmpty());
			}
			else
			{
				PromptText->SetText(FText::Format(LOCTEXT("PromptFormat", "[E] {0}"), Prompt));
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
