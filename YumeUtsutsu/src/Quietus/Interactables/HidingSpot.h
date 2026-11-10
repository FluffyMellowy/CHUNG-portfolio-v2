#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "HidingSpot.generated.h"

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UHidingSpot : public UInterface
{
	GENERATED_BODY()
};

/**
 *  プレイヤーが中に隠れられ、敵がこじ開けられる場所（ロッカー等）の抽象。
 *  敵AIは具体的な家具クラスを知らず、このインターフェースと UHidingSpotSubsystem だけに依存する。
 */
class IHidingSpot
{
	GENERATED_BODY()

public:
	/** 中に誰か隠れているか */
	virtual bool IsOccupied() const = 0;

	/** 敵がこじ開ける。中身がいれば捕獲（ゲームオーバー）まで行う */
	virtual void BustOpen() = 0;
};
