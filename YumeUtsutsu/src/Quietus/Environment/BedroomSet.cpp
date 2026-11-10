#include "Environment/BedroomSet.h"
#include "Camera/CameraActor.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Materials/MaterialInstanceDynamic.h"

ABedroomSet::ABedroomSet()
{
	PrimaryActorTick.bCanEverTick = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void ABedroomSet::ApplyState(EBedroomState State)
{
	const bool bNight = State == EBedroomState::Night;
	auto Show = [](const TArray<TObjectPtr<AActor>>& Actors, bool bShow)
	{
		for (AActor* Actor : Actors)
		{
			if (Actor)
			{
				Actor->SetActorHiddenInGame(!bShow);
			}
		}
	};
	Show(NightActors, bNight);
	Show(MorningActors, State == EBedroomState::Morning);
	Show(DreamActors, State == EBedroomState::Dream);

	if (UStaticMeshComponent* LampMesh = Lamp ? Lamp->GetStaticMeshComponent() : nullptr)
	{
		const int32 Slot = LampMesh->GetMaterialIndex(LampSlot);
		if (UMaterialInstanceDynamic* Bulb = Slot != INDEX_NONE ? LampMesh->CreateDynamicMaterialInstance(Slot) : nullptr)
		{
			Bulb->SetScalarParameterValue(TEXT("EmissiveStrength"), bNight ? LampEmissionNight : 0.0f);
		}
	}
}

ACameraActor* ABedroomSet::GetCamera(EBedroomState State) const
{
	switch (State)
	{
	case EBedroomState::Morning: return MorningCamera;
	case EBedroomState::Dream:   return DreamCamera;
	default:                     return TitleCamera;
	}
}
