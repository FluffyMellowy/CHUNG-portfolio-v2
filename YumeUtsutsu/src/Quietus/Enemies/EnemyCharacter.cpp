#include "Enemies/EnemyCharacter.h"
#include "Quietus.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Enemies/EnemyController.h"
#include "Enemies/KillSequence.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "Animation/AnimSequence.h"
#include "Core/DreamFeedbackSettings.h"
#include "Feedback/FeedbackSubsystem.h"

AEnemyCharacter::AEnemyCharacter()
{
	// 判断はコントローラ、アニメーションは AnimBP が更新するので自前の Tick は不要
	PrimaryActorTick.bCanEverTick = false;

	// 配置・スポーンどちらでも自動的にAIコントローラが憑依する
	AIControllerClass = AEnemyController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// 進行方向を向いて歩く（コントローラの回転には追従しない）
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 240.0f, 0.0f);

	// 共有スケルトンのメッシュは足元が原点・+Y が正面なので、カプセルの底へ下げて正面を +X に向ける
	GetMesh()->SetRelativeLocationAndRotation(
		FVector(0.0f, 0.0f, -GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()),
		FRotator(0.0f, -90.0f, 0.0f));
	GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);

	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(GetMesh(), WeaponSocket);
	WeaponMesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	WeaponMesh->SetGenerateOverlapEvents(false);
	WeaponMesh->SetCanEverAffectNavigation(false);
	WeaponMesh->CanCharacterStepUpOn = ECB_No;
	WeaponMesh->bReceivesDecals = false;
}

void AEnemyCharacter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// BP でソケット名を変えた場合も付け直す。ずれはソケット側で持つので武器自身はオフセットなし
	if (WeaponMesh && WeaponMesh->GetAttachSocketName() != WeaponSocket)
	{
		WeaponMesh->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponSocket);
	}
}

