#include "Throwable.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Player/PlayerCharacter.h"
#include "Core/DreamFeedbackSettings.h"
#include "Feedback/FeedbackSubsystem.h"
#include "Quietus.h"

#define LOCTEXT_NAMESPACE "Interaction"

AThrowable::AThrowable()
{
	// 本体メッシュ（基底が生成）は投擲中の衝突判定も受ける
	GetMesh()->SetCollisionProfileName(FName("PhysicsActor"));
	GetMesh()->SetNotifyRigidBodyCollision(true);

	// 弾道は投げた瞬間だけ動かす
	Projectile = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Projectile"));
	Projectile->bAutoActivate = false;
	Projectile->ProjectileGravityScale = 1.0f;
	Projectile->bShouldBounce = true;
	Projectile->Bounciness = 0.25f;
	Projectile->Friction = 0.6f;
	Projectile->BounceVelocityStopSimulatingThreshold = 50.0f;
}

void AThrowable::BeginPlay()
{
	Super::BeginPlay();
	GetMesh()->OnComponentHit.AddDynamic(this, &AThrowable::OnMeshHit);
}

void AThrowable::Launch(APawn* Thrower, const FVector& Velocity)
{
	ThrownBy = Thrower;
	bInFlight = true;
	bNoiseDone = false;

	// 投げた本人と即衝突しないようにする
	if (Thrower)
	{
		GetMesh()->IgnoreActorWhenMoving(Thrower, true);
	}
	Projectile->Velocity = Velocity;
	Projectile->Activate(true);
}

void AThrowable::OnMeshHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	if (!bInFlight)
	{
		return;
	}

	// 最初の着弾で1回だけノイズを出す（敵AIの聴覚に届く）
	if (!bNoiseDone && ThrownBy)
	{
		bNoiseDone = true;
		ThrownBy->MakeNoise(ImpactLoudness, ThrownBy, Hit.ImpactPoint);
		const UDreamFeedbackSettings& Sfx = UDreamFeedbackSettings::Get();
		UFeedbackSubsystem::PlayAt(this, Sfx.ThrowableImpact, Sfx.ImpactAttenuation, Hit.ImpactPoint, TEXT("ThrowableImpact"));
		UE_LOG(LogQuietus, Log, TEXT("%s: impact noise at (%.0f, %.0f, %.0f)"),
			*GetName(), Hit.ImpactPoint.X, Hit.ImpactPoint.Y, Hit.ImpactPoint.Z);
	}

	// 転がりが収まったらまた拾える状態に戻す
	if (Projectile->Velocity.SizeSquared() < FMath::Square(100.0f))
	{
		bInFlight = false;
		Projectile->Deactivate();
		if (ThrownBy)
		{
			GetMesh()->IgnoreActorWhenMoving(ThrownBy, false);
		}
	}
}

bool AThrowable::CanBePickedUp(const APlayerCharacter* Interactor) const
{
	return !bInFlight && Interactor;
}

void AThrowable::OnPickedUp(APlayerCharacter* Interactor)
{
	Interactor->AddThrowable(GetClass());
	UFeedbackSubsystem::Play2D(this, UDreamFeedbackSettings::Get().PickupGeneric, TEXT("PickupGeneric"));
}

FText AThrowable::GetInteractionPrompt_Implementation() const
{
	return bInFlight ? FText::GetEmpty() : LOCTEXT("PickUpThrowable", "瓶を拾う");
}

#undef LOCTEXT_NAMESPACE
