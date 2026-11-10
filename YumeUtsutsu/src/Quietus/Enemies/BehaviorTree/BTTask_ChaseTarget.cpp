#include "Enemies/BehaviorTree/BTTask_ChaseTarget.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "Core/RunGameState.h"
#include "Enemies/EnemyController.h"
#include "Player/PlayerCharacter.h"
#include "Quietus.h"

UBTTask_ChaseTarget::UBTTask_ChaseTarget()
{
	NodeName = TEXT("Chase Target");
	INIT_TASK_NODE_NOTIFY_FLAGS();

	TargetKey.SelectedKeyName = EnemyKeys::TargetActor;
	TargetKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_ChaseTarget, TargetKey), AActor::StaticClass());
	LastKnownLocationKey.SelectedKeyName = EnemyKeys::LastKnownLocation;
	LastKnownLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_ChaseTarget, LastKnownLocationKey));
	SeeingTargetKey.SelectedKeyName = EnemyKeys::SeeingTarget;
	SeeingTargetKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_ChaseTarget, SeeingTargetKey));
}

void UBTTask_ChaseTarget::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);
	if (const UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		TargetKey.ResolveSelectedKey(*BBAsset);
		LastKnownLocationKey.ResolveSelectedKey(*BBAsset);
		SeeingTargetKey.ResolveSelectedKey(*BBAsset);
	}
}

uint16 UBTTask_ChaseTarget::GetInstanceMemorySize() const
{
	return sizeof(FChaseMemory);
}

void UBTTask_ChaseTarget::InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const
{
	InitializeNodeMemory<FChaseMemory>(NodeMemory, InitType);
}

void UBTTask_ChaseTarget::CleanupMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryClear::Type CleanupType) const
{
	CleanupNodeMemory<FChaseMemory>(NodeMemory, CleanupType);
}

EBTNodeResult::Type UBTTask_ChaseTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	const UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!OwnerComp.GetAIOwner() || !BB || !BB->GetValueAsObject(TargetKey.SelectedKeyName))
	{
		return EBTNodeResult::Failed;
	}

	FChaseMemory* Memory = CastInstanceNodeMemory<FChaseMemory>(NodeMemory);
	*Memory = FChaseMemory();

	// 初回は即座に経路を引く（以後は RepathInterval 間隔）
	Repath(OwnerComp);
	return EBTNodeResult::InProgress;
}

void UBTTask_ChaseTarget::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AAIController* AIOwner = OwnerComp.GetAIOwner();
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	const AActor* Target = BB ? Cast<AActor>(BB->GetValueAsObject(TargetKey.SelectedKeyName)) : nullptr;
	const APawn* Pawn = AIOwner ? AIOwner->GetPawn() : nullptr;
	if (!Target || !Pawn)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// 捕獲判定。ロッカー内のプレイヤーは壁越し距離では捕まえない（こじ開けだけが捕獲手段）
	const APlayerCharacter* Player = Cast<APlayerCharacter>(Target);
	const bool bHidden = Player && Player->IsHiddenInLocker();
	if (!bHidden && FVector::Dist(Pawn->GetActorLocation(), Target->GetActorLocation()) <= CatchDistance)
	{
		AIOwner->StopMovement();
		if (ARunGameState* GameState = ARunGameState::Get(AIOwner))
		{
			// 勝敗確定の通知でコントローラがツリーごと止めるので、ここでは終了させない
			GameState->TriggerGameOver();
			return;
		}
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	FChaseMemory* Memory = CastInstanceNodeMemory<FChaseMemory>(NodeMemory);

	// 視界を失っている間は最後に見た位置へ突っ込み続け、猶予を超えたら諦める
	if (BB->GetValueAsBool(SeeingTargetKey.SelectedKeyName))
	{
		Memory->LostSightTimer = 0.0f;
	}
	else
	{
		Memory->LostSightTimer += DeltaSeconds;
		if (Memory->LostSightTimer >= MemoryTime)
		{
			UE_LOG(LogQuietus, Log, TEXT("%s: chase memory expired -> Search"), *AIOwner->GetName());
			BB->ClearValue(TargetKey.SelectedKeyName);
			FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
			return;
		}
	}

	Memory->RepathTimer += DeltaSeconds;
	if (Memory->RepathTimer >= RepathInterval)
	{
		Memory->RepathTimer = 0.0f;
		Repath(OwnerComp);
	}
}

EBTNodeResult::Type UBTTask_ChaseTarget::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (AAIController* AIOwner = OwnerComp.GetAIOwner())
	{
		AIOwner->StopMovement();
	}
	return EBTNodeResult::Aborted;
}

void UBTTask_ChaseTarget::Repath(UBehaviorTreeComponent& OwnerComp) const
{
	AAIController* AIOwner = OwnerComp.GetAIOwner();
	const UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!AIOwner || !BB)
	{
		return;
	}

	AActor* Target = Cast<AActor>(BB->GetValueAsObject(TargetKey.SelectedKeyName));
	if (Target && BB->GetValueAsBool(SeeingTargetKey.SelectedKeyName))
	{
		// 既定（bStopOnOverlap）だとカプセル半径分手前で止まり、CatchDistance に届かない
		AIOwner->MoveToActor(Target, AcceptanceRadius, /*bStopOnOverlap*/ false);
	}
	else
	{
		AIOwner->MoveToLocation(BB->GetValueAsVector(LastKnownLocationKey.SelectedKeyName), AcceptanceRadius);
	}
}

FString UBTTask_ChaseTarget::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s: %s\nCatch %.0f cm / Memory %.1f s"),
		*Super::GetStaticDescription(), *TargetKey.SelectedKeyName.ToString(), CatchDistance, MemoryTime);
}
