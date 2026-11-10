#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interactables/Interactable.h"
#include "AlarmClock.generated.h"

class UStaticMeshComponent;

/**
 *  階段の上のテーブルに置かれた目覚まし時計。触れるとベルが鳴り、鳴り終わると夢から覚める（脱出成功）。
 *  部品ごとのメッシュ（ピボットは元リグの骨の位置）を組み合わせ、鳴る動きは部品の小さな回転で作る。
 *  調べる判定は本体の UCX だけが受け、他の部品は当たり判定なし。メッシュと部品の配置は BP で設定する。
 */
UCLASS()
class AAlarmClock : public AActor, public IInteractable
{
	GENERATED_BODY()

	/** 本体（原点 = 底面の中心、正面 -X） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> Body;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> Glass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> Hook;

	/** ベルを叩くハンマー（X 軸で振る） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> Hammer;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> BellL;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> BellR;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> HourHand;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> MinuteHand;

	/** 秒針（常に動かす） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> SecondHand;

protected:

	/** 鳴ってから目覚める（脱出が確定する）まで（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AlarmClock", meta = (ClampMin = 0, Units = "s"))
	float RingDuration = 2.5f;

	/** ハンマーの振れ幅（度）と振動数 (Hz) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AlarmClock")
	float HammerSwing = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AlarmClock")
	float HammerFrequency = 14.0f;

	/** ベルと本体の揺れ（度） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AlarmClock")
	float BellShake = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AlarmClock")
	float BodyShake = 1.5f;

public:
	AAlarmClock();

	virtual void Tick(float DeltaSeconds) override;

	//~ Begin IInteractable
	virtual void Interact_Implementation(APlayerCharacter* Interactor) override;
	virtual FText GetInteractionPrompt_Implementation() const override;
	//~ End IInteractable

protected:
	virtual void BeginPlay() override;

private:
	bool bRinging = false;
	bool bWokeUp = false;
	float RingElapsed = 0.0f;
	FRotator BodyRestRotation = FRotator::ZeroRotator;
};
