#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Interactable.generated.h"

class APlayerCharacter;

// UHTが要求するインターフェース用の空クラス。実装はIInteractable側に書く
UINTERFACE(MinimalAPI, Blueprintable)
class UInteractable : public UInterface
{
	GENERATED_BODY()
};

/**
 *  プレイヤーが「使う」ことのできるオブジェクトの共通インターフェース。
 *  欠片の拾得・ドア開閉・隠れ場所など、E キーで作用する対象はすべてこれを実装する。
 *  BlueprintNativeEvent なので C++（_Implementation）でも BP でも実装可能。
 *  呼び出し側は必ず IInteractable::Execute_Interact() 経由で呼ぶこと（BP実装にも届くように）。
 */
class IInteractable
{
	GENERATED_BODY()

public:

	/** プレイヤーがこのオブジェクトに作用したときに呼ばれる */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interaction")
	void Interact(APlayerCharacter* Interactor);

	/** HUDに表示する操作プロンプト（例:「開ける」）を返す */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interaction")
	FText GetInteractionPrompt() const;
};
