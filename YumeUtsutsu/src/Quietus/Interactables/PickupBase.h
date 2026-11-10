#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interactables/Interactable.h"
#include "PickupBase.generated.h"

class UStaticMeshComponent;

/**
 *  E で拾うと消えるアイテムの共通基底（Template Method）。
 *  「拾えるか → 拾った効果 → 自分を消す」の流れは Interact でここに固定し、
 *  派生クラスは CanBePickedUp / OnPickedUp / プロンプトだけを実装する。
 *  本体メッシュをルートに持ち、Tick はしない。
 */
UCLASS(Abstract)
class APickupBase : public AActor, public IInteractable
{
	GENERATED_BODY()

	/** 本体メッシュ（トレース判定もこれが受ける） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> Mesh;

public:
	APickupBase();

	UStaticMeshComponent* GetMesh() const { return Mesh; }

	//~ Begin IInteractable
	virtual void Interact_Implementation(APlayerCharacter* Interactor) override;
	virtual FText GetInteractionPrompt_Implementation() const override { return FText::GetEmpty(); }
	//~ End IInteractable

protected:
	/** 今拾えるか（投擲物の飛行中などは false） */
	virtual bool CanBePickedUp(const APlayerCharacter* Interactor) const { return true; }

	/** 拾われたときの効果。この直後にアクターは破棄される */
	virtual void OnPickedUp(APlayerCharacter* Interactor) PURE_VIRTUAL(APickupBase::OnPickedUp, );
};
