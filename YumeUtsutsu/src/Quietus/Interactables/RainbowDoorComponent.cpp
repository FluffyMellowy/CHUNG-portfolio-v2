#include "Interactables/RainbowDoorComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/DreamGimmicks.h"
#include "Core/RunGameState.h"
#include "Enemies/EnemyCharacter.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Interactables/Door.h"
#include "Materials/MaterialInterface.h"
#include "NavAreas/NavArea_Null.h"
#include "NavigationSystem.h"
#include "Player/PlayerCharacter.h"
#include "Quietus.h"
#include "TimerManager.h"

namespace
{
	/** 灯りを扉の面から離す距離 (cm) */
	constexpr float GlowOffset = 130.0f;

	/** ドアの開口（150x210）をナビ上で塞ぐ箱の半径 */
	const FVector NavBlockerExtent(80.0f, 25.0f, 105.0f);
}

URainbowDoorComponent::URainbowDoorComponent()
{
	// 光の色を巡らせるためだけに Tick する
	PrimaryComponentTick.bCanEverTick = true;
}

void URainbowDoorComponent::OnRegister()
{
	Super::OnRegister();

	AActor* Owner = GetOwner();
	if (!Owner || !Owner->GetWorld() || !Owner->GetWorld()->IsGameWorld())
	{
		return;
	}
	const UDreamGimmickSettings& Settings = UDreamGimmickSettings::Get();

	// 扉板の面に置くと板だけが白飛びするので、扉の両側へ少し離して1つずつ置く（通り抜け方向はローカル Y）
	for (const float Side : { 1.0f, -1.0f })
	{
		UPointLightComponent* Glow = NewObject<UPointLightComponent>(Owner);
		Glow->SetupAttachment(this);
		Glow->SetRelativeLocation(FVector(0.0f, Side * GlowOffset, 250.0f));
		Glow->SetMobility(EComponentMobility::Movable);
		Glow->SetIntensityUnits(ELightUnits::Candelas);
		Glow->SetIntensity(Settings.RainbowLightIntensity);
		Glow->SetAttenuationRadius(Settings.RainbowLightRadius);
		Glow->SetCastShadows(false);
		Glow->RegisterComponent();
		Glows.Add(Glow);
	}

	NavBlocker = NewObject<UBoxComponent>(Owner);
	NavBlocker->SetupAttachment(this);
	NavBlocker->SetRelativeLocation(FVector(0.0f, 0.0f, NavBlockerExtent.Z));
	NavBlocker->SetBoxExtent(NavBlockerExtent);
	NavBlocker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	NavBlocker->bDynamicObstacle = true;
	NavBlocker->SetAreaClassOverride(UNavArea_Null::StaticClass());
	NavBlocker->SetCanEverAffectNavigation(true);
	NavBlocker->RegisterComponent();

	ApplySurface();
}

void URainbowDoorComponent::OnUnregister()
{
	for (UPointLightComponent* Glow : Glows)
	{
		if (Glow)
		{
			Glow->DestroyComponent();
		}
	}
	Glows.Reset();
	if (NavBlocker)
	{
		NavBlocker->DestroyComponent();
		NavBlocker = nullptr;
	}
	RestoreSurface();
	Super::OnUnregister();
}

ADoor* URainbowDoorComponent::GetDoor() const
{
	return Cast<ADoor>(GetOwner());
}

TArray<UMeshComponent*> URainbowDoorComponent::GetDoorMeshes() const
{
	TArray<UMeshComponent*> Meshes;
	if (const AActor* Owner = GetOwner())
	{
		TArray<UStaticMeshComponent*> StaticMeshes;
		Owner->GetComponents(StaticMeshes);
		for (UStaticMeshComponent* Mesh : StaticMeshes)
		{
			if (Mesh->GetStaticMesh())
			{
				Meshes.Add(Mesh);
			}
		}
	}
	return Meshes;
}

