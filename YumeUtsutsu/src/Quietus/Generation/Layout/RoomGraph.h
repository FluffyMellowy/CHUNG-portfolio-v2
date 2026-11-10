#pragma once

#include "CoreMinimal.h"
#include "Generation/Layout/LevelLayout.h"

namespace LevelGen
{
	/**
	 *  部屋をどう繋ぐかを決める。部屋中心間の距離でMST（プリム法）を作り、
	 *  MSTに含まれない辺から短い順に ExtraLoops 本を追加する（行き止まりだけの迷路を避ける）。
	 *  @return 部屋番号のペア一覧（MST → ループの順）
	 */
	TArray<TPair<int32, int32>> BuildRoomConnections(const TArray<FRoom>& Rooms, int32 ExtraLoops);
}
