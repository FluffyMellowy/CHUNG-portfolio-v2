#include "Player/KillCamComponent.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "Enemies/EnemyCharacter.h"
#include "Enemies/KillSequence.h"
#include "GameFramework/PlayerController.h"
#include "Player/PlayerCharacter.h"
#include "TimerManager.h"

UKillCamComponent::UKillCamComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	// 敵の位置が決まったあとで視点を置く
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

bool UKillCamComponent::Play(AEnemyCharacter* InKiller, APlayerCharacter* Victim, const FVector& FromDirection)
{
	UCameraComponent* Cam = Victim ? Victim->GetFirstPersonCameraComponent() : nullptr;
	if (IsPlaying() || !InKiller || !Cam || !InKiller->StartKill(Victim, Cam->GetComponentLocation(), FromDirection))
	{
		return false;
	}

	Sequence = InKiller->GetKillSequence();
	Killer = InKiller;
	Camera = Cam;
	Elapsed = 0.0f;
	bImpact = false;
	StartView = Cam->GetComponentTransform();

	// 視点を体（一人称の腕の head ソケット）から外して直接動かす。腕と体は立ったまま残ってしまうので隠す
	// （体は自分には見えないが、倒れた視点の懐中電灯に照らされて影が出る）
	Cam->bUsePawnControlRotation = false;
	Cam->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	if (USkeletalMeshComponent* Arms = Victim->GetFirstPersonMesh())
	{
		Arms->SetVisibility(false);
	}
	Victim->GetMesh()->SetVisibility(false);

	SetComponentTickEnabled(true);
	UpdateView();
	return true;
}

void UKillCamComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	Elapsed += DeltaTime;
	UpdateView();

	// 暗転の時刻もアニメの再生位置で見る
	const AEnemyCharacter* MyKiller = Killer.Get();
	if (!bImpact && Sequence && MyKiller && MyKiller->GetMesh()->GetPosition() >= Sequence->ImpactTime)
	{
		bImpact = true;
		const APlayerController* PC = Cast<APlayerController>(GetOwner());
		if (PC && PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StartCameraFade(0.0f, 1.0f, Sequence->BlackoutTime, FLinearColor::Black, false, /*bHoldWhenFinished=*/ true);
		}
		GetWorld()->GetTimerManager().SetTimer(BlackoutTimer, this, &UKillCamComponent::FinishBlackout, FMath::Max(Sequence->BlackoutTime, 0.01f), false);
	}
}

void UKillCamComponent::UpdateView()
{
	UCameraComponent* Cam = Camera.Get();
	const AEnemyCharacter* MyKiller = Killer.Get();
	if (!Cam || !MyKiller || !Sequence)
	{
		SetComponentTickEnabled(false);
		return;
	}

	// 視点はアニメの再生位置に合わせる（ティックの順番でアニメと 1 フレームずれないように）
	const USkeletalMeshComponent* Mesh = MyKiller->GetMesh();
	const FTransform Key = Sequence->SampleCamera(Mesh->GetPosition()) * Mesh->GetComponentTransform();
	// 捕まった瞬間の視点から軌跡へなめらかに寄せる
	const float Alpha = Sequence->CameraBlendTime > 0.0f ? FMath::SmoothStep(0.0f, 1.0f, Elapsed / Sequence->CameraBlendTime) : 1.0f;
	Cam->SetWorldLocationAndRotation(
		FMath::Lerp(StartView.GetLocation(), Key.GetLocation(), Alpha),
		FQuat::Slerp(StartView.GetRotation(), Key.GetRotation(), Alpha));
}

void UKillCamComponent::FinishBlackout()
{
	// 真っ暗になったので視点はもう動かさなくてよい
	SetComponentTickEnabled(false);
	OnBlackout.ExecuteIfBound();
}
