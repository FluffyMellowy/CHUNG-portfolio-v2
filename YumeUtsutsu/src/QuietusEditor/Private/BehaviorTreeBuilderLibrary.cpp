#include "BehaviorTreeBuilderLibrary.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/BTCompositeNode.h"
#include "BehaviorTree/BTDecorator.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Composites/BTComposite_SimpleParallel.h"
#include "BehaviorTree/Tasks/BTTask_RunBehavior.h"
#include "BehaviorTreeGraph.h"
#include "BehaviorTreeGraphNode_Composite.h"
#include "BehaviorTreeGraphNode_Decorator.h"
#include "BehaviorTreeGraphNode_Root.h"
#include "BehaviorTreeGraphNode_SimpleParallel.h"
#include "BehaviorTreeGraphNode_SubtreeTask.h"
#include "BehaviorTreeGraphNode_Task.h"
#include "EdGraphSchema_BehaviorTree.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Modules/ModuleManager.h"
#include "UObject/UObjectHash.h"

DEFINE_LOG_CATEGORY_STATIC(LogBTBuilder, Log, All);

namespace
{
	// 横に並べる間隔と段の高さ。デコレータがノード内に縦に積まれるので段は広めに取る
	constexpr int32 ColumnWidth = 320;
	constexpr int32 RowHeight = 300;

	TArray<UBehaviorTreeGraphNode*> GetChildren(const UBehaviorTreeGraphNode* Node)
	{
		TArray<UBehaviorTreeGraphNode*> Children;
		if (const UEdGraphPin* OutPin = Node->GetOutputPin())
		{
			for (const UEdGraphPin* Linked : OutPin->LinkedTo)
			{
				if (UBehaviorTreeGraphNode* Child = Cast<UBehaviorTreeGraphNode>(Linked->GetOwningNode()))
				{
					Children.Add(Child);
				}
			}
		}
		return Children;
	}

	// 葉を左から順に並べ、親は子の中央に置く。戻り値はノードの X
	int32 LayoutSubtree(UBehaviorTreeGraphNode* Node, int32 Depth, int32& NextLeafX)
	{
		const TArray<UBehaviorTreeGraphNode*> Children = GetChildren(Node);
		if (Children.Num() == 0)
		{
			Node->NodePosX = NextLeafX;
			NextLeafX += ColumnWidth;
		}
		else
		{
			const int32 FirstX = LayoutSubtree(Children[0], Depth + 1, NextLeafX);
			int32 LastX = FirstX;
			for (int32 Index = 1; Index < Children.Num(); ++Index)
			{
				LastX = LayoutSubtree(Children[Index], Depth + 1, NextLeafX);
			}
			Node->NodePosX = (FirstX + LastX) / 2;
		}
		Node->NodePosY = Depth * RowHeight;
		return Node->NodePosX;
	}

	UBehaviorTreeGraphNode_Root* FindRoot(const UEdGraph* Graph)
	{
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (UBehaviorTreeGraphNode_Root* Root = Cast<UBehaviorTreeGraphNode_Root>(Node))
			{
				return Root;
			}
		}
		return nullptr;
	}

	UObject* GetInstance(UEdGraphNode* Node, FName PropertyName, FProperty*& OutProperty)
	{
		const UAIGraphNode* AINode = Cast<UAIGraphNode>(Node);
		UObject* Instance = AINode ? AINode->NodeInstance.Get() : nullptr;
		if (!Instance)
		{
			UE_LOG(LogBTBuilder, Error, TEXT("%s has no node instance"), *GetNameSafe(Node));
			return nullptr;
		}
		OutProperty = FindFProperty<FProperty>(Instance->GetClass(), PropertyName);
		if (!OutProperty)
		{
			UE_LOG(LogBTBuilder, Error, TEXT("%s has no property '%s'"), *Instance->GetClass()->GetName(), *PropertyName.ToString());
			return nullptr;
		}
		return Instance;
	}
}

