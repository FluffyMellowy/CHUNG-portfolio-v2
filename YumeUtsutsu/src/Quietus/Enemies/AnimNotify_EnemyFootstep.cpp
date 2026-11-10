#include "Enemies/AnimNotify_EnemyFootstep.h"
#include "Components/SkeletalMeshComponent.h"
#include "Enemies/EnemyCharacter.h"

void UAnimNotify_EnemyFootstep::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (AEnemyCharacter* Enemy = MeshComp ? Cast<AEnemyCharacter>(MeshComp->GetOwner()) : nullptr)
	{
		Enemy->PlayFootstep();
	}
}
