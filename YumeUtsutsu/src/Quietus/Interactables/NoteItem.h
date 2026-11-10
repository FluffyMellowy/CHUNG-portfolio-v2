#pragma once

#include "CoreMinimal.h"
#include "Interactables/PickupBase.h"
#include "NoteItem.generated.h"

/**
 *  ロアメモ。拾うと本文をオーバーレイ表示し、収集数がGameStateに記録される（進行には無関係）。
 */
UCLASS()
class ANoteItem : public APickupBase
{
	GENERATED_BODY()

protected:

	/** メモの見出し */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Note")
	FText NoteTitle;

	/** メモ本文 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Note", meta = (MultiLine = true))
	FText NoteBody;

public:
	/** 生成器がロアテキストを割り当てるのに使う */
	void SetNoteText(const FText& InTitle, const FText& InBody);

	//~ Begin IInteractable
	virtual FText GetInteractionPrompt_Implementation() const override;
	//~ End IInteractable

protected:
	/** 読み手（のコントローラ）が必要なので、プレイヤー以外からは拾えない */
	virtual bool CanBePickedUp(const APlayerCharacter* Interactor) const override { return Interactor != nullptr; }

	/** 収集数を記録し、本文をオーバーレイ表示する（閉じるまでポーズ） */
	virtual void OnPickedUp(APlayerCharacter* Interactor) override;
};
