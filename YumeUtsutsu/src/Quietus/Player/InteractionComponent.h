#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractionComponent.generated.h"

class UCameraComponent;

/**
 *  視線インタラクションを担当するコンポーネント。
 *  毎フレーム、視点カメラから前方へスフィアトレースして IInteractable 実装アクターを検出し、
 *  現在のフォーカス対象とHUD用プロンプトを提供する。
 *  実行（Eキー相当）は TryInteract() を呼ぶだけでよい。
 */
UCLASS(ClassGroup=(Quietus), meta=(BlueprintSpawnableComponent))
class UInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

protected:

	/** インタラクト可能な距離（カメラからのトレース長, cm） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction", meta = (ClampMin = 0, Units = "cm"))
	float InteractionDistance = 250.0f;

	/** トレースの球半径（cm）。0より大きくすると多少狙いが甘くても当たる */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction", meta = (ClampMin = 0, Units = "cm"))
	float InteractionRadius = 15.0f;

	/** 現在視線の先にあるインタラクト対象（なければ null） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="Interaction")
	TObjectPtr<AActor> FocusedInteractable = nullptr;

#if WITH_EDITORONLY_DATA
	/** 自動テスト用フック: trueにすると次のTickでTryInteract()を1回実行して自動でfalseに戻る（エディタ専用） */
	UPROPERTY(EditAnywhere, Transient, Category="Debug")
	bool bDebugTriggerInteract = false;
#endif

private:

	/** トレース起点となる視点カメラ（BeginPlayでオーナーから取得してキャッシュ） */
	UPROPERTY(Transient)
	TObjectPtr<UCameraComponent> ViewCamera = nullptr;

public:
	UInteractionComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** フォーカス中の対象に作用する。対象がなければ何もしない */
	UFUNCTION(BlueprintCallable, Category="Interaction")
	void TryInteract();

	/** フォーカス中の対象のプロンプト文字列を返す（対象がなければ空）。HUD用 */
	UFUNCTION(BlueprintPure, Category="Interaction")
	FText GetInteractionPrompt() const;

	UFUNCTION(BlueprintPure, Category="Interaction")
	AActor* GetFocusedInteractable() const { return FocusedInteractable; }

protected:
	virtual void BeginPlay() override;

	/** カメラから前方へスフィアトレースし、FocusedInteractable を更新する */
	void UpdateFocus();
};
