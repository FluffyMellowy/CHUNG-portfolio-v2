#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ItemSlotsComponent.generated.h"

class AActor;
class ADoor;
class APickupBase;
class UStaticMeshComponent;
class UTexture2D;

/**
 *  アイテム欄（画面左下）。番号キーで欄の物を手に持ち、R で使う。
 *  - 1: 投擲物（ビン）。投げると落ちた所で音が鳴り、ピエロを呼ぶ
 *  - 2: 鍵。閉じた扉（脱出扉以外）に使うと迷路の別の扉の前へ移る
 *  同じ番号をもう一度押すとしまう。持っている物の数が 0 になると自動でしまう。
 *  投擲物の数は UThrowInventoryComponent、鍵の数はここが持つ。
 *  手に持った見た目は欄の物のクラス（APickupBase の本体メッシュと HeldTransform）から作る。
 */
UCLASS(ClassGroup=(Quietus), meta=(BlueprintSpawnableComponent))
class UItemSlotsComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	static constexpr int32 NoSlot = 0;
	static constexpr int32 ThrowableSlot = 1;
	static constexpr int32 KeySlot = 2;
	static constexpr int32 NumSlots = 2;

	UItemSlotsComponent();

	/** 手に持った物を表示するメッシュ（持ち主がカメラに付けて渡す） */
	void SetHeldMesh(UStaticMeshComponent* InHeldMesh) { HeldMesh = InHeldMesh; }

	/** 番号キー。同じ番号ならしまい、空の欄は持たない */
	void SelectSlot(int32 Slot);

	/** R。手に持った物を使う。Focused は視線の先のインタラクト対象 */
	bool UseSelected(AActor* Focused);

	/** 鍵を拾う */
	void AddKey(TSubclassOf<APickupBase> KeyClass);

	/** 拾った・使った後に呼ぶ。数が 0 の物を持っていたらしまう */
	void RefreshHeld();

	int32 GetSelectedSlot() const { return SelectedSlot; }
	int32 GetCount(int32 Slot) const;
	UTexture2D* GetIcon(int32 Slot) const;

	/** 今持っている鍵をこの対象に使えるか（HUD の「[R] 鍵を使う」に使う） */
	bool CanUseKeyOn(const AActor* Focused) const;

private:
	TSubclassOf<APickupBase> GetItemClass(int32 Slot) const;
	const APickupBase* GetItemDefaults(int32 Slot) const;
	ADoor* AsKeyDoor(const AActor* Focused) const;
	void ShowHeld();

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> HeldMesh;

	UPROPERTY(VisibleAnywhere, Transient, Category="Item Slot")
	int32 SelectedSlot = NoSlot;

	UPROPERTY(VisibleAnywhere, Transient, Category="Item Slot")
	int32 KeyCount = 0;

	UPROPERTY(Transient)
	TSubclassOf<APickupBase> KeyItemClass;
};
