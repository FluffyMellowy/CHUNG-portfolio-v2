#include "Feedback/FeedbackCameraShakes.h"
#include "Shakes/PerlinNoiseCameraShakePattern.h"

namespace
{
	/** 回転と上下だけを揺らす。左右移動と FOV は揺らさない（酔いやすいので） */
	UPerlinNoiseCameraShakePattern* MakePattern(UCameraShakeBase* Shake, float Duration, float PitchYaw, float Roll, float Bob, float Frequency)
	{
		UPerlinNoiseCameraShakePattern* Pattern = Shake->CreateDefaultSubobject<UPerlinNoiseCameraShakePattern>(TEXT("Pattern"));
		Pattern->Duration = Duration;
		Pattern->BlendInTime = FMath::Min(0.05f, Duration * 0.25f);
		Pattern->BlendOutTime = Duration * 0.5f;
		Pattern->X.Amplitude = 0.0f;
		Pattern->Y.Amplitude = 0.0f;
		Pattern->Z.Amplitude = Bob;
		Pattern->Z.Frequency = Frequency;
		Pattern->Pitch.Amplitude = PitchYaw;
		Pattern->Pitch.Frequency = Frequency;
		Pattern->Yaw.Amplitude = PitchYaw;
		Pattern->Yaw.Frequency = Frequency * 0.8f;
		Pattern->Roll.Amplitude = Roll;
		Pattern->Roll.Frequency = Frequency * 0.6f;
		Pattern->FOV.Amplitude = 0.0f;
		return Pattern;
	}
}

UChaseStartCameraShake::UChaseStartCameraShake(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bSingleInstance = true;
	SetRootShakePattern(MakePattern(this, 0.4f, 0.6f, 0.3f, 0.4f, 14.0f));
}

UFootstepCameraShake::UFootstepCameraShake(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetRootShakePattern(MakePattern(this, 0.18f, 0.12f, 0.0f, 0.35f, 18.0f));
}
