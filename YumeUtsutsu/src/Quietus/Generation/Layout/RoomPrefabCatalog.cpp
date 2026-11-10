#include "Generation/Layout/RoomPrefabCatalog.h"
#include "Engine/World.h"
#include "Generation/RoomPrefab.h"

namespace LevelGen
{
	void FRoomPrefabCatalog::Build(UWorld* World, const TArray<TSubclassOf<ARoomPrefab>>& Classes)
	{
		Entries.Reset();
		Entries.Reserve(Classes.Num());

		for (const TSubclassOf<ARoomPrefab>& PrefabClass : Classes)
		{
			FPrefabEntry& Entry = Entries.AddDefaulted_GetRef();
			if (!PrefabClass)
			{
				continue;
			}

			const ARoomPrefab* CDO = PrefabClass->GetDefaultObject<ARoomPrefab>();
			Entry.Class = PrefabClass;
			Entry.ThemeName = CDO->GetThemeName();
			Entry.Footprint = CDO->GetFootprintTiles();
			Entry.bCanBeStart = CDO->CanBeStartRoom();
			Entry.bCanBeExit = CDO->CanBeExitRoom();
			Entry.MaxInstances = CDO->GetMaxInstancesPerMap();

			ARoomPrefab* Probe = SpawnRoom(World, Entries.Num() - 1, FVector::ZeroVector);
			if (!Probe)
			{
				continue;
			}
			TArray<UDoorSocketComponent*> Sockets;
			Probe->GetDoorSockets(Sockets);
			for (const UDoorSocketComponent* Socket : Sockets)
			{
				FPrefabSocket& Out = Entry.Sockets.AddDefaulted_GetRef();
				Out.Name = Socket->GetName();
				Out.LocalLocation = Socket->GetComponentLocation() - Probe->GetActorLocation();
				Out.Forward = Socket->GetForwardVector();
			}
			Probe->Destroy();
		}
	}

	ARoomPrefab* FRoomPrefabCatalog::SpawnRoom(UWorld* World, int32 Index, const FVector& Origin) const
	{
		if (!World || !Entries.IsValidIndex(Index) || !Entries[Index].IsValid())
		{
			return nullptr;
		}
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		return World->SpawnActor<ARoomPrefab>(Entries[Index].Class, Origin, FRotator::ZeroRotator, Params);
	}
}
