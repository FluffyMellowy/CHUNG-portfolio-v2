#include "Environment/FlickerLight.h"
#include "Components/PointLightComponent.h"

AFlickerLight::AFlickerLight()
{
	PrimaryActorTick.bCanEverTick = true;

	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	SetRootComponent(Light);
	Light->SetMobility(EComponentMobility::Movable);
	Light->SetIntensityUnits(ELightUnits::Candelas);
	Light->SetIntensity(BaseIntensity);
	Light->SetAttenuationRadius(900.0f);
	Light->SetLightColor(FLinearColor(1.0f, 0.9f, 0.75f));
	Light->SetCastShadows(true);
}

void AFlickerLight::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 消灯中はその継続時間を消化する
	if (DropoutTimer > 0.0f)
	{
		DropoutTimer -= DeltaSeconds;
		if (DropoutTimer <= 0.0f)
		{
			Light->SetIntensity(BaseIntensity);
		}
		return;
	}

	Timer += DeltaSeconds;
	if (Timer < FlickerInterval)
	{
		return;
	}
	Timer = 0.0f;

	if (FMath::FRand() < DropoutChance)
	{
		Light->SetIntensity(0.0f);
		DropoutTimer = DropoutDuration;
		return;
	}
	Light->SetIntensity(BaseIntensity * FMath::FRandRange(FlickerMin, FlickerMax));
}
