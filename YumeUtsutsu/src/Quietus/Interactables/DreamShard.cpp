#include "Interactables/DreamShard.h"
#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/RunGameState.h"

#define LOCTEXT_NAMESPACE "Interaction"

ADreamShard::ADreamShard()
{
	// 漂う動きのためだけに Tick を使う
	PrimaryActorTick.bCanEverTick = true;

	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCastShadow(false);

	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetupAttachment(GetMesh());
	InteractionSphere->InitSphereRadius(18.0f);
	InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionSphere->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow"));
	Glow->SetupAttachment(GetMesh());
	Glow->SetMobility(EComponentMobility::Movable);
	Glow->SetIntensityUnits(ELightUnits::Candelas);
	Glow->SetIntensity(8.0f);
	Glow->SetAttenuationRadius(250.0f);
	Glow->SetLightColor(FLinearColor(0.55f, 0.75f, 1.0f));
	Glow->SetCastShadows(false);
}

void ADreamShard::BeginPlay()
{
	Super::BeginPlay();

	BaseLocation = GetActorLocation() + FVector(0.0f, 0.0f, HoverHeight);
	SetActorLocation(BaseLocation);
}

void ADreamShard::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	Age += DeltaSeconds;
	const float Bob = BobAmplitude * FMath::Sin(Age * UE_TWO_PI / BobPeriod);
	SetActorLocationAndRotation(BaseLocation + FVector(0.0f, 0.0f, Bob),
		GetActorRotation() + FRotator(0.0f, SpinSpeed * DeltaSeconds, 0.0f));
}

void ADreamShard::OnPickedUp(APlayerCharacter* Interactor)
{
	if (ARunGameState* GameState = ARunGameState::Get(this))
	{
		GameState->AddShard();
	}
}

FText ADreamShard::GetInteractionPrompt_Implementation() const
{
	return LOCTEXT("PickUpShard", "夢の欠片を拾う");
}

#undef LOCTEXT_NAMESPACE
