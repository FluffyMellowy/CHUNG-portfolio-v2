#pragma once

#include "CoreMinimal.h"
#include "Interactables/PickupBase.h"
#include "Battery.generated.h"

/**
 *  拾うと懐中電灯のバッテリーを回復する消耗品。
 */
UCLASS()
class ABattery : public APickupBase
{
	GENERATED_BODY()

protected:

	/** 回復量（MaxBatteryを上限にクランプされる） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Battery", meta = (ClampMin = 0))
	float RechargeAmount = 40.0f;

public:
	//~ Begin IInteractable
	virtual FText GetInteractionPrompt_Implementation() const override;
	//~ End IInteractable

protected:
	virtual void OnPickedUp(APlayerCharacter* Interactor) override;
};