void URainbowDoorComponent::ApplySurface()
{
	UMaterialInterface* Surface = UDreamGimmickSettings::Get().RainbowSurfaceMaterial.LoadSynchronous();
	if (!Surface)
	{
		UE_LOG(LogQuietus, Warning, TEXT("%s: rainbow surface material is not set"), *GetNameSafe(GetOwner()));
		return;
	}
	for (UMeshComponent* Mesh : GetDoorMeshes())
	{
		FRainbowSavedMaterials& Saved = SavedMaterials.Add(Mesh);
		for (int32 i = 0; i < Mesh->GetNumMaterials(); ++i)
		{
			Saved.Materials.Add(Mesh->GetMaterial(i));
			Mesh->SetMaterial(i, Surface);
		}
	}
}

void URainbowDoorComponent::RestoreSurface()
{
	for (const TPair<TObjectPtr<UMeshComponent>, FRainbowSavedMaterials>& Pair : SavedMaterials)
	{
		if (UMeshComponent* Mesh = Pair.Key)
		{
			for (int32 i = 0; i < Pair.Value.Materials.Num(); ++i)
			{
				Mesh->SetMaterial(i, Pair.Value.Materials[i]);
			}
		}
	}
	SavedMaterials.Reset();
}

void URainbowDoorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 灯りの色相をゆっくり一周させ、明るさもかすかに揺らす
	Age += DeltaTime;
	const FLinearColor Color = FLinearColor::MakeFromHSV8(static_cast<uint8>(FMath::Fmod(Age * 18.0f, 255.0f)), 90, 255);
	const float Intensity = UDreamGimmickSettings::Get().RainbowLightIntensity * (0.85f + 0.15f * FMath::Sin(Age * 1.7f));
	for (UPointLightComponent* Glow : Glows)
	{
		Glow->SetLightColor(Color);
		Glow->SetIntensity(Intensity);
	}
}

void URainbowDoorComponent::Use(APlayerCharacter* Player)
{
	if (!bCharged || bInUse || !Player)
	{
		return;
	}
	bInUse = true;
	Traveler = Player;

	// 操作を止めて画面を光で満たす
	if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
	{
		PC->SetIgnoreMoveInput(true);
		PC->SetIgnoreLookInput(true);
		if (PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StartCameraFade(0.0f, 1.0f, UDreamGimmickSettings::Get().WhiteoutTime,
				FLinearColor::White, false, /*bHoldWhenFinished=*/ true);
		}
	}
	GetWorld()->GetTimerManager().SetTimer(StepTimer, this, &URainbowDoorComponent::Teleport,
		UDreamGimmickSettings::Get().WhiteoutTime + 0.05f, false);
}

void URainbowDoorComponent::Teleport()
{
	APlayerCharacter* Player = Traveler.Get();
	if (!Player)
	{
		Revert();
		return;
	}

	const FVector Departure = GetOwner()->GetActorLocation();
	FVector Location;
	FRotator Rotation;
	if (PickArrival(Player, Location, Rotation))
	{
		Player->TeleportTo(Location, Rotation);
		if (AController* Controller = Player->GetController())
		{
			Controller->SetControlRotation(Rotation);
		}
		UE_LOG(LogQuietus, Log, TEXT("%s: rainbow door -> %s"), *GetOwner()->GetName(), *Location.ToCompactString());

		// 追っていたピエロはここで見失い、出発した扉のあたりを探す
		if (ARunGameState* GameState = ARunGameState::Get(this))
		{
			GameState->NotifyPlayerTeleported(Departure);
		}
	}
	else
	{
		UE_LOG(LogQuietus, Warning, TEXT("%s: no arrival door found, staying"), *GetOwner()->GetName());
	}

	if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
	{
		if (PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StartCameraFade(1.0f, 0.0f, UDreamGimmickSettings::Get().RevealTime, FLinearColor::White);
		}
	}
	GetWorld()->GetTimerManager().SetTimer(StepTimer, this, &URainbowDoorComponent::FinishReveal,
		UDreamGimmickSettings::Get().RevealTime * 0.3f, false);
}

