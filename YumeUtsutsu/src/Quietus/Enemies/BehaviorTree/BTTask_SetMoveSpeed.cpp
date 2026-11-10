#include "Enemies/BehaviorTree/BTTask_SetMoveSpeed.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Core/RunGameInstance.h"

UBTTask_SetMoveSpeed::UBTTask_SetMoveSpeed()
{
	NodeName = TEXT("Set Move Speed");
}

EBTNodeResult::Type UBTTask_SetMoveSpeed::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	const AAIController* AIOwner = OwnerComp.GetAIOwner();
	const ACharacter* Character = AIOwner ? Cast<ACharacter>(AIOwner->GetPawn()) : nullptr;
	if (!Character)
	{
		return EBTNodeResult::Failed;
	}
	// 難易度の倍率を掛ける（ノードの Speed は 悪夢 の値）
	Character->GetCharacterMovement()->MaxWalkSpeed = Speed * URunGameInstance::GetParams(Character).EnemySpeedScale;
	return EBTNodeResult::Succeeded;
}

FString UBTTask_SetMoveSpeed::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s: %.0f cm/s"), *Super::GetStaticDescription(), Speed);
}
