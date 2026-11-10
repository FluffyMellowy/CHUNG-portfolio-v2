#include "RoomPrefab.h"

ARoomPrefab::ARoomPrefab()
{
	PrimaryActorTick.bCanEverTick = false;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
	GetRootComponent()->SetMobility(EComponentMobility::Static);
}

void ARoomPrefab::GetDoorSockets(TArray<UDoorSocketComponent*>& OutSockets) const
{
	GetComponents<UDoorSocketComponent>(OutSockets);
}

void ARoomPrefab::GetMarkersOfType(ESpawnMarkerType Type, TArray<USpawnMarkerComponent*>& OutMarkers) const
{
	OutMarkers.Reset();
	TArray<USpawnMarkerComponent*> All;
	GetComponents<USpawnMarkerComponent>(All);
	for (USpawnMarkerComponent* Marker : All)
	{
		if (Marker->MarkerType == Type)
		{
			OutMarkers.Add(Marker);
		}
	}
}