void AEnemyCharacter::PlayFootstep()
{
	if (Footsteps == EEnemyFootsteps::None)
	{
		return;
	}
	const UDreamFeedbackSettings& S = UDreamFeedbackSettings::Get();
	const FVector Feet = GetActorLocation() - FVector(0.0f, 0.0f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	UFeedbackSubsystem::PlayAt(this, S.ClownStep, S.ClownStepAttenuation, Feet, TEXT("ClownStep"));
	if (Footsteps == EEnemyFootsteps::ClownBells)
	{
		UFeedbackSubsystem::PlayAt(this, S.ClownBell, S.ClownStepAttenuation, Feet, TEXT("ClownBell"));
		if (FMath::FRand() < S.ClownSqueakChance)
		{
			UFeedbackSubsystem::PlayAt(this, S.ClownSqueak, S.ClownStepAttenuation, Feet, TEXT("ClownSqueak"));
		}
	}
	if (UFeedbackSubsystem* Feedback = UFeedbackSubsystem::Get(this))
	{
		Feedback->NotifyEnemyFootstep(Feet);
	}
}

bool AEnemyCharacter::HasWeapon() const
{
	return WeaponMesh && WeaponMesh->GetStaticMesh() != nullptr;
}

bool AEnemyCharacter::StartKill(const AActor* Victim, const FVector& VictimEye, const FVector& FromDirection)
{
	if (!KillSequence || !KillSequence->IsPlayable())
	{
		return false;
	}

	// 判断と移動を止める（コントローラも捕獲の通知で止まるが、通知の順番に頼らない）
	if (AAIController* AI = Cast<AAIController>(GetController()))
	{
		AI->StopMovement();
		AI->ClearFocus(EAIFocusPriority::Gameplay);
		if (AI->BrainComponent)
		{
			AI->BrainComponent->StopLogic(TEXT("Kill"));
		}
	}
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();

	// 相手の方を向き、軌跡の最初のキー（演出の視点）が水平で相手の目と重なる位置へ立つ
	FVector ToVictim = -FromDirection.GetSafeNormal2D();
	if (ToVictim.IsNearlyZero())
	{
		ToVictim = (VictimEye - GetActorLocation()).GetSafeNormal2D();
	}
	if (ToVictim.IsNearlyZero())
	{
		ToVictim = GetActorForwardVector();
	}
	const float BaseYaw = ToVictim.Rotation().Yaw;
	// 暗転までの視点の通り道（自分の空間）。間引いて壁に触れないかを見る
	const FTransform MeshOffset = GetMesh()->GetRelativeTransform();
	const FVector FirstView = MeshOffset.TransformPosition(KillSequence->CameraTrack[0].GetLocation());
	TArray<FVector> ViewPath;
	const int32 LastKey = FMath::Min(FMath::CeilToInt32(KillSequence->ImpactTime * KillSequence->FrameRate), KillSequence->CameraTrack.Num() - 1);
	for (int32 Key = 0; Key <= LastKey; Key = (Key == LastKey) ? LastKey + 1 : FMath::Min(Key + 6, LastKey))
	{
		ViewPath.Add(MeshOffset.TransformPosition(KillSequence->CameraTrack[Key].GetLocation()));
	}

	// 前が狭い（ロッカーの前、壁際で捕まった等）と立ち位置が壁の中や壁の向こうになる。
	// そのときは自分と演出の視点を組のまま相手の後ろへ下げる（最大 120cm。ロッカーなら中へ倒れ込む形。視点の出だしのずれはカメラ側で寄せる）か、
	// 相手の周りを 20 度ずつ回って空きを探す。
	// bStrict でなければ、床があって体が壁に重ならないことだけを見る（視線や倒れ込む先は諦める）。@return だめな理由（よければ null）
	FCollisionQueryParams Params(SCENE_QUERY_STAT(EnemyKillPlacement), false, this);
	Params.AddIgnoredActor(Victim);
	const float HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const FCollisionShape Body = FCollisionShape::MakeCapsule(GetCapsuleComponent()->GetScaledCapsuleRadius(), HalfHeight - 5.0f);
	// 視点の周りに要る空き（ニアクリップで壁の裏が見えないように）
	const FCollisionShape ViewRoom = FCollisionShape::MakeSphere(20.0f);
	const float StandHeight = HalfHeight + GetCharacterMovement()->MaxStepHeight * 0.1f;
	auto Check = [&](float Yaw, float PullBack, bool bStrict, FVector& OutSpot) -> const TCHAR*
	{
		const FRotator Facing(0.0f, Yaw, 0.0f);
		OutSpot = VictimEye + Facing.Vector() * PullBack - Facing.RotateVector(FVector(FirstView.X, FirstView.Y, 0.0f));
		OutSpot.Z = GetActorLocation().Z;

		// 高さは立つ場所の床に合わせる（床が無ければ壁の向こうか迷路の外）
		FHitResult Floor;
		if (!GetWorld()->LineTraceSingleByChannel(Floor, OutSpot + FVector(0.0f, 0.0f, 60.0f), OutSpot - FVector(0.0f, 0.0f, HalfHeight + 60.0f), ECC_Visibility, Params))
		{
			return TEXT("no floor");
		}
		OutSpot.Z = Floor.ImpactPoint.Z + StandHeight;
		if (GetWorld()->OverlapBlockingTestByChannel(OutSpot + FVector(0.0f, 0.0f, 5.0f), FQuat::Identity, ECC_Pawn, Body, Params))
		{
			return TEXT("body");
		}
		if (!bStrict)
		{
			return nullptr;
		}
		const FVector Start = OutSpot + Facing.RotateVector(FirstView);
		if (GetWorld()->LineTraceTestByChannel(OutSpot + FVector(0.0f, 0.0f, HalfHeight * 0.6f), Start, ECC_Visibility, Params))
		{
			return TEXT("sight");
		}
		if (PullBack > 0.0f && GetWorld()->LineTraceTestByChannel(VictimEye, Start, ECC_Visibility, Params))
		{
			return TEXT("pull back");
		}
		for (int32 i = 0; i + 1 < ViewPath.Num(); ++i)
		{
			if (GetWorld()->SweepTestByChannel(OutSpot + Facing.RotateVector(ViewPath[i]), OutSpot + Facing.RotateVector(ViewPath[i + 1]),
				FQuat::Identity, ECC_Visibility, ViewRoom, Params))
			{
				return TEXT("fall");
			}
		}
		return nullptr;
	};

	float Yaw = BaseYaw;
	float PullBack = 0.0f;
	FVector Spot = GetActorLocation();
	const TCHAR* Result = TEXT("no free spot, stayed in place");
	bool bFound = false;
	for (const bool bStrict : { true, false })
	{
		for (int32 i = 0; i < 18 && !bFound; ++i)
		{
			// 0, -20, +20, -40, ... ±180。同じ向きなら下げ幅の小さい方から
			const float CandidateYaw = BaseYaw + (i % 2 == 0 ? 1.0f : -1.0f) * 20.0f * ((i + 1) / 2);
			for (const float Back : { 0.0f, 30.0f, 60.0f, 90.0f, 120.0f })
			{
				FVector CandidateSpot;
				const TCHAR* Reason = Check(CandidateYaw, Back, bStrict, CandidateSpot);
				UE_LOG(LogQuietus, Verbose, TEXT("  kill placement %+.0f deg, back %.0f%s: %s"), CandidateYaw - BaseYaw, Back,
					bStrict ? TEXT("") : TEXT(" (relaxed)"), Reason ? Reason : TEXT("ok"));
				if (!Reason)
				{
					Yaw = CandidateYaw;
					PullBack = Back;
					Spot = CandidateSpot;
					Result = bStrict ? TEXT("ok") : TEXT("relaxed, the view may be blocked");
					bFound = true;
					break;
				}
			}
		}
		if (bFound)
		{
			break;
		}
	}
	// どこにも立てなければ、今の場所（歩いて来られた場所）のまま相手の方を向くだけにする
	UE_LOG(LogQuietus, Log, TEXT("%s: kill placement %+.0f deg, back %.0f cm (%s)"), *GetName(), Yaw - BaseYaw, PullBack, Result);

	const FRotator Facing(0.0f, Yaw, 0.0f);
	SetActorLocationAndRotation(Spot, Facing, false, nullptr, ETeleportType::TeleportPhysics);
	if (AController* MyController = GetController())
	{
		MyController->SetControlRotation(Facing);
	}

	// AnimBP を外して演出のアニメだけを流す（終わったらレベルを開き直すので戻さない）
	GetMesh()->PlayAnimation(KillSequence->Animation, false);
	return true;
}
