#include "Enemies/BehaviorTree/BTTask_UpdatePatrolPoint.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "Enemies/EnemyController.h"

UBTTask_UpdatePatrolPoint::UBTTask_UpdatePatrolPoint()
{
	NodeName = TEXT("Update Patrol Point");
	PatrolPointKey.SelectedKeyName = EnemyKeys::PatrolPoint;
	PatrolPointKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_UpdatePatrolPoint, PatrolPointKey), AActor::StaticClass());
}

void UBTTask_UpdatePatrolPoint::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);
	if (const UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		PatrolPointKey.ResolveSelectedKey(*BBAsset);
	}
}

uint16 UBTTask_UpdatePatrolPoint::GetInstanceMemorySize() const
{
	return sizeof(FPatrolMemory);
}

void UBTTask_UpdatePatrolPoint::InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const
{
	InitializeNodeMemory<FPatrolMemory>(NodeMemory, InitType);
}

void UBTTask_UpdatePatrolPoint::CleanupMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryClear::Type CleanupType) const
{
	CleanupNodeMemory<FPatrolMemory>(NodeMemory, CleanupType);
}

EBTNodeResult::Type UBTTask_UpdatePatrolPoint::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	const AEnemyController* Controller = Cast<AEnemyController>(OwnerComp.GetAIOwner());
	const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!Pawn || !BB)
	{
		return EBTNodeResult::Failed;
	}

	const TArray<TObjectPtr<AActor>>& Points = Controller->GetPatrolPoints();
	if (Points.Num() == 0)
	{
		return EBTNodeResult::Failed;
	}

	FPatrolMemory* Memory = CastInstanceNodeMemory<FPatrolMemory>(NodeMemory);
	Memory->Index %= Points.Num();

	const AActor* Current = Points[Memory->Index];
	if (Current && FVector::Dist2D(Current->GetActorLocation(), Pawn->GetActorLocation()) <= ArrivedRadius)
	{
		Memory->Index = (Memory->Index + 1) % Points.Num();
	}

	BB->SetValueAsObject(PatrolPointKey.SelectedKeyName, Points[Memory->Index]);
	return EBTNodeResult::Succeeded;
}

FString UBTTask_UpdatePatrolPoint::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s -> %s"), *Super::GetStaticDescription(), *PatrolPointKey.SelectedKeyName.ToString());
}
