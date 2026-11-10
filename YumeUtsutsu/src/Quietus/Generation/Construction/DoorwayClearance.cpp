#include "Generation/Construction/DoorwayClearance.h"
#include "Components/StaticMeshComponent.h"
#include "Generation/Construction/GeometryBuilder.h"
#include "Generation/RoomPrefab.h"

namespace LevelGen
{
	namespace
	{
		/** 開口150cmの半分＋余白 */
		constexpr float DoorwayHalfWidth = 85.0f;

		/**
		 *  扉の開口中心から、壁沿い ±DoorwayHalfWidth × 通行方向 ±Depth の軸整列ボックスを作る。
		 *  壁は常に軸整列なので回転は考えなくてよい
		 */
		FBox MakeClearanceBox(const FDoorway& Doorway, float Depth)
		{
			const FVector Pos = Doorway.Transform.GetLocation();
			const float Yaw = FMath::UnwindDegrees(Doorway.Transform.Rotator().Yaw);
			const bool bWallAlongX = FMath::IsNearlyZero(Yaw, 1.0f) || FMath::IsNearlyEqual(FMath::Abs(Yaw), 180.0f, 1.0f);
			const FVector Extent = bWallAlongX
				? FVector(DoorwayHalfWidth, Depth, 0.0f)
				: FVector(Depth, DoorwayHalfWidth, 0.0f);
			return FBox(Pos - Extent - FVector(0, 0, 10.0f), Pos + Extent + FVector(0, 0, 220.0f));
		}
	}

	int32 ClearDressingAtDoorways(const TArray<FDoorway>& Doorways, const TArray<ARoomPrefab*>& RoomActors, float Depth)
	{
		if (Doorways.Num() == 0 || RoomActors.Num() == 0)
		{
			return 0;
		}

		TArray<FBox> ClearBoxes;
		ClearBoxes.Reserve(Doorways.Num());
		for (const FDoorway& Doorway : Doorways)
		{
			ClearBoxes.Add(MakeClearanceBox(Doorway, Depth));
		}

		int32 NumCulled = 0;
		for (ARoomPrefab* Room : RoomActors)
		{
			if (!Room)
			{
				continue;
			}
			TInlineComponentArray<UStaticMeshComponent*> Meshes(Room);
			for (UStaticMeshComponent* Mesh : Meshes)
			{
				if (!Mesh->GetStaticMesh() || Mesh->GetCollisionEnabled() == ECollisionEnabled::NoCollision)
				{
					continue;
				}
				const FBox MeshBounds = Mesh->Bounds.GetBox();
				for (const FBox& Clear : ClearBoxes)
				{
					if (Clear.Intersect(MeshBounds))
					{
						Mesh->SetVisibility(false);
						Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
						++NumCulled;
						break;
					}
				}
			}
		}
		return NumCulled;
	}
}
