#include "Player/ItemSlotsComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Interactables/Door.h"
#include "Interactables/PickupBase.h"
#include "Interactables/Throwable.h"
#include "Player/DoorWarpComponent.h"
#include "Player/PlayerCharacter.h"
#include "Player/ThrowInventoryComponent.h"
#include "Quietus.h"

UItemSlotsComponent::UItemSlotsComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

TSubclassOf<APickupBase> UItemSlotsComponent::GetItemClass(int32 Slot) const
{
	const APlayerCharacter* Player = Cast<APlayerCharacter>(GetOwner());
	if (Slot == ThrowableSlot && Player && Player->GetThrowInventory())
	{
		return Player->GetThrowInventory()->GetCarriedClass();
	}
	if (Slot == KeySlot)
	{
		return KeyItemClass;
	}
	return nullptr;
}

const APickupBase* UItemSlotsComponent::GetItemDefaults(int32 Slot) const
{
	const TSubclassOf<APickupBase> Class = GetItemClass(Slot);
	return Class ? Class->GetDefaultObject<APickupBase>() : nullptr;
}

int32 UItemSlotsComponent::GetCount(int32 Slot) const
{
	const APlayerCharacter* Player = Cast<APlayerCharacter>(GetOwner());
	if (Slot == ThrowableSlot && Player && Player->GetThrowInventory())
	{
		return Player->GetThrowInventory()->GetCount();
	}
	return Slot == KeySlot ? KeyCount : 0;
}

UTexture2D* UItemSlotsComponent::GetIcon(int32 Slot) const
{
	const APickupBase* Item = GetItemDefaults(Slot);
	return Item ? Item->GetSlotIcon() : nullptr;
}

void UItemSlotsComponent::AddKey(TSubclassOf<APickupBase> KeyClass)
{
	if (KeyClass)
	{
		KeyItemClass = KeyClass;
	}
	++KeyCount;
}

void UItemSlotsComponent::SelectSlot(int32 Slot)
{
	// 同じ番号でしまう。空の欄は持たない
	SelectedSlot = (Slot == SelectedSlot || GetCount(Slot) <= 0) ? NoSlot : Slot;
	ShowHeld();
}

void UItemSlotsComponent::RefreshHeld()
{
	if (SelectedSlot != NoSlot && GetCount(SelectedSlot) <= 0)
	{
		SelectedSlot = NoSlot;
	}
	ShowHeld();
}

void UItemSlotsComponent::ShowHeld()
{
	if (!HeldMesh)
	{
		return;
	}
	const APickupBase* Item = SelectedSlot != NoSlot ? GetItemDefaults(SelectedSlot) : nullptr;
	const UStaticMeshComponent* Source = Item ? Item->GetMesh() : nullptr;
	if (!Source || !Source->GetStaticMesh())
	{
		HeldMesh->SetVisibility(false);
		return;
	}

	// 欄の物の本体メッシュ（BP で付けたマテリアルも）をそのまま手元に出す
	HeldMesh->SetStaticMesh(Source->GetStaticMesh());
	for (int32 i = 0; i < Source->GetNumMaterials(); ++i)
	{
		HeldMesh->SetMaterial(i, Source->GetMaterial(i));
	}
	// BP で本体メッシュを拡大縮小している物（仮の円柱のビンなど）もあるので、その倍率も掛ける
	FTransform Held = Item->GetHeldTransform();
	Held.SetScale3D(Held.GetScale3D() * Source->GetRelativeScale3D());
	HeldMesh->SetRelativeTransform(Held);
	HeldMesh->SetVisibility(true);
}

ADoor* UItemSlotsComponent::AsKeyDoor(const AActor* Focused) const
{
	// 脱出扉は ADoor ではないので、ここで自然に外れる
	ADoor* Door = Cast<ADoor>(const_cast<AActor*>(Focused));
	return Door && Door->IsClosed() ? Door : nullptr;
}

bool UItemSlotsComponent::CanUseKeyOn(const AActor* Focused) const
{
	const APlayerCharacter* Player = Cast<APlayerCharacter>(GetOwner());
	const bool bWarping = Player && Player->GetDoorWarp() && Player->GetDoorWarp()->IsWarping();
	return SelectedSlot == KeySlot && KeyCount > 0 && !bWarping && AsKeyDoor(Focused) != nullptr;
}

bool UItemSlotsComponent::UseSelected(AActor* Focused)
{
	APlayerCharacter* Player = Cast<APlayerCharacter>(GetOwner());
	if (!Player || Player->IsHiddenInLocker())
	{
		return false;
	}

	bool bUsed = false;
	if (SelectedSlot == ThrowableSlot && Player->GetThrowInventory())
	{
		const UCameraComponent* Camera = Player->GetFirstPersonCameraComponent();
		bUsed = Player->GetThrowInventory()->Throw(Camera->GetComponentLocation(), Camera->GetForwardVector());
	}
	else if (SelectedSlot == KeySlot && CanUseKeyOn(Focused))
	{
		bUsed = Player->GetDoorWarp()->Begin(AsKeyDoor(Focused));
		if (bUsed)
		{
			--KeyCount;
			UE_LOG(LogQuietus, Log, TEXT("Used a key on %s (%d left)"), *GetNameSafe(Focused), KeyCount);
		}
	}
	RefreshHeld();
	return bUsed;
}
