#include "Player/DoorWarpComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Core/DreamFeedbackSettings.h"
#include "Core/DreamGimmicks.h"
#include "Core/RunGameState.h"
#include "Enemies/EnemyCharacter.h"
#include "EngineUtils.h"
#include "Feedback/FeedbackSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "Interactables/Door.h"
#include "NavigationSystem.h"
#include "Player/PlayerCharacter.h"
#include "Quietus.h"
#include "TimerManager.h"

UDoorWarpComponent::UDoorWarpComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

APlayerCharacter* UDoorWarpComponent::GetPlayer() const
{
	return Cast<APlayerCharacter>(GetOwner());
}

void UDoorWarpComponent::SetInputBlocked(bool bBlocked) const
{
	const APlayerCharacter* Player = GetPlayer();
	if (APlayerController* PC = Player ? Cast<APlayerController>(Player->GetController()) : nullptr)
	{
		PC->SetIgnoreMoveInput(bBlocked);
		PC->SetIgnoreLookInput(bBlocked);
	}
}

bool UDoorWarpComponent::Begin(ADoor* From)
{
	APlayerCharacter* Player = GetPlayer();
	if (bWarping || !From || !Player)
	{
		return false;
	}
	bWarping = true;
	Departure = From;

	// 鍵を回して扉を開けた音。操作を止めて画面を光で満たす
	const UDreamFeedbackSettings& Sfx = UDreamFeedbackSettings::Get();
	UFeedbackSubsystem::PlayAt(this, Sfx.DoorOpen, Sfx.InteractAttenuation, From->GetActorLocation() + FVector(0.0f, 0.0f, 100.0f), TEXT("DoorOpen"));
	SetInputBlocked(true);
	const float Whiteout = UDreamGimmickSettings::Get().WhiteoutTime;
	if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
	{
		if (PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StartCameraFade(0.0f, 1.0f, Whiteout, FLinearColor::White, false, /*bHoldWhenFinished=*/ true);
		}
	}
	GetWorld()->GetTimerManager().SetTimer(StepTimer, this, &UDoorWarpComponent::Teleport, Whiteout + 0.05f, false);
	return true;
}

void UDoorWarpComponent::Teleport()
{
	APlayerCharacter* Player = GetPlayer();
	const ADoor* From = Departure.Get();
	if (!Player || !From)
	{
		SetInputBlocked(false);
		bWarping = false;
		return;
	}

	FVector Location;
	FRotator Rotation;
	if (PickArrival(From, Location, Rotation))
	{
		Player->TeleportTo(Location, Rotation);
		if (AController* Controller = Player->GetController())
		{
			Controller->SetControlRotation(Rotation);
		}
		UE_LOG(LogQuietus, Log, TEXT("Key warp: %s -> %s"), *From->GetName(), *Location.ToCompactString());

		// 追っていたピエロはここで見失い、出発した扉のあたりを探す
		if (ARunGameState* GameState = ARunGameState::Get(this))
		{
			GameState->NotifyPlayerTeleported(From->GetActorLocation());
		}
	}
	else
	{
		UE_LOG(LogQuietus, Warning, TEXT("Key warp: no arrival door found from %s, staying"), *From->GetName());
	}

	if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
	{
		if (PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StartCameraFade(1.0f, 0.0f, UDreamGimmickSettings::Get().RevealTime, FLinearColor::White);
		}
	}
	GetWorld()->GetTimerManager().SetTimer(StepTimer, this, &UDoorWarpComponent::FinishReveal,
		UDreamGimmickSettings::Get().RevealTime * 0.3f, false);
}

void UDoorWarpComponent::FinishReveal()
{
	// 光が引き始めたら操作を返す
	SetInputBlocked(false);
	bWarping = false;
	Departure.Reset();
}

bool UDoorWarpComponent::PickArrival(const ADoor* From, FVector& OutLocation, FRotator& OutRotation) const
{
	const UWorld* World = GetWorld();
	const APlayerCharacter* Player = GetPlayer();
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
			if (Door == From || Door->IsInExitRoom())
			{
				continue;
			}
			const FVector DoorLocation = Door->GetActorLocation();
			if (FVector::Dist2D(DoorLocation, From->GetActorLocation()) < MinDistance)
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
				Candidates.Add({ Feet + FVector(0.0f, 0.0f, Player->GetSimpleCollisionHalfHeight() + 2.0f),
					static_cast<float>(Outward.Rotation().Yaw), NearestEnemy });
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
