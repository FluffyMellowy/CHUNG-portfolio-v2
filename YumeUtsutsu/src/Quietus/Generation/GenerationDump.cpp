#include "Generation/GenerationDump.h"

#if !UE_BUILD_SHIPPING

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Misc/FileHelper.h"
#include "Quietus.h"

namespace
{
	FString FormatTransform(const FTransform& Transform)
	{
		const FVector L = Transform.GetLocation();
		const FRotator R = Transform.Rotator();
		const FVector S = Transform.GetScale3D();
		return FString::Printf(TEXT("L(%.2f %.2f %.2f) R(%.2f %.2f %.2f) S(%.3f %.3f %.3f)"),
			L.X, L.Y, L.Z, R.Pitch, R.Yaw, R.Roll, S.X, S.Y, S.Z);
	}

	/** メッシュを持つコンポーネントなら、割り当てられているアセット名を返す */
	FString DescribeMesh(const USceneComponent* Comp)
	{
		if (const UStaticMeshComponent* StaticMesh = Cast<UStaticMeshComponent>(Comp))
		{
			return FString::Printf(TEXT(" mesh=%s"), *GetNameSafe(StaticMesh->GetStaticMesh()));
		}
		if (const USkeletalMeshComponent* Skeletal = Cast<USkeletalMeshComponent>(Comp))
		{
			return FString::Printf(TEXT(" mesh=%s"), *GetNameSafe(Skeletal->GetSkeletalMeshAsset()));
		}
		return FString();
	}
}

namespace LevelGen
{
	void DumpWorldLayout(UWorld* World, const FString& FilePath)
	{
		if (!World)
		{
			return;
		}

		// アクターはスポーン順（＝乱数の消費順）に並ぶので、そのままの順で書く
		TArray<FString> Lines;
		int32 NumComponents = 0;
		int32 NumInstances = 0;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			const AActor* Actor = *It;
			Lines.Add(FString::Printf(TEXT("%s %s"), *Actor->GetClass()->GetName(), *FormatTransform(Actor->GetActorTransform())));

			TInlineComponentArray<USceneComponent*> Components(Actor);
			for (const USceneComponent* Comp : Components)
			{
				Lines.Add(FString::Printf(TEXT("  %s:%s %s%s"), *Comp->GetClass()->GetName(), *Comp->GetName(),
					*FormatTransform(Comp->GetComponentTransform()), *DescribeMesh(Comp)));
				++NumComponents;

				if (const UInstancedStaticMeshComponent* ISM = Cast<UInstancedStaticMeshComponent>(Comp))
				{
					for (int32 i = 0; i < ISM->GetInstanceCount(); ++i)
					{
						FTransform Instance;
						ISM->GetInstanceTransform(i, Instance, /*bWorldSpace=*/ true);
						Lines.Add(FString::Printf(TEXT("    [%d] %s"), i, *FormatTransform(Instance)));
						++NumInstances;
					}
				}
			}
		}

		if (FFileHelper::SaveStringArrayToFile(Lines, *FilePath))
		{
			UE_LOG(LogQuietus, Log, TEXT("GenDump: wrote %d lines (%d components, %d instances) to %s"), Lines.Num(), NumComponents, NumInstances, *FilePath);
		}
		else
		{
			UE_LOG(LogQuietus, Error, TEXT("GenDump: failed to write %s"), *FilePath);
		}
	}
}

#endif
