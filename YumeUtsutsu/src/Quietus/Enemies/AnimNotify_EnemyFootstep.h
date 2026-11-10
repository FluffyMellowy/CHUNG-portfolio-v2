#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_EnemyFootstep.generated.h"

/** 敵の歩行・走行アニメで足が床に着く瞬間に置く。持ち主の敵の足音を鳴らす */
UCLASS(meta = (DisplayName = "Enemy Footstep"))
class UAnimNotify_EnemyFootstep : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override { return TEXT("EnemyFootstep"); }
};
