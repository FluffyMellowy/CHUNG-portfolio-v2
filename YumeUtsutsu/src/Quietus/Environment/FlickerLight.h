#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FlickerLight.generated.h"

class UPointLightComponent;

/**
 *  明滅する天井灯。基準光量の周りをランダムに揺らし、時々ふっと落ちる。
 *  メインメニューの演出用だが、ゲーム内の不安定な照明にもそのまま使える。
 */
UCLASS()
class AFlickerLight : public AActor
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPointLightComponent> Light;

protected:

	/** 基準光量 (cd) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flicker", meta = (ClampMin = 0))
	float BaseIntensity = 800.0f;

	/** 揺らぎの下限・上限（基準光量に対する倍率） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flicker", meta = (ClampMin = 0, ClampMax = 1))
	float FlickerMin = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flicker", meta = (ClampMin = 0, ClampMax = 2))
	float FlickerMax = 1.0f;

	/** 揺らぎの更新間隔（秒）。蛍光灯のチラつき感はこの粗さで出る */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flicker", meta = (ClampMin = 0.01))
	float FlickerInterval = 0.08f;

	/** 更新のたびに完全消灯する確率 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flicker", meta = (ClampMin = 0, ClampMax = 1))
	float DropoutChance = 0.08f;

	/** 消灯が続く時間（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flicker", meta = (ClampMin = 0))
	float DropoutDuration = 0.18f;

	float Timer = 0.0f;
	float DropoutTimer = 0.0f;

public:
	AFlickerLight();

	virtual void Tick(float DeltaSeconds) override;
};
