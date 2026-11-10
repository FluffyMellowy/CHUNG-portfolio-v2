#pragma once

#include "CoreMinimal.h"
#include "CabinetLootTable.generated.h"

/** キャビネットの引き出し1段ごとの中身の抽選重み（合計で割るので正規化は不要） */
USTRUCT(BlueprintType)
struct FCabinetLootTable
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0))
	float BatteryWeight = 0.30f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0))
	float ThrowableWeight = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0))
	float NoteWeight = 0.15f;

	/** 空振り */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0))
	float EmptyWeight = 0.30f;
};
