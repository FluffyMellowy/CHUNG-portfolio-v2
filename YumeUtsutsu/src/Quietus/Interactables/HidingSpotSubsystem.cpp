#include "Interactables/HidingSpotSubsystem.h"
#include "GameFramework/Actor.h"
#include "Interactables/HidingSpot.h"

void UHidingSpotSubsystem::Register(AActor* Spot)
{
	if (Spot && Spot->Implements<UHidingSpot>())
	{
		Spots.AddUnique(Spot);
	}
}

void UHidingSpotSubsystem::Unregister(AActor* Spot)
{
	Spots.Remove(Spot);
}

AActor* UHidingSpotSubsystem::FindOccupiedNear(const FVector& Location, float Radius) const
{
	for (const TWeakObjectPtr<AActor>& Weak : Spots)
	{
		AActor* Spot = Weak.Get();
		const IHidingSpot* HidingSpot = Cast<IHidingSpot>(Spot);
		if (HidingSpot && HidingSpot->IsOccupied() && FVector::Dist2D(Spot->GetActorLocation(), Location) <= Radius)
		{
			return Spot;
		}
	}
	return nullptr;
}
