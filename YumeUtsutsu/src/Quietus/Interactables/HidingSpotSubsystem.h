#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "HidingSpotSubsystem.generated.h"

/**
 *  ワールド内の隠れ場所（IHidingSpot 実装アクター）の登録簿。
 *  隠れ場所は BeginPlay/EndPlay で自分を登録・解除し、敵AIは全アクターを走査する代わりにここへ問い合わせる。
 */
UCLASS()
class UHidingSpotSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Spot は IHidingSpot を実装していること */
	void Register(AActor* Spot);
	void Unregister(AActor* Spot);

	/** Location から水平距離 Radius 以内にある、使用中の隠れ場所（無ければ null） */
	AActor* FindOccupiedNear(const FVector& Location, float Radius) const;

private:
	TArray<TWeakObjectPtr<AActor>> Spots;
};
