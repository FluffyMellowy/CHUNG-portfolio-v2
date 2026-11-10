#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "KillCamComponent.generated.h"

class AEnemyCharacter;
class APlayerCharacter;
class UCameraComponent;
class UKillSequence;

/**
 *  捕まったときの演出中、プレイヤーの視点を敵の演出データ（UKillSequence）の軌跡に沿って動かす。プレイヤーコントローラに付ける。
 *  視点のカメラを体から外して直接動かすので、カメラに付いた懐中電灯も一緒に倒れ込む。
 *  刃が届く瞬間に暗転し、暗転し切ったら OnBlackout を呼ぶ。そのあとはレベルを開き直すまで真っ暗のまま。
 */
UCLASS()
class UKillCamComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UKillCamComponent();

	/** 演出を始める。FromDirection は相手から見て敵が立つ向き（ゼロなら敵の今いる向き）。敵に演出データがなければ何もせず false */
	bool Play(AEnemyCharacter* Killer, APlayerCharacter* Victim, const FVector& FromDirection = FVector::ZeroVector);

	bool IsPlaying() const { return Sequence != nullptr; }

	/** 暗転し切った */
	FSimpleDelegate OnBlackout;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void UpdateView();
	void FinishBlackout();

	UPROPERTY(Transient)
	TObjectPtr<UKillSequence> Sequence;

	TWeakObjectPtr<AEnemyCharacter> Killer;
	TWeakObjectPtr<UCameraComponent> Camera;

	/** 捕まった瞬間の視点（ここから軌跡へ寄せる） */
	FTransform StartView;
	float Elapsed = 0.0f;
	bool bImpact = false;
	FTimerHandle BlackoutTimer;
};
