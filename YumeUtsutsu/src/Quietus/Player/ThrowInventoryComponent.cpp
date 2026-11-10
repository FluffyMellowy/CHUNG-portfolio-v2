#include "Player/ThrowInventoryComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Interactables/Throwable.h"

UThrowInventoryComponent::UThrowInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	ThrowableClass = AThrowable::StaticClass();
}

void UThrowInventoryComponent::Add(TSubclassOf<AThrowable> PickedClass)
{
	if (PickedClass)
	{
		CarriedClass = PickedClass;
	}
	++ThrowableCount;
}

bool UThrowInventoryComponent::Throw(const FVector& ViewLocation, const FVector& ViewDirection)
{
	const TSubclassOf<AThrowable> SpawnClass = CarriedClass ? CarriedClass : ThrowableClass;
	if (ThrowableCount <= 0 || !SpawnClass)
	{
		return false;
	}

	// 視点の少し前から視線方向へ投げる（軽く上向きに補正）
	APawn* Thrower = Cast<APawn>(GetOwner());
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.Instigator = Thrower;
	AThrowable* Thrown = GetWorld()->SpawnActor<AThrowable>(SpawnClass,
		ViewLocation + ViewDirection * ThrowSpawnOffset, ViewDirection.Rotation(), Params);
	if (!Thrown)
	{
		return false;
	}
	Thrown->Launch(Thrower, ViewDirection * ThrowSpeed + FVector(0, 0, ThrowUpwardBoost));
	--ThrowableCount;
	return true;
}
