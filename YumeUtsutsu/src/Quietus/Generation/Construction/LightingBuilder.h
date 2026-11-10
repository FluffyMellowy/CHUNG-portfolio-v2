#pragma once

#include "CoreMinimal.h"
#include "Generation/GenerationSettings.h"

namespace LevelGen
{
	struct FContext;

	/**
	 *  天井で外光が遮られた室内に、最低限の電球ライトを置く。
	 *  部屋は中心に1灯、廊下は一定間隔で部屋より弱い灯り（主光源は懐中電灯）。
	 */
	class FLightingBuilder
	{
	public:
		FLightingBuilder(const FContext& InContext, const FGenLightingSettings& InSettings, float InCeilingHeight)
			: Context(InContext), Settings(InSettings), CeilingHeight(InCeilingHeight)
		{
		}

		/** @return 置いたライトの数 */
		int32 Build() const;

	private:
		bool SpawnLight(const FVector& Location, float Intensity, float Radius) const;

		const FContext& Context;
		const FGenLightingSettings& Settings;
		float CeilingHeight = 300.0f;
	};
}
