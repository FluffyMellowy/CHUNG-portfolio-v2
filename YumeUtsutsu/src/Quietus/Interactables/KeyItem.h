#pragma once

#include "CoreMinimal.h"
#include "Interactables/PickupBase.h"
#include "KeyItem.generated.h"

/**
 *  鍵。拾うとアイテム欄の 2 番に入る。
 *  2 番を持って閉じた扉（脱出扉以外）に R で使うと、迷路の別の扉の前へ移る。1 本で 1 回。
 */
UCLASS()
class AKeyItem : public APickupBase
{
	GENERATED_BODY()

public:
	//~ Begin IInteractable
	virtual FText GetInteractionPrompt_Implementation() const override;
	//~ End IInteractable

protected:
	virtual void OnPickedUp(APlayerCharacter* Interactor) override;
};
