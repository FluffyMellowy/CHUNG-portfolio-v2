#include "Interactables/AlarmClock.h"
#include "Components/StaticMeshComponent.h"
#include "Core/RunGameState.h"

#define LOCTEXT_NAMESPACE "Interaction"

AAlarmClock::AAlarmClock()
{
	// 秒針と鳴る動きのため
	PrimaryActorTick.bCanEverTick = true;

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	SetRootComponent(Body);
	Body->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Body->SetCollisionResponseToAllChannels(ECR_Ignore);
	Body->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	auto MakePart = [this](const TCHAR* Name)
	{
		UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Part->SetupAttachment(Body);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		return Part;
	};
	Glass = MakePart(TEXT("Glass"));
	Hook = MakePart(TEXT("Hook"));
	Hammer = MakePart(TEXT("Hammer"));
	BellL = MakePart(TEXT("BellL"));
	BellR = MakePart(TEXT("BellR"));
	HourHand = MakePart(TEXT("HourHand"));
	MinuteHand = MakePart(TEXT("MinuteHand"));
	SecondHand = MakePart(TEXT("SecondHand"));
}

void AAlarmClock::BeginPlay()
{
	Super::BeginPlay();
	BodyRestRotation = Body->GetRelativeRotation();
}

void AAlarmClock::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 秒針は 1 秒ごとに 6° 進める
	SecondHand->AddLocalRotation(FRotator(0.0f, 0.0f, -6.0f * DeltaSeconds));

	if (!bRinging)
	{
		return;
	}

	RingElapsed += DeltaSeconds;
	const float Phase = RingElapsed * HammerFrequency * UE_TWO_PI;
	Hammer->SetRelativeRotation(FRotator(0.0f, 0.0f, HammerSwing * FMath::Sin(Phase)));
	BellL->SetRelativeRotation(FRotator(0.0f, 0.0f, BellShake * FMath::Sin(Phase + 0.5f)));
	BellR->SetRelativeRotation(FRotator(0.0f, 0.0f, -BellShake * FMath::Sin(Phase + 0.5f)));
	Body->SetRelativeRotation(BodyRestRotation + FRotator(0.0f, BodyShake * FMath::Sin(Phase * 0.5f), 0.0f));

	if (!bWokeUp && RingElapsed >= RingDuration)
	{
		bWokeUp = true;
		if (ARunGameState* GameState = ARunGameState::Get(this))
		{
			GameState->TriggerEscape();
		}
	}
}

void AAlarmClock::Interact_Implementation(APlayerCharacter* Interactor)
{
	if (!bRinging)
	{
		bRinging = true;
		RingElapsed = 0.0f;
	}
}

FText AAlarmClock::GetInteractionPrompt_Implementation() const
{
	return bRinging ? FText::GetEmpty() : LOCTEXT("TouchAlarmClock", "目覚まし時計に触れる");
}

#undef LOCTEXT_NAMESPACE
