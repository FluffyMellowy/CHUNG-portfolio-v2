#include "FlashlightComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Core/DreamFeedbackSettings.h"
#include "Core/DreamLightingSettings.h"
#include "Feedback/FeedbackSubsystem.h"

UFlashlightComponent::UFlashlightComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	SetMobility(EComponentMobility::Movable);

	// 初期状態は消灯。ライトはVisibilityで点け消しするのが最も簡単
	SetVisibility(false);
}

void UFlashlightComponent::BeginPlay()
{
	Super::BeginPlay();

	// BP側で調整された値を反映するため、初期化はコンストラクタではなくここで行う
	Battery = MaxBattery;

	BobBaseOffset = GetRelativeLocation();
	BobBaseRotation = GetRelativeRotation();

	if (bStartOn)
	{
		bIsOn = true;
		SetVisibility(true);
	}
}

void UFlashlightComponent::ApplyLightingSettings()
{
	const UDreamLightingSettings& S = UDreamLightingSettings::Get();
	SetIntensityUnits(ELightUnits::Unitless);
	SetUseInverseSquaredFalloff(S.bFlashlightInverseSquare);
	SetLightFalloffExponent(S.FlashlightFalloffExponent);
	SetIntensity(S.FlashlightIntensity);
	SetAttenuationRadius(S.FlashlightRadius);
	SetInnerConeAngle(S.FlashlightInnerCone);
	SetOuterConeAngle(S.FlashlightOuterCone);
	SetLightColor(S.FlashlightColor);
	SetIndirectLightingIntensity(S.FlashlightIndirect);
	SetSpecularScale(S.FlashlightSpecular);
	SetCastShadows(S.bFlashlightShadows);
}

void UFlashlightComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateBob(DeltaTime);
	UpdateBattery(DeltaTime);
}

void UFlashlightComponent::UpdateBattery(float DeltaTime)
{
	if (!bIsOn)
	{
		return;
	}

	Battery = FMath::Max(Battery - BatteryDrainRate * DeltaTime, 0.0f);

	// 枯渇で強制消灯（バッテリー回復はアイテム側で対応予定）
	if (Battery <= 0.0f)
	{
		bIsOn = false;
		SetVisibility(false);
	}
}

void UFlashlightComponent::Toggle()
{
	// バッテリー切れの場合は点灯不可（消灯は常に可能）
	if (!bIsOn && Battery <= 0.0f)
	{
		return;
	}

	bIsOn = !bIsOn;
	SetVisibility(bIsOn);

	const UDreamFeedbackSettings& Sfx = UDreamFeedbackSettings::Get();
	UFeedbackSubsystem::Play2D(this, bIsOn ? Sfx.FlashlightOn : Sfx.FlashlightOff, bIsOn ? TEXT("FlashlightOn") : TEXT("FlashlightOff"));
}

void UFlashlightComponent::Recharge(float Amount)
{
	Battery = FMath::Clamp(Battery + Amount, 0.0f, MaxBattery);
}

void UFlashlightComponent::UpdateBob(float DeltaTime)
{
	const ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter)
	{
		return;
	}

	// 接地して動いている間だけ揺らす。振幅はエンベロープで滑らかに立ち上げ/減衰させる
	const float Speed = OwnerCharacter->GetVelocity().Size2D();
	const bool bMoving = Speed > 10.0f && OwnerCharacter->GetCharacterMovement()->IsMovingOnGround();

	const float SpeedRatio = FMath::Clamp(Speed / BobReferenceSpeed, 0.0f, 1.0f);
	const float TargetEnvelope = bMoving ? SpeedRatio : 0.0f;
	BobEnvelope = FMath::FInterpTo(BobEnvelope, TargetEnvelope, DeltaTime, BobBlendSpeed);

	const float Frequency = FMath::Lerp(BobWalkFrequency, BobSprintFrequency, SpeedRatio);
	BobPhase += DeltaTime * Frequency * 2.0f * PI;

	// 縦を主、横を弱く（歩行の8の字っぽい揺れ）。縦は1歩ごとに2回沈む感覚で2倍周期。
	// 平行移動だけではスポットライトのビームがほとんど動かないため、首振り（回転）を主とする
	const float VerticalOffset = FMath::Sin(BobPhase * 2.0f) * BobAmplitude * BobEnvelope;
	const float LateralOffset = FMath::Sin(BobPhase) * BobAmplitude * 0.35f * BobEnvelope;
	const float PitchSwing = FMath::Sin(BobPhase * 2.0f) * BobPitchAmplitude * BobEnvelope;
	const float YawSwing = FMath::Sin(BobPhase) * BobYawAmplitude * BobEnvelope;

	SetRelativeLocation(BobBaseOffset + FVector(0.0f, LateralOffset, VerticalOffset));
	SetRelativeRotation(BobBaseRotation + FRotator(PitchSwing, YawSwing, 0.0f));
}
