#include "Interactables/PickupBase.h"
#include "Components/StaticMeshComponent.h"

APickupBase::APickupBase()
{
	// 拾われるまで何も更新しないのでTickは不要
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
}

void APickupBase::Interact_Implementation(APlayerCharacter* Interactor)
{
	if (!CanBePickedUp(Interactor))
	{
		return;
	}
	OnPickedUp(Interactor);
	Destroy();
}
