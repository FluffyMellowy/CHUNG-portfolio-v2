#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HUDWidget.generated.h"

class UProgressBar;
class UTextBlock;
class UItemSlotWidget;
class UWidgetTree;

/**
 *  インゲームHUD。スタミナ／バッテリー／夢の欠片の数と次の目標／難易度と残りの命／インタラクトプロンプト／アイテム欄を表示する。
 *  UMG側は同名ウィジェット（BindWidget）を置くだけでよく、更新ロジックはここが持つ。
 */
UCLASS(abstract)
class UHUDWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	/** スタミナバー */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> StaminaBar;

	/** バッテリーバー */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> BatteryBar;

	/** 夢の欠片の数（見つけた数 / 必要数） */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ShardText;

	/** 次にやること（例: 扉に夢の欠片をはめる）。WBP 側に未配置でも動くよう Optional */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ObjectiveText = nullptr;

	/** 難易度と残りの命（例: 悪夢　命 2） */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LivesText = nullptr;

	/** 画面中央のメッセージ（捕まったときの「まだ目覚めない」など）。普段は非表示 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MessageText = nullptr;

	/** インタラクトプロンプト（例: [E] 開ける） */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PromptText;

	/** アイテム欄（左下）。1 = 投擲物、2 = 鍵 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UItemSlotWidget> ItemSlot1 = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UItemSlotWidget> ItemSlot2 = nullptr;

	/** スタミナバーが橙色に変わる残量比 */
	UPROPERTY(EditDefaultsOnly, Category="HUD|Stamina", meta = (ClampMin = 0, ClampMax = 1))
	float LowStaminaThreshold = 0.3f;

	/** 文字の縁取りと影（Dream Lighting）をすべての TextBlock に掛ける。明るい床の上でも読めるように */
	void ApplyTextStyle();

	/** アイテム欄の表示を手持ちの数・選んでいる欄に合わせる */
	void RefreshItemSlots(const class APlayerCharacter* Player);

#if WITH_EDITOR
	void HandleLightingChanged(UObject* Settings, struct FPropertyChangedEvent& Event) { ApplyTextStyle(); }
	FDelegateHandle LightingChangedHandle;
#endif

	/** 目標表示は GameState の変更通知で更新する */
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** 毎フレーム、連続的に変わるプレイヤーの値（スタミナ・バッテリー・プロンプト）を反映する */
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** 欠片・次の目標・命の表示を GameState の現在値で更新する（OnObjectiveChanged から呼ばれる） */
	UFUNCTION()
	void RefreshObjective();

public:
	/** 捕まったが命が残っているときの中央メッセージ */
	void ShowLifeLost(int32 LivesRemaining);

	/** HUD の文字の縁取りと影を、ツリー内のすべての TextBlock に掛ける（アイテム欄の中の文字にも使う） */
	static void ApplyHudTextStyle(UWidgetTree* Tree);
};
