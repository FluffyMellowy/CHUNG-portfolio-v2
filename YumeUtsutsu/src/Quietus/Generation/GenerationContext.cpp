#include "Generation/GenerationContext.h"
#include "Engine/World.h"
#include "Generation/RoomPrefab.h"

namespace LevelGen
{
	FVector FContext::CellCenter(const FIntPoint& Cell, float Z) const
	{
		return Origin + FVector((Cell.X + 0.5f) * CellSize, (Cell.Y + 0.5f) * CellSize, Z);
	}

	FIntPoint FContext::WorldToCell(const FVector& Location) const
	{
		const FVector Local = (Location - Origin) / CellSize;
		return FIntPoint(FMath::FloorToInt(Local.X), FMath::FloorToInt(Local.Y));
	}

	FVector FContext::EdgeCenter(const FIntPoint& Cell, const FIntPoint& Dir) const
	{
		return CellCenter(Cell) + FVector(Dir.X, Dir.Y, 0) * (CellSize * 0.5f);
	}

	ARoomPrefab* FContext::GetRoomActor(int32 RoomIdx) const
	{
		return RoomActors.IsValidIndex(RoomIdx) ? RoomActors[RoomIdx] : nullptr;
	}

	AActor* FContext::SpawnActor(UClass* Class, const FVector& Location, const FRotator& Rotation) const
	{
		if (!Class || !World)
		{
			return nullptr;
		}
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		return World->SpawnActor<AActor>(Class, Location, Rotation, Params);
	}
}
