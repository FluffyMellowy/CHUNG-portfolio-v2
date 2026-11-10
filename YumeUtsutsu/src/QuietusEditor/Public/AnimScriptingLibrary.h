#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AnimScriptingLibrary.generated.h"

class UBlendSpace;

/**
 *  アニメーションアセットをエディタスクリプト（Python）から組むための補助。
 *  ブレンドスペースはサンプル配列を書き換えただけでは実行時の補間データが作られない
 *  （エディタのブレンドスペース画面は追加のたびに作り直している）ため、その処理だけを公開する。
 *  使用例は Tools/Refactor/enemy_animbp.py
 */
UCLASS()
class UAnimScriptingLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** サンプルを検証し、実行時の補間データを作り直す。サンプルや軸を書き換えた後に呼ぶ */
	UFUNCTION(BlueprintCallable, Category="Animation|Editor Scripting")
	static void RebuildBlendSpace(UBlendSpace* BlendSpace);
};
