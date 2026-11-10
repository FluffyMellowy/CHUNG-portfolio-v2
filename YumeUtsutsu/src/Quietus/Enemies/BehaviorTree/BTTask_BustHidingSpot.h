#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_BustHidingSpot.generated.h"

/**
 *  近くに使用中の隠れ場所（ロッカー等）があれば歩み寄ってこじ開ける。無ければ即成功。
 *  捜索地点に着いた直後に置く（目撃されたまま隠れた場合の罰。視線を切ってから隠れれば
 *  最後に知覚した位置が隠れ場所から離れるので、ここでは見つからない）。
 */
UCLASS(meta = (DisplayName = "Bust Hiding Spot"))
class UBTTask_BustHidingSpot : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_BustHidingSpot();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual uint16 GetInstanceMemorySize() const override;
	virtual void InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const override;
	virtual void CleanupMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryClear::Type CleanupType) const override;
	virtual FString GetStaticDescription() const override;

protected:
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	/** 自分の位置からこの水平距離以内の使用中の隠れ場所を探す (cm) */
	UPROPERTY(EditAnywhere, Category="Bust", meta = (ClampMin = 0, Units = "cm"))
	float SearchRadius = 300.0f;

	/** 隠れ場所へ向かう時の到達判定半径 (cm) */
	UPROPERTY(EditAnywhere, Category="Bust", meta = (ClampMin = 0, Units = "cm"))
	float ApproachRadius = 100.0f;

	/** この水平距離まで近づいたらこじ開ける (cm) */
	UPROPERTY(EditAnywhere, Category="Bust", meta = (ClampMin = 0, Units = "cm"))
	float ReachDistance = 180.0f;

private:
	struct FBustMemory
	{
		TWeakObjectPtr<AActor> Spot;
	};
};
