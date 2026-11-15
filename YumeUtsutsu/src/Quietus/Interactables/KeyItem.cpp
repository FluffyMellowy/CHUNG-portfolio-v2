#include "Interactables/KeyItem.h"
#include "Core/DreamFeedbackSettings.h"
#include "Feedback/FeedbackSubsystem.h"
#include "Player/ItemSlotsComponent.h"
#include "Player/PlayerCharacter.h"

#define LOCTEXT_NAMESPACE "Interaction"

void AKeyItem::OnPickedUp(APlayerCharacter* Interactor)
{
	if (Interactor && Interactor->GetItemSlots())
	{
		Interactor->GetItemSlots()->AddKey(GetClass());
	}
	UFeedbackSubsystem::Play2D(this, UDreamFeedbackSettings::Get().PickupGeneric, TEXT("PickupGeneric"));
}

FText AKeyItem::GetInteractionPrompt_Implementation() const
{
	return LOCTEXT("PickUpKey", "鍵を拾う");
}

#undef LOCTEXT_NAMESPACE
