#pragma once

#include "CoreMinimal.h"

class ARoomPrefab;

namespace LevelGen
{
	struct FDoorway;

	/**
	 *  使用中の扉の内外 Depth cm 以内に食い込むプレハブ装飾（コリジョン持ち）を撤去する。
	 *  プレハブは扉の位置を知らずに内装されるため、扉の目の前を棚などが塞いで通行不能になるのを防ぐ。
	 *  コリジョンの無い純装飾は残す。
	 *  @return 撤去したメッシュ数
	 */
	int32 ClearDressingAtDoorways(const TArray<FDoorway>& Doorways, const TArray<ARoomPrefab*>& RoomActors, float Depth);
}