UEdGraphNode* UBehaviorTreeBuilderLibrary::ResetGraph(UBehaviorTree* Tree)
{
	if (!Tree)
	{
		return nullptr;
	}

	// グラフのクラス情報キャッシュは BT エディタモジュールが持っている
	FModuleManager::LoadModuleChecked<IModuleInterface>(TEXT("BehaviorTreeEditor"));

	Tree->Modify();
	UBlackboardData* Blackboard = Tree->BlackboardAsset;

	// 古いグラフとノードは一時パッケージへ追い出して保存対象から外す
	TArray<UObject*> OldObjects;
	GetObjectsWithOuter(Tree, OldObjects, EGetObjectsFlags::None);
	for (UObject* Object : OldObjects)
	{
		if (Object->IsA<UEdGraph>() || Object->IsA<UBTNode>())
		{
			Object->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional);
		}
	}
	Tree->BTGraph = nullptr;
	Tree->RootNode = nullptr;
	Tree->RootDecorators.Empty();
	Tree->RootDecoratorOps.Empty();

	// BT エディタが新規アセットを開いた時と同じ手順でグラフを作る
	UBehaviorTreeGraph* Graph = CastChecked<UBehaviorTreeGraph>(FBlueprintEditorUtils::CreateNewGraph(
		Tree, TEXT("Behavior Tree"), UBehaviorTreeGraph::StaticClass(), UEdGraphSchema_BehaviorTree::StaticClass()));
	Tree->BTGraph = Graph;
	Graph->GetSchema()->CreateDefaultNodesForGraph(*Graph);
	Graph->OnCreated();
	Graph->Initialize();

	// ルートは生成時に適当なブラックボードを拾うので元に戻す
	UBehaviorTreeGraphNode_Root* Root = FindRoot(Graph);
	if (Root)
	{
		Root->BlackboardAsset = Blackboard;
	}
	Tree->BlackboardAsset = Blackboard;
	return Root;
}

UEdGraphNode* UBehaviorTreeBuilderLibrary::AddChildNode(UEdGraphNode* Parent, TSubclassOf<UBTNode> NodeClass)
{
	UBehaviorTreeGraphNode* ParentNode = Cast<UBehaviorTreeGraphNode>(Parent);
	UBehaviorTreeGraph* Graph = ParentNode ? Cast<UBehaviorTreeGraph>(ParentNode->GetGraph()) : nullptr;
	if (!Graph || !NodeClass)
	{
		UE_LOG(LogBTBuilder, Error, TEXT("AddChildNode: invalid parent or class"));
		return nullptr;
	}

	TSubclassOf<UBehaviorTreeGraphNode> GraphNodeClass;
	if (NodeClass->IsChildOf(UBTComposite_SimpleParallel::StaticClass()))
	{
		GraphNodeClass = UBehaviorTreeGraphNode_SimpleParallel::StaticClass();
	}
	else if (NodeClass->IsChildOf(UBTCompositeNode::StaticClass()))
	{
		GraphNodeClass = UBehaviorTreeGraphNode_Composite::StaticClass();
	}
	else if (NodeClass->IsChildOf(UBTTask_RunBehavior::StaticClass()))
	{
		GraphNodeClass = UBehaviorTreeGraphNode_SubtreeTask::StaticClass();
	}
	else if (NodeClass->IsChildOf(UBTTaskNode::StaticClass()))
	{
		GraphNodeClass = UBehaviorTreeGraphNode_Task::StaticClass();
	}
	else
	{
		UE_LOG(LogBTBuilder, Error, TEXT("AddChildNode: %s is not a composite or task"), *NodeClass->GetName());
		return nullptr;
	}

	// スキーマの「新規ノード」アクションと同じ手順（インスタンスは PostPlacedNewNode が作る）
	UBehaviorTreeGraphNode* Node = NewObject<UBehaviorTreeGraphNode>(Graph, GraphNodeClass, NAME_None, RF_Transactional);
	Node->ClassData = FGraphNodeClassData(NodeClass.Get(), FString());
	// エディタは兄弟の実行順を X 座標で並べ直すことがある。仮の X を生成順に振って順番を守る
	Node->NodePosX = Graph->Nodes.Num() * 10;
	Graph->AddNode(Node, false, false);
	Node->CreateNewGuid();
	Node->PostPlacedNewNode();
	Node->AllocateDefaultPins();

	UEdGraphPin* OutPin = ParentNode->GetOutputPin();
	UEdGraphPin* InPin = Node->GetInputPin();
	if (!OutPin || !InPin)
	{
		UE_LOG(LogBTBuilder, Error, TEXT("AddChildNode: %s cannot have children"), *ParentNode->GetName());
		return nullptr;
	}
	OutPin->MakeLinkTo(InPin);
	return Node;
}

