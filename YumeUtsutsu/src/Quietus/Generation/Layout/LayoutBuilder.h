#pragma once

#include "CoreMinimal.h"
#include "Generation/GenerationSettings.h"
#include "Generation/Layout/LevelLayout.h"

namespace LevelGen
{
	/**
	 *  レイアウト生成の戦略インターフェース。
	 *  1回の試行で部屋配置・接続・開始/脱出部屋の決定まで行い、成立しなければ false を返す
	 *  （呼び出し側がシードを派生させて再試行する）。ワールドには一切触れない。
	 */
	class FLayoutBuilder
	{
	public:
		FLayoutBuilder(const FGenLayoutSettings& InSettings, const FString& InLogOwner)
			: Settings(InSettings), LogOwner(InLogOwner)
		{
		}
		virtual ~FLayoutBuilder() = default;

		virtual bool TryBuild(FLayout& OutLayout, FRandomStream& Rand) const = 0;

		/** ログ・統計表示用のモード名 */
		virtual const TCHAR* GetModeName() const = 0;

	protected:
		/** W×H の部屋がグリッド内（外周1タイルは壁用に空ける）に収まるか */
		bool FitsInGrid(int32 W, int32 H) const;

		/** W×H の部屋をグリッド内のランダムな位置に置いた候補を作る */
		FRoom MakeRandomRoom(int32 W, int32 H, FRandomStream& Rand) const;

		/** 既存の部屋と間隔込みで重ならなければ確定してグリッドへ書き込む */
		bool TryCommitRoom(FLayout& Layout, const FRoom& Candidate) const;

		/** 扉辺の外側に ExitClearCells 分の空きが一直線に続くか（グリッド外は空き扱い） */
		bool IsExitPathClear(const FLayout& Layout, const FIntPoint& Cell, const FIntPoint& Dir) const;

		/** 決まった脱出扉の外側を階段セット用に確保する */
		void ReserveExitPath(FLayout& Layout) const;

		const FGenLayoutSettings& Settings;
		FString LogOwner;
	};
}
