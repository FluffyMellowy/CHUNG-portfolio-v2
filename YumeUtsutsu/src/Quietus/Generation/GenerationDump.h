#pragma once

#include "CoreMinimal.h"

class UWorld;

namespace LevelGen
{
#if !UE_BUILD_SHIPPING
	/**
	 *  ワールド内の全アクター・コンポーネント・ISMインスタンスの配置をテキストで書き出す（回帰確認用）。
	 *  同じシードで生成した2つのダンプを diff すれば、リファクタリングで生成結果が変わっていないかを確認できる。
	 */
	void DumpWorldLayout(UWorld* World, const FString& FilePath);
#endif
}
