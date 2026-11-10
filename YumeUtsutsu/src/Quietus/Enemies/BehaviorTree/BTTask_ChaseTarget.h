#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_ChaseTarget.generated.h"

/**
 *  追跡対象を追い続ける（完了しないタスク）。
 *   - 見えている間は対象へ、見失っている間は最後に見た位置へ、一定間隔で経路を引き直す
 *   - CatchDistance まで近づいたら捕獲＝ゲームオーバー（ロッカー内のプレイヤーは捕まえない）
 *   - 見失ったまま MemoryTime を過ぎたら追跡対象キーを消して失敗で終わる（→捜索へ）
 */
UCLASS(meta = (DisplayName = "Chase Target"))
class UBTTask_ChaseTarget : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_ChaseTarget();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual uint16 GetInstanceMemorySize() const override;
	virtual void InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const override;
	virtual void CleanupMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryClear::Type CleanupType) const override;
	virtual FString GetStaticDescription() const override;

protected:
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	/** 追跡対象（Object） */
	UPROPERTY(EditAnywhere, Category="Blackboard")
	FBlackboardKeySelector TargetKey;

	/** 最後に見た位置（Vector） */
	UPROPERTY(EditAnywhere, Category="Blackboard")
	FBlackboardKeySelector LastKnownLocationKey;

	/** 対象が視界内か（Bool） */
	UPROPERTY(EditAnywhere, Category="Blackboard")
	FBlackboardKeySelector SeeingTargetKey;

	/** この距離まで近づいたら捕獲＝ゲームオーバー (cm) */
	UPROPERTY(EditAnywhere, Category="Chase", meta = (ClampMin = 0, Units = "cm"))
	float CatchDistance = 120.0f;

	/** 移動目標への到達判定半径 (cm)。対象を追う時は中心間の距離。CatchDistance より小さくすること */
	UPROPERTY(EditAnywhere, Category="Chase", meta = (ClampMin = 0, Units = "cm"))
	float AcceptanceRadius = 80.0f;

	/** 経路を引き直す間隔（秒）。毎フレーム要求するとコーナーで経路計算が暴れる */
	UPROPERTY(EditAnywhere, Category="Chase", meta = (ClampMin = 0.05, Units = "s"))
	float RepathInterval = 0.25f;

	/** 視界を失ってもこの秒数は最後に見た位置へ追い続ける。
	 *  すぐ諦めると、体の向きの揺れで視界が明滅するたびに経路がリセットされ扉前で足踏みする */
	UPROPERTY(EditAnywhere, Category="Chase", meta = (ClampMin = 0, Units = "s"))
	float MemoryTime = 2.5f;

private:
	struct FChaseMemory
	{
		float RepathTimer = 0.0f;
		float LostSightTimer = 0.0f;
	};

	void Repath(UBehaviorTreeComponent& OwnerComp) const;
};
