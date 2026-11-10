#include "Battery.h"
#include "Player/PlayerCharacter.h"
#include "Player/FlashlightComponent.h"
#include "Core/DreamFeedbackSettings.h"
#include "Feedback/FeedbackSubsystem.h"

#define LOCTEXT_NAMESPACE "Interaction"

void ABattery::OnPickedUp(APlayerCharacter* Interactor)
{
	if (Interactor && Interactor->GetFlashlight())
	{
		Interactor->GetFlashlight()->Recharge(RechargeAmount);
	}
	UFeedbackSubsystem::Play2D(this, UDreamFeedbackSettings::Get().PickupBattery, TEXT("PickupBattery"));
}

FText ABattery::GetInteractionPrompt_Implementation() const
{
	return LOCTEXT("PickUpBattery", "電池を拾う");
}

#undef LOCTEXT_NAMESPACE
