#include "Interactables/Balloon.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/DreamGimmicks.h"
#include "Interactables/Throwable.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Player/PlayerCharacter.h"
#include "Quietus.h"
#include "Sound/SoundBase.h"

ABalloon::ABalloon()
{
	// 揺れのためだけに Tick する
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCanEverAffectNavigation(false);

	// プレイヤーは素通りしながら割る（オーバーラップ）。投げた物は当たって跳ね返る（ブロック）
	PopTrigger = CreateDefaultSubobject<USphereComponent>(TEXT("PopTrigger"));
	PopTrigger->SetupAttachment(Mesh);
	PopTrigger->InitSphereRadius(16.0f);
	PopTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PopTrigger->SetCollisionObjectType(ECC_WorldDynamic);
	PopTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	PopTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	PopTrigger->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Block);
	PopTrigger->SetGenerateOverlapEvents(true);
	PopTrigger->SetCanEverAffectNavigation(false);
}

void ABalloon::BeginPlay()
{
	Super::BeginPlay();

	PopTrigger->OnComponentBeginOverlap.AddDynamic(this, &ABalloon::OnTriggerOverlap);
	PopTrigger->OnComponentHit.AddDynamic(this, &ABalloon::OnTriggerHit);

	BaseMeshLocation = Mesh->GetRelativeLocation();
	// 風船ごとに揺れの位相をずらす
	PhaseA = FMath::FRandRange(0.0f, UE_TWO_PI);
	PhaseB = FMath::FRandRange(0.0f, UE_TWO_PI);
}

void ABalloon::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 紐の下端を軸に、2 方向へゆっくり違う周期で傾けてふわふわ漂わせる
	Age += DeltaSeconds;
	const float Pitch = SwayAngle * FMath::Sin(Age * SwaySpeed + PhaseA);
	const float Roll = SwayAngle * FMath::Sin(Age * SwaySpeed * 0.73f + PhaseB);
	const float Bob = BobAmplitude * FMath::Sin(Age * SwaySpeed * 0.5f + PhaseB);
	Mesh->SetRelativeLocationAndRotation(BaseMeshLocation + FVector(0.0f, 0.0f, Bob),
		FRotator(Pitch, Mesh->GetRelativeRotation().Yaw, Roll));
}

void ABalloon::SetColor(const FLinearColor& Color)
{
	if (UMaterialInstanceDynamic* Body = Mesh->CreateDynamicMaterialInstance(BodyMaterialIndex))
	{
		Body->SetVectorParameterValue(ColorParameter, Color);
		UE_LOG(LogQuietus, Verbose, TEXT("%s color %s"), *GetName(), *Color.ToString());
	}
}

void ABalloon::OnTriggerOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (APlayerCharacter* Player = Cast<APlayerCharacter>(OtherActor))
	{
		Pop(Player);
	}
}

void ABalloon::OnTriggerHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	if (const AThrowable* Thrown = Cast<AThrowable>(OtherActor))
	{
		Pop(Thrown->GetThrower());
	}
}

void ABalloon::Pop(APawn* NoiseInstigator)
{
	if (bPopped)
	{
		return;
	}
	bPopped = true;

	const UDreamGimmickSettings& Settings = UDreamGimmickSettings::Get();
	const FVector Location = PopTrigger->GetComponentLocation();
	if (USoundBase* Sound = Settings.BalloonPopSound.LoadSynchronous())
	{
		UGameplayStatics::PlaySoundAtLocation(this, Sound, Location);
	}

	// ピエロには「プレイヤーの立てた大きな音」として届く
	if (NoiseInstigator)
	{
		NoiseInstigator->MakeNoise(Settings.BalloonNoiseLoudness, NoiseInstigator, Location);
	}
	UE_LOG(LogQuietus, Log, TEXT("%s popped at %s"), *GetName(), *Location.ToCompactString());

	// 割れた破片の演出は後で（VFX パック待ち）。今は消すだけ
	Mesh->SetVisibility(false);
	PopTrigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetActorTickEnabled(false);
	SetLifeSpan(3.0f);
}
