#pragma once

#include "CoreMinimal.h"

/**
 *  複数のクラスが同じ値を参照する固定名（レベル名・アクタータグ）。
 *  文字列リテラルを各所に散らすと片方だけ変えた時に静かに壊れるため、ここに集約する。
 */
namespace GameNames
{
	/** 手続き生成のランを行うレベル */
	inline const FName GeneratedLevel(TEXT("Lvl_Generated"));

	/** メインメニューのレベル */
	inline const FName MainMenuLevel(TEXT("Lvl_MainMenu"));

	/** 生成器が巡回ポイントに付け、敵AIが巡回先として収集するタグ */
	inline const FName PatrolPointTag(TEXT("PatrolPoint"));
}
