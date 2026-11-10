#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraShakeBase.h"
#include "FeedbackCameraShakes.generated.h"

/** 追跡が始まった瞬間の短い揺れ（約 0.4 秒） */
UCLASS()
class UChaseStartCameraShake : public UCameraShakeBase
{
	GENERATED_BODY()

public:
	UChaseStartCameraShake(const FObjectInitializer& ObjectInitializer);
};

/** 近くのピエロの足音に合わせたごく弱い揺れ */
UCLASS()
class UFootstepCameraShake : public UCameraShakeBase
{
	GENERATED_BODY()

public:
	UFootstepCameraShake(const FObjectInitializer& ObjectInitializer);
};
