#include "Enemies/BehaviorTree/BTTask_BustHidingSpot.h"
#include "AIController.h"
#include "Engine/World.h"
#include "Interactables/HidingSpot.h"
#include "Interactables/HidingSpotSubsystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "Quietus.h"

UBTTask_BustHidingSpot::UBTTask_BustHidingSpot()
{
	NodeName = TEXT("Bust Hiding Spot");
	INIT_TASK_NODE_NOTIFY_FLAGS();
}

uint16 UBTTask_BustHidingSpot::GetInstanceMemorySize() const
{
	return sizeof(FBustMemory);
}

void UBTTask_BustHidingSpot::InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const
{
	InitializeNodeMemory<FBustMemory>(NodeMemory, InitType);
}

void UBTTask_BustHidingSpot::CleanupMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryClear::Type CleanupType) const
{
	CleanupNodeMemory<FBustMemory>(NodeMemory, CleanupType);
}

EBTNodeResult::Type UBTTask_BustHidingSpot::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIOwner = OwnerComp.GetAIOwner();
	const APawn* Pawn = AIOwner ? AIOwner->GetPawn() : nullptr;
	if (!Pawn)
	{
		return EBTNodeResult::Failed;
	}

	const UHidingSpotSubsystem* Registry = Pawn->GetWorld()->GetSubsystem<UHidingSpotSubsystem>();
	AActor* Spot = Registry ? Registry->FindOccupiedNear(Pawn->GetActorLocation(), SearchRadius) : nullptr;
	if (!Spot)
	{
		return EBTNodeResult::Succeeded;
	}

	UE_LOG(LogQuietus, Log, TEXT("%s: suspicious hiding spot nearby -> approaching"), *AIOwner->GetName());
	CastInstanceNodeMemory<FBustMemory>(NodeMemory)->Spot = Spot;
	AIOwner->MoveToActor(Spot, ApproachRadius);
	return EBTNodeResult::InProgress;
}

void UBTTask_BustHidingSpot::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AAIController* AIOwner = OwnerComp.GetAIOwner();
	const APawn* Pawn = AIOwner ? AIOwner->GetPawn() : nullptr;
	FBustMemory* Memory = CastInstanceNodeMemory<FBustMemory>(NodeMemory);
	AActor* Spot = Memory->Spot.Get();
	if (!Pawn || !Spot)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	if (FVector::Dist2D(Spot->GetActorLocation(), Pawn->GetActorLocation()) <= ReachDistance)
	{
		UE_LOG(LogQuietus, Log, TEXT("%s: busting hiding spot open"), *AIOwner->GetName());
		if (IHidingSpot* HidingSpot = Cast<IHidingSpot>(Spot))
		{
			HidingSpot->BustOpen();
		}
		Memory->Spot.Reset();
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	// 届かないまま移動が止まった（経路なし等）なら諦めて捜索を続ける
	if (AIOwner->GetMoveStatus() == EPathFollowingStatus::Idle)
	{
		Memory->Spot.Reset();
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

EBTNodeResult::Type UBTTask_BustHidingSpot::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	CastInstanceNodeMemory<FBustMemory>(NodeMemory)->Spot.Reset();
	if (AAIController* AIOwner = OwnerComp.GetAIOwner())
	{
		AIOwner->StopMovement();
	}
	return EBTNodeResult::Aborted;
}

FString UBTTask_BustHidingSpot::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s: within %.0f cm"), *Super::GetStaticDescription(), SearchRadius);
}
