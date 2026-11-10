#pragma once

#include "CoreMinimal.h"
#include "Interactables/PickupBase.h"
#include "DreamShard.generated.h"

class UPointLightComponent;
class USphereComponent;

/**
 *  夢の欠片。E で拾うと GameState の手持ち数が増え、アクターは消える。
 *  暗い迷路でも見つけられるよう、ゆっくり回りながら上下に漂い、弱い光をまとう。
 *  メッシュには当たり判定が無いので、調べる判定は小さな球が受ける。
 */
UCLASS()
class ADreamShard : public APickupBase
{
	GENERATED_BODY()

	/** 調べる判定（Visibility だけをブロック） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> InteractionSphere;

	/** 欠片の周りを照らす弱い光 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPointLightComponent> Glow;

protected:

	/** スポーン位置（床・棚の面）からどれだけ浮かせるか (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DreamShard", meta = (Units = "cm"))
	float HoverHeight = 30.0f;

	/** 上下に漂う幅 (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DreamShard", meta = (ClampMin = 0, Units = "cm"))
	float BobAmplitude = 3.0f;

	/** 上下に漂う周期（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DreamShard", meta = (ClampMin = 0.1, Units = "s"))
	float BobPeriod = 2.5f;

	/** 回転速度（度/秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DreamShard")
	float SpinSpeed = 45.0f;

public:
	ADreamShard();

	virtual void Tick(float DeltaSeconds) override;

	//~ Begin IInteractable
	virtual FText GetInteractionPrompt_Implementation() const override;
	//~ End IInteractable

protected:
	virtual void BeginPlay() override;
	virtual void OnPickedUp(APlayerCharacter* Interactor) override;

private:
	FVector BaseLocation = FVector::ZeroVector;
	float Age = 0.0f;
};