UEdGraphNode* UBehaviorTreeBuilderLibrary::AddDecorator(UEdGraphNode* Owner, TSubclassOf<UBTDecorator> DecoratorClass)
{
	UBehaviorTreeGraphNode* OwnerNode = Cast<UBehaviorTreeGraphNode>(Owner);
	UEdGraph* Graph = OwnerNode ? OwnerNode->GetGraph() : nullptr;
	if (!Graph || !DecoratorClass)
	{
		UE_LOG(LogBTBuilder, Error, TEXT("AddDecorator: invalid owner or class"));
		return nullptr;
	}

	UBehaviorTreeGraphNode_Decorator* Decorator = NewObject<UBehaviorTreeGraphNode_Decorator>(Graph);
	Decorator->ClassData = FGraphNodeClassData(DecoratorClass.Get(), FString());
	OwnerNode->AddSubNode(Decorator, Graph);
	return Decorator;
}

bool UBehaviorTreeBuilderLibrary::SetNodeProperty(UEdGraphNode* Node, FName PropertyName, const FString& Value)
{
	FProperty* Property = nullptr;
	UObject* Instance = GetInstance(Node, PropertyName, Property);
	if (!Instance)
	{
		return false;
	}

	Instance->PreEditChange(Property);
	if (!Property->ImportText_InContainer(*Value, Instance, Instance, PPF_None))
	{
		UE_LOG(LogBTBuilder, Error, TEXT("%s.%s: cannot parse '%s'"), *Instance->GetClass()->GetName(), *PropertyName.ToString(), *Value);
		return false;
	}
	FPropertyChangedEvent Event(Property, EPropertyChangeType::ValueSet);
	Instance->PostEditChangeProperty(Event);
	return true;
}

