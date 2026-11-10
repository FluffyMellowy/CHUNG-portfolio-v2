#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BehaviorTreeBuilderLibrary.generated.h"

class UBehaviorTree;
class UBlackboardData;
class UBTDecorator;
class UBTNode;
class UEdGraphNode;

/**
 *  ビヘイビアツリーのエディタグラフをスクリプトから組み立てるためのヘルパー。
 *  エンジンの Python からはグラフを編集できないので、ここで必要な操作だけ公開する。
 *  手でノードを置いた時と同じグラフを作るので、出来上がったツリーは BT エディタでそのまま編集できる。
 *  使用例は Tools/BehaviorTree/build_enemy_bt.py
 */
UCLASS()
class UBehaviorTreeBuilderLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** グラフを捨ててルートだけの状態に作り直す。ブラックボードは維持。ルートのグラフノードを返す */
	UFUNCTION(BlueprintCallable, Category="BT Builder")
	static UEdGraphNode* ResetGraph(UBehaviorTree* Tree);

	/** Parent の下にコンポジット/タスクを追加する。子の実行順は追加順 */
	UFUNCTION(BlueprintCallable, Category="BT Builder")
	static UEdGraphNode* AddChildNode(UEdGraphNode* Parent, TSubclassOf<UBTNode> NodeClass);

	/** Owner にデコレータを付ける */
	UFUNCTION(BlueprintCallable, Category="BT Builder")
	static UEdGraphNode* AddDecorator(UEdGraphNode* Owner, TSubclassOf<UBTDecorator> DecoratorClass);

	/** ノードのプロパティをテキスト形式で設定する（詳細パネルで値を変えた時と同じ通知を出す）
	 *  例: "450.0" / "Both" / "(DefaultValue=80.0)" */
	UFUNCTION(BlueprintCallable, Category="BT Builder")
	static bool SetNodeProperty(UEdGraphNode* Node, FName PropertyName, const FString& Value);

	/** ノードの FBlackboardKeySelector にキーを設定してツリーのブラックボードで解決する */
	UFUNCTION(BlueprintCallable, Category="BT Builder")
	static bool SetNodeBlackboardKey(UEdGraphNode* Node, FName PropertyName, FName KeyName);

	/** ノードを並べ直し、グラフからランタイムのツリーを組み直す。保存前に必ず呼ぶ */
	UFUNCTION(BlueprintCallable, Category="BT Builder")
	static void FinalizeGraph(UBehaviorTree* Tree);

	/** Object キーの BaseClass を設定する（Python からは書き込めないプロパティ） */
	UFUNCTION(BlueprintCallable, Category="BT Builder")
	static bool SetObjectKeyBaseClass(UBlackboardData* Blackboard, FName KeyName, UClass* BaseClass);
};
