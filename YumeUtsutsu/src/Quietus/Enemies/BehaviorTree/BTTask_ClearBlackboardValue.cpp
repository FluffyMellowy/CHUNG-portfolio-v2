#include "Enemies/BehaviorTree/BTTask_ClearBlackboardValue.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_ClearBlackboardValue::UBTTask_ClearBlackboardValue()
{
	NodeName = TEXT("Clear Blackboard Value");
}

EBTNodeResult::Type UBTTask_ClearBlackboardValue::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB)
	{
		return EBTNodeResult::Failed;
	}
	BB->ClearValue(GetSelectedBlackboardKey());
	return EBTNodeResult::Succeeded;
}

FString UBTTask_ClearBlackboardValue::GetStaticDescription() const
{
	return FString::Printf(TEXT("Clear: %s"), *GetSelectedBlackboardKey().ToString());
}
