#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_SetMoveSpeed.generated.h"

/** ポーンの歩行速度（MaxWalkSpeed）を切り替える。各ブランチの先頭に置く */
UCLASS(meta = (DisplayName = "Set Move Speed"))
class UBTTask_SetMoveSpeed : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_SetMoveSpeed();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;

protected:
	/** 設定する速度 (cm/s)。巡回140・追跡450が基準（プレイヤー歩行250／スプリント550） */
	UPROPERTY(EditAnywhere, Category="Movement", meta = (ClampMin = 0, Units = "cm/s"))
	float Speed = 140.0f;
};
