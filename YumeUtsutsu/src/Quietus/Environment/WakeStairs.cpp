#include "Environment/WakeStairs.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/RunGameState.h"
#include "Engine/World.h"
#include "Interactables/AlarmClock.h"
#include "Player/PlayerCharacter.h"

AWakeStairs::AWakeStairs()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	// 扉をくぐってすぐの入口の踊り場（扉の厚みの分だけ奥から）
	SafeZone = CreateDefaultSubobject<UBoxComponent>(TEXT("SafeZone"));
	SafeZone->SetupAttachment(Root);
	SafeZone->SetRelativeLocation(FVector(100.0f, 0.0f, 120.0f));
	SafeZone->SetBoxExtent(FVector(70.0f, 100.0f, 120.0f));
	SafeZone->SetCollisionProfileName(TEXT("Trigger"));

	Table = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Table"));
	Table->SetupAttachment(Root);
}

void AWakeStairs::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// ピース・見えない壁・灯りはここで毎回作り直す（構築スクリプト扱いなので再構築時に自動で消える）
	FBox Previous(ForceInit);
	for (int32 Index = 0; Index < Modules.Num(); ++Index)
	{
		const FWakeStairModule& Module = Modules[Index];
		if (!Module.Mesh)
		{
			continue;
		}

		UStaticMeshComponent* Piece = NewObject<UStaticMeshComponent>(this);
		Piece->CreationMethod = EComponentCreationMethod::UserConstructionScript;
		Piece->SetStaticMesh(Module.Mesh);
		Piece->SetupAttachment(Root);
		Piece->SetRelativeLocation(Module.Location);
		Piece->RegisterComponent();

		const FBox Box = Module.Mesh->GetBoundingBox().ShiftBy(Module.Location);

		// 両脇の壁
		const float T = BarrierThickness;
		AddBarrier(FVector(Box.Min.X, Box.Max.Y, Box.Min.Z), FVector(Box.Max.X, Box.Max.Y + T, Box.Max.Z + BarrierHeight));
		AddBarrier(FVector(Box.Min.X, Box.Min.Y - T, Box.Min.Z), FVector(Box.Max.X, Box.Min.Y, Box.Max.Z + BarrierHeight));

		// 前のピースと幅が違う継ぎ目は、広い側の張り出しを塞ぐ
		if (Previous.IsValid && !FMath::IsNearlyEqual(Previous.Max.Y, Box.Max.Y, 1.0f))
		{
			const FBox& Wide = Previous.Max.Y > Box.Max.Y ? Previous : Box;
			const FBox& Narrow = Previous.Max.Y > Box.Max.Y ? Box : Previous;
			const float Z0 = FMath::Min(Wide.Min.Z, Narrow.Min.Z);
			const float Z1 = FMath::Max(Wide.Max.Z, Narrow.Max.Z) + BarrierHeight;
			const float X = Box.Min.X;
			AddBarrier(FVector(X - T, Narrow.Max.Y, Z0), FVector(X, Wide.Max.Y + T, Z1));
			AddBarrier(FVector(X - T, Wide.Min.Y - T, Z0), FVector(X, Narrow.Min.Y, Z1));
		}

		// 終端
		if (Index == Modules.Num() - 1)
		{
			AddBarrier(FVector(Box.Max.X, Box.Min.Y - T, Box.Min.Z), FVector(Box.Max.X + T, Box.Max.Y + T, Box.Max.Z + BarrierHeight));
		}

		// 平らなピース（踊り場・頂上）の上に灯り
		if (Box.GetSize().Z < 30.0f)
		{
			UPointLightComponent* Light = NewObject<UPointLightComponent>(this);
			Light->CreationMethod = EComponentCreationMethod::UserConstructionScript;
			Light->SetupAttachment(Root);
			Light->SetRelativeLocation(FVector(Box.GetCenter().X, 0.0f, Box.Max.Z + LightHeight));
			Light->SetMobility(EComponentMobility::Movable);
			Light->SetIntensityUnits(ELightUnits::Candelas);
			Light->SetIntensity(LightIntensity);
			Light->SetLightColor(LightColor);
			Light->SetAttenuationRadius(LightRadius);
			Light->RegisterComponent();
		}
		Previous = Box;
	}
}

void AWakeStairs::AddBarrier(const FVector& Min, const FVector& Max)
{
	UBoxComponent* Barrier = NewObject<UBoxComponent>(this);
	Barrier->CreationMethod = EComponentCreationMethod::UserConstructionScript;
	Barrier->SetupAttachment(Root);
	Barrier->SetRelativeLocation((Min + Max) * 0.5f);
	Barrier->SetBoxExtent((Max - Min) * 0.5f);
	Barrier->SetCollisionProfileName(TEXT("InvisibleWall"));
	Barrier->RegisterComponent();
}

void AWakeStairs::BeginPlay()
{
	Super::BeginPlay();

	SafeZone->OnComponentBeginOverlap.AddDynamic(this, &AWakeStairs::OnSafeZoneOverlap);

	if (ClockClass && Table->DoesSocketExist(ClockSocket))
	{
		FActorSpawnParameters Params;
		Params.Owner = this;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		GetWorld()->SpawnActor<AAlarmClock>(ClockClass, Table->GetSocketTransform(ClockSocket), Params);
	}
}

void AWakeStairs::OnSafeZoneOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!Cast<APlayerCharacter>(OtherActor))
	{
		return;
	}
	if (ARunGameState* GameState = ARunGameState::Get(this))
	{
		GameState->EnterSafeZone();
	}
}
