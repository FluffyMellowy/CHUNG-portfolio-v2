#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_UpdatePatrolPoint.generated.h"

/**
 *  巡回先をブラックボードに書き込む。
 *  今の巡回先に着いていれば次のポイントへ進め、まだなら同じポイントのまま
 *  （追跡や捜索から戻った時は、途中だった巡回先へ向かい直す）。
 *  ポイントの一覧は AEnemyController がレベルから集めたものを使う。
 */
UCLASS(meta = (DisplayName = "Update Patrol Point"))
class UBTTask_UpdatePatrolPoint : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_UpdatePatrolPoint();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual uint16 GetInstanceMemorySize() const override;
	virtual void InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const override;
	virtual void CleanupMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryClear::Type CleanupType) const override;
	virtual FString GetStaticDescription() const override;

protected:
	/** 巡回先を書き込むキー（Object） */
	UPROPERTY(EditAnywhere, Category="Blackboard")
	FBlackboardKeySelector PatrolPointKey;

	/** 巡回先からこの水平距離以内なら「着いた」とみなして次へ進む (cm)。MoveTo の許容半径より少し大きく */
	UPROPERTY(EditAnywhere, Category="Patrol", meta = (ClampMin = 0, Units = "cm"))
	float ArrivedRadius = 150.0f;

private:
	struct FPatrolMemory
	{
		int32 Index = 0;
	};
};
