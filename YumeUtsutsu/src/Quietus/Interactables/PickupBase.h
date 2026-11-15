#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interactables/Interactable.h"
#include "PickupBase.generated.h"

class UStaticMeshComponent;
class UTexture2D;

/**
 *  E で拾うと消えるアイテムの共通基底（Template Method）。
 *  「拾えるか → 拾った効果 → 自分を消す」の流れは Interact でここに固定し、
 *  派生クラスは CanBePickedUp / OnPickedUp / プロンプトだけを実装する。
 *  本体メッシュをルートに持ち、Tick はしない。
 *  アイテム欄（1/2…）に入る物は、欄のアイコンと手に持ったときの見え方もここで決める（BP で設定）。
 */
UCLASS(Abstract)
class APickupBase : public AActor, public IInteractable
{
	GENERATED_BODY()

	/** 本体メッシュ（トレース判定もこれが受ける） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> Mesh;

protected:
	/** アイテム欄に出すアイコン（白い線画。欄の色で塗られる） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item Slot")
	TObjectPtr<UTexture2D> SlotIcon;

	/** 手に持ったときの位置・向き・大きさ（カメラ基準。X=前、Y=右、Z=上）。本体メッシュをそのまま表示する。
	 *  大きさは本体メッシュの倍率に掛かる（床に置いたときと同じ大きさなら 1） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item Slot")
	FTransform HeldTransform = FTransform(FRotator(0.0f, 160.0f, 0.0f), FVector(32.0f, 14.0f, -14.0f));

public:
	APickupBase();

	UStaticMeshComponent* GetMesh() const { return Mesh; }
	UTexture2D* GetSlotIcon() const { return SlotIcon; }
	const FTransform& GetHeldTransform() const { return HeldTransform; }

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
