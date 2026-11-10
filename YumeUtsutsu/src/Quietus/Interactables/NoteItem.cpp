#include "NoteItem.h"
#include "Core/RunGameState.h"
#include "Player/FirstPersonPlayerController.h"
#include "Player/PlayerCharacter.h"
#include "Core/DreamFeedbackSettings.h"
#include "Feedback/FeedbackSubsystem.h"

#define LOCTEXT_NAMESPACE "Interaction"

void ANoteItem::SetNoteText(const FText& InTitle, const FText& InBody)
{
	NoteTitle = InTitle;
	NoteBody = InBody;
}

void ANoteItem::OnPickedUp(APlayerCharacter* Interactor)
{
	if (ARunGameState* GameState = ARunGameState::Get(this))
	{
		GameState->AddNote();
	}

	// 読んでいる間はポーズになるので、ポーズ中も鳴る音として出す
	UFeedbackSubsystem::Play2D(this, UDreamFeedbackSettings::Get().PickupNote, TEXT("PickupNote"), 1.0f, true);

	// 本文をオーバーレイ表示（閉じるまでポーズ）。表示はプレイヤーコントローラが受け持つ
	if (AFirstPersonPlayerController* PC = Cast<AFirstPersonPlayerController>(Interactor->GetController()))
	{
		PC->ShowNote(NoteTitle, NoteBody);
	}
}

FText ANoteItem::GetInteractionPrompt_Implementation() const
{
	return LOCTEXT("ReadNote", "メモを読む");
}

#undef LOCTEXT_NAMESPACE
