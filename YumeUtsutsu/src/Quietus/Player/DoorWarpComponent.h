#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DoorWarpComponent.generated.h"

class ADoor;
class APlayerCharacter;

/**
 *  扉から迷路の別の扉の前へ移る（鍵を使ったとき）。
 *  画面が光で白く満ちる → 行き先へ移る → 光が引く。追っていたピエロはプレイヤーを見失い、出発した扉のあたりを探す。
 *  行き先の選び方は仮で、旧「虹色の扉」と同じ規則（遠い扉・ピエロから離れた扉・脱出部屋以外）。値は Dream Gimmicks の KeyWarp。
 */
UCLASS(ClassGroup=(Quietus), meta=(BlueprintSpawnableComponent))
class UDoorWarpComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDoorWarpComponent();

	/** From の扉から移り始める。移っている途中なら何もしない */
	bool Begin(ADoor* From);

	bool IsWarping() const { return bWarping; }

private:
	APlayerCharacter* GetPlayer() const;

	/** 行き先の扉の前の位置と向き（扉を背にする）を選ぶ。見つからなければ false */
	bool PickArrival(const ADoor* From, FVector& OutLocation, FRotator& OutRotation) const;

	void Teleport();
	void FinishReveal();

	/** 操作を止める / 返す */
	void SetInputBlocked(bool bBlocked) const;

	TWeakObjectPtr<ADoor> Departure;
	FTimerHandle StepTimer;
	bool bWarping = false;
};