bool UBehaviorTreeBuilderLibrary::SetNodeBlackboardKey(UEdGraphNode* Node, FName PropertyName, FName KeyName)
{
	FProperty* Property = nullptr;
	UObject* Instance = GetInstance(Node, PropertyName, Property);
	const FStructProperty* StructProperty = CastField<FStructProperty>(Property);
	if (!Instance || !StructProperty || StructProperty->Struct != FBlackboardKeySelector::StaticStruct())
	{
		UE_LOG(LogBTBuilder, Error, TEXT("%s is not a blackboard key selector"), *PropertyName.ToString());
		return false;
	}

	const UBehaviorTree* Tree = Instance->GetTypedOuter<UBehaviorTree>();
	const UBlackboardData* Blackboard = Tree ? Tree->BlackboardAsset.Get() : nullptr;
	if (!Blackboard)
	{
		UE_LOG(LogBTBuilder, Error, TEXT("%s: tree has no blackboard"), *GetNameSafe(Instance));
		return false;
	}

	FBlackboardKeySelector* Selector = StructProperty->ContainerPtrToValuePtr<FBlackboardKeySelector>(Instance);
	Instance->PreEditChange(Property);
	Selector->SelectedKeyName = KeyName;
	Selector->ResolveSelectedKey(*Blackboard);
	if (!Selector->IsSet())
	{
		UE_LOG(LogBTBuilder, Error, TEXT("%s: key '%s' not found in %s"), *Instance->GetClass()->GetName(), *KeyName.ToString(), *Blackboard->GetName());
		return false;
	}

	// ノードのキーフィルタに合わない型（Object キーの BaseClass 不一致など）はエディタのドロップダウンにも出ない
	if (Selector->AllowedTypes.Num() > 0)
	{
		const FBlackboardEntry* Entry = Blackboard->GetKey(Selector->GetSelectedKeyID());
		const bool bAllowed = Entry && Entry->KeyType && Selector->AllowedTypes.ContainsByPredicate(
			[Entry](const TObjectPtr<UBlackboardKeyType>& Filter) { return Filter && Entry->KeyType->IsAllowedByFilter(Filter.Get()); });
		if (!bAllowed)
		{
			UE_LOG(LogBTBuilder, Warning, TEXT("%s.%s: key '%s' does not match the node's key filter"),
				*Instance->GetClass()->GetName(), *PropertyName.ToString(), *KeyName.ToString());
		}
	}

	// BB デコレータはキー型の変更通知で比較方法（OperationType）を決めるので、型の変更として通知する
	FProperty* KeyTypeProperty = FBlackboardKeySelector::StaticStruct()->FindPropertyByName(
		GET_MEMBER_NAME_CHECKED(FBlackboardKeySelector, SelectedKeyType));
	FPropertyChangedEvent Event(KeyTypeProperty, EPropertyChangeType::ValueSet);
	Event.SetActiveMemberProperty(Property);
	Instance->PostEditChangeProperty(Event);
	return true;
}

void UBehaviorTreeBuilderLibrary::FinalizeGraph(UBehaviorTree* Tree)
{
	UBehaviorTreeGraph* Graph = Tree ? Cast<UBehaviorTreeGraph>(Tree->BTGraph) : nullptr;
	UBehaviorTreeGraphNode_Root* Root = Graph ? FindRoot(Graph) : nullptr;
	if (!Root)
	{
		UE_LOG(LogBTBuilder, Error, TEXT("FinalizeGraph: %s has no graph"), *GetNameSafe(Tree));
		return;
	}

	int32 NextLeafX = 0;
	LayoutSubtree(Root, 0, NextLeafX);

	// エディタの保存時と同じ処理（グラフ → ランタイムのノード構造）
	Graph->OnSave();
	Tree->MarkPackageDirty();

	UE_LOG(LogBTBuilder, Log, TEXT("%s rebuilt: root=%s, %d graph nodes"),
		*Tree->GetName(), *GetNameSafe(Tree->RootNode), Graph->Nodes.Num());
}

bool UBehaviorTreeBuilderLibrary::SetObjectKeyBaseClass(UBlackboardData* Blackboard, FName KeyName, UClass* BaseClass)
{
	if (!Blackboard || !BaseClass)
	{
		return false;
	}

	for (FBlackboardEntry& Entry : Blackboard->Keys)
	{
		if (Entry.EntryName != KeyName)
		{
			continue;
		}
		UBlackboardKeyType_Object* KeyType = Cast<UBlackboardKeyType_Object>(Entry.KeyType);
		if (!KeyType)
		{
			UE_LOG(LogBTBuilder, Error, TEXT("%s.%s is not an Object key"), *Blackboard->GetName(), *KeyName.ToString());
			return false;
		}
		Blackboard->Modify();
		KeyType->Modify();
		KeyType->BaseClass = BaseClass;
		Blackboard->MarkPackageDirty();
		return true;
	}

	UE_LOG(LogBTBuilder, Error, TEXT("%s has no key '%s'"), *Blackboard->GetName(), *KeyName.ToString());
	return false;
}