void URainbowDoorComponent::FinishReveal()
{
	// 光が引き始めたら操作を返し、この扉は普通の扉に戻す
	if (APlayerCharacter* Player = Traveler.Get())
	{
		if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
		{
			PC->SetIgnoreMoveInput(false);
			PC->SetIgnoreLookInput(false);
		}
	}
	Revert();
}

void URainbowDoorComponent::Revert()
{
	bCharged = false;
	bInUse = false;
	DestroyComponent();
}

bool URainbowDoorComponent::PickArrival(const APlayerCharacter* Player, FVector& OutLocation, FRotator& OutRotation) const
{
	const UWorld* World = GetWorld();
	const ADoor* Self = GetDoor();
	const UDreamGimmickSettings& Settings = UDreamGimmickSettings::Get();
	const UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);

	TArray<FVector> EnemyLocations;
	for (TActorIterator<AEnemyCharacter> It(World); It; ++It)
	{
		EnemyLocations.Add(It->GetActorLocation());
	}

	struct FCandidate
	{
		FVector Location;
		float Yaw;
		float NearestEnemy;
	};
	TArray<FCandidate> Candidates;
	auto Collect = [&](float MinDistance)
	{
	for (TActorIterator<ADoor> It(World); It; ++It)
	{
		const ADoor* Door = *It;
		if (Door == Self || Door->IsRainbow() || Door->IsInExitRoom())
		{
			continue;
		}
		const FVector DoorLocation = Door->GetActorLocation();
		if (Self && FVector::Dist2D(DoorLocation, Self->GetActorLocation()) < MinDistance)
		{
			continue;
		}
		float NearestEnemy = TNumericLimits<float>::Max();
		for (const FVector& Enemy : EnemyLocations)
		{
			NearestEnemy = FMath::Min(NearestEnemy, static_cast<float>(FVector::Dist2D(Enemy, DoorLocation)));
		}

		// 扉の両側のうち、ナビメッシュに乗る側に出る。向きは扉を背にする（扉の通り抜け方向はローカル Y）
		const FVector Through = Door->GetActorRightVector();
		for (const float Side : { 1.0f, -1.0f })
		{
			const FVector Outward = Through * Side;
			const FVector Probe = DoorLocation + Outward * Settings.ArrivalOffset;
			FVector Feet = Probe;
			if (NavSys)
			{
				FNavLocation Projected;
				if (!NavSys->ProjectPointToNavigation(Probe, Projected, FVector(60.0f, 60.0f, 200.0f)))
				{
					continue;
				}
				Feet = Projected.Location;
			}
			const float HalfHeight = Player->GetSimpleCollisionHalfHeight();
			Candidates.Add({ Feet + FVector(0.0f, 0.0f, HalfHeight + 2.0f), static_cast<float>(Outward.Rotation().Yaw), NearestEnemy });
		}
	}
	};
	Collect(Settings.MinTravelDistance);
	if (Candidates.IsEmpty())
	{
		// 小さい迷路で遠い扉が無いときは距離の条件を外す
		Collect(0.0f);
	}

	// ピエロから離れた候補から選ぶ。条件を満たす扉が無ければ、いちばん離れた候補にする
	TArray<const FCandidate*> Safe;
	const FCandidate* Farthest = nullptr;
	for (const FCandidate& Candidate : Candidates)
	{
		if (Candidate.NearestEnemy >= Settings.ArrivalEnemyExclusionRadius)
		{
			Safe.Add(&Candidate);
		}
		if (!Farthest || Candidate.NearestEnemy > Farthest->NearestEnemy)
		{
			Farthest = &Candidate;
		}
	}
	const FCandidate* Chosen = Safe.Num() > 0 ? Safe[FMath::RandRange(0, Safe.Num() - 1)] : Farthest;
	if (!Chosen)
	{
		return false;
	}
	OutLocation = Chosen->Location;
	OutRotation = FRotator(0.0f, Chosen->Yaw, 0.0f);
	return true;
}
