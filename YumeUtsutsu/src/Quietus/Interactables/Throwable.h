#pragma once

#include "CoreMinimal.h"
#include "Interactables/PickupBase.h"
#include "Throwable.generated.h"

class UProjectileMovementComponent;

/**
 *  投擲物（瓶・缶など）。
 *  置いてある間は E で拾ってカウントに加算し、投げると着地点でノイズを発生させて敵を誘導する。
 *  着地後はまた拾える（使い回し可能な陽動リソース）。
 */
UCLASS()
class AThrowable : public APickupBase
{
	GENERATED_BODY()

	/** 投擲時のみ有効化する弾道移動 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UProjectileMovementComponent> Projectile;

protected:

	/** 着地ノイズの大きさ（敵AI聴覚半径への倍率。スプリント=1.0が目安） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Throwable", meta = (ClampMin = 0))
	float ImpactLoudness = 1.2f;

	/** 投擲中か（trueの間は拾えない） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="Throwable")
	bool bInFlight = false;

	/** ノイズを発生させたか（最初の衝突1回だけ鳴らす） */
	bool bNoiseDone = false;

	/** 投げた本人（ノイズのインスティゲーター） */
	UPROPERTY(Transient)
	TObjectPtr<APawn> ThrownBy = nullptr;

public:
	AThrowable();

	/** 投擲を開始する。Velocityはワールド空間の初速 */
	void Launch(APawn* Thrower, const FVector& Velocity);

	/** 投げた本人（まだ投げていなければ nullptr） */
	APawn* GetThrower() const { return ThrownBy; }

	//~ Begin IInteractable
	virtual FText GetInteractionPrompt_Implementation() const override;
	//~ End IInteractable

protected:
	/** 投擲中（転がり中）は拾えない */
	virtual bool CanBePickedUp(const APlayerCharacter* Interactor) const override;

	/** クラスごと所持数に加える（投げると同じものが飛ぶ） */
	virtual void OnPickedUp(APlayerCharacter* Interactor) override;

	UFUNCTION()
	void OnMeshHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		FVector NormalImpulse, const FHitResult& Hit);

	virtual void BeginPlay() override;
};
