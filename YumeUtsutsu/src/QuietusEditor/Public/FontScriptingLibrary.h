#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FontScriptingLibrary.generated.h"

class UFontFace;

/**
 *  フォントをエディタスクリプト（Python）から取り込むための補助。
 *  標準のインポーターは取り込み後に PostEditChange で Slate のフォントキャッシュを消しに行くので、
 *  Slate のないヘッドレス実行（-run=pythonscript）では落ちる。中身の入れ方はインポーターと同じ。
 *  使用例は Tools/Refactor/ui_fonts.py
 */
UCLASS()
class UFontScriptingLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** フォントファイルを FontFace にする（同名があれば中身を差し替える）。保存は呼び出し側で行う */
	UFUNCTION(BlueprintCallable, Category="Font|Editor Scripting")
	static UFontFace* ImportFontFace(const FString& SourceFile, const FString& PackagePath, const FString& AssetName);
};
