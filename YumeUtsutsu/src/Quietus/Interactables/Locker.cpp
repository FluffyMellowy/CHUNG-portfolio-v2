#include "Interactables/Locker.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Core/RunGameState.h"
#include "Interactables/HidingSpotSubsystem.h"
#include "Player/PlayerCharacter.h"
#include "Quietus.h"
#include "Core/DreamFeedbackSettings.h"
#include "Feedback/FeedbackSubsystem.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "Interaction"

ALocker::ALocker()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	// ボディ実測: 幅90・奥行80(ローカルy -80..0)・高205、開口=ローカル+Y面。
	// マーカー規約（+X=正面）に合わせ、-90°回して開口をアクター+Xへ向ける
	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(Root);
	BodyMesh->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	BodyMesh->SetCollisionProfileName(FName("BlockAll"));

	// 扉実測: 厚みX・幅82(+Y)・高197。蝶番を開口の+Y端に置き、幅をアクター-Yへ向ける。
	// 開閉は正のYawで外(+X)側へ開く
	DoorPivot = CreateDefaultSubobject<USceneComponent>(TEXT("DoorPivot"));
	DoorPivot->SetupAttachment(Root);
	DoorPivot->SetRelativeLocation(FVector(0.0f, 41.0f, 4.0f));

	DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
	DoorMesh->SetupAttachment(DoorPivot);
	DoorMesh->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
	DoorMesh->SetCollisionProfileName(FName("BlockAll"));
	// 中の人の視線トレースが扉に当たって出る操作ができるよう、Pawnだけ素通しにはしない
}

void ALocker::BeginPlay()
{
	Super::BeginPlay();

	if (UHidingSpotSubsystem* Registry = GetWorld()->GetSubsystem<UHidingSpotSubsystem>())
	{
		Registry->Register(this);
	}
}

void ALocker::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UHidingSpotSubsystem* Registry = GetWorld()->GetSubsystem<UHidingSpotSubsystem>())
	{
		Registry->Unregister(this);
	}

	Super::EndPlay(EndPlayReason);
}

void ALocker::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const float CurrentYaw = DoorPivot->GetRelativeRotation().Yaw;
	if (!FMath::IsNearlyEqual(CurrentYaw, TargetDoorYaw, 0.01f))
	{
		const float NewYaw = FMath::FInterpConstantTo(CurrentYaw, TargetDoorYaw, DeltaSeconds, DoorSpeed);
		DoorPivot->SetRelativeRotation(FRotator(0.0f, NewYaw, 0.0f));
	}
}

void ALocker::Interact_Implementation(APlayerCharacter* Interactor)
{
	if (!Interactor)
	{
		return;
	}
	if (Occupant == nullptr)
	{
		Enter(Interactor);
	}
	else if (Occupant == Interactor)
	{
		Exit();
	}
}

FText ALocker::GetInteractionPrompt_Implementation() const
{
	return Occupant ? LOCTEXT("ExitLocker", "出る") : LOCTEXT("EnterLocker", "隠れる");
}

void ALocker::Enter(APlayerCharacter* Player)
{
	Occupant = Player;

	// 内部中央へ移して移動を止める（視点回転は生かす）。
	// 扉はわずかに半開きにして隙間から外が見えるようにする
	Player->SetActorLocation(GetActorLocation() - GetActorForwardVector() * 40.0f + FVector(0, 0, 98.0f));
	Player->GetCharacterMovement()->DisableMovement();
	Player->SetHiddenInLocker(true, this);
	TargetDoorYaw = AjarAngle;
	PlayOpenThenClose();

	UE_LOG(LogQuietus, Log, TEXT("%s: player hid inside"), *GetName());
}

void ALocker::Exit()
{
	APlayerCharacter* Player = Occupant;
	Occupant = nullptr;

	// 正面(+X)の外へ出して移動を返す
	Player->SetActorLocation(GetExitSpot() + FVector(0, 0, 98.0f));
	Player->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	Player->SetHiddenInLocker(false);
	TargetDoorYaw = 0.0f;
	PlayOpenThenClose();

	UE_LOG(LogQuietus, Log, TEXT("%s: player left"), *GetName());
}

FVector ALocker::GetExitSpot() const
{
	// 出口に床が無ければロッカー位置へ退避（奈落落下防止）
	const FVector ExitSpot = GetActorLocation() + GetActorForwardVector() * 90.0f;
	FHitResult Floor;
	if (!GetWorld()->LineTraceSingleByChannel(Floor,
		ExitSpot + FVector(0, 0, 150.0f), ExitSpot - FVector(0, 0, 150.0f), ECC_Visibility))
	{
		return GetActorLocation() + GetActorForwardVector() * 20.0f;
	}
	return ExitSpot;
}

void ALocker::DragOut()
{
	APlayerCharacter* Player = Occupant;
	if (!Player)
	{
		return;
	}
	Occupant = nullptr;
	Player->SetActorLocation(GetExitSpot() + FVector(0, 0, 98.0f));
	Player->SetHiddenInLocker(false);
	UE_LOG(LogQuietus, Log, TEXT("%s: occupant dragged out"), *GetName());
}

void ALocker::PlayOpenThenClose()
{
	// 出入りは一瞬なので、扉を開けて閉める音を少しずらして続けて鳴らす
	const UDreamFeedbackSettings& Sfx = UDreamFeedbackSettings::Get();
	UFeedbackSubsystem::PlayAt(this, Sfx.LockerOpen, Sfx.InteractAttenuation, GetActorLocation(), TEXT("LockerOpen"));
	TWeakObjectPtr<ALocker> WeakThis(this);
	GetWorldTimerManager().SetTimer(CloseSoundTimer, FTimerDelegate::CreateLambda([WeakThis]()
	{
		if (ALocker* Self = WeakThis.Get())
		{
			const UDreamFeedbackSettings& S = UDreamFeedbackSettings::Get();
			UFeedbackSubsystem::PlayAt(Self, S.LockerClose, S.InteractAttenuation, Self->GetActorLocation(), TEXT("LockerClose"));
		}
	}), FMath::Max(Sfx.LockerCloseDelay, 0.01f), false);
}

void ALocker::BustOpen()
{
	TargetDoorYaw = OpenAngle;
	GetWorldTimerManager().ClearTimer(CloseSoundTimer);
	const UDreamFeedbackSettings& Sfx = UDreamFeedbackSettings::Get();
	UFeedbackSubsystem::PlayAt(this, Sfx.LockerBust, Sfx.InteractAttenuation, GetActorLocation(), TEXT("LockerBust"));

	if (Occupant)
	{
		UE_LOG(LogQuietus, Log, TEXT("%s: busted open - occupant caught"), *GetName());
		if (ARunGameState* GameState = ARunGameState::Get(this))
		{
			GameState->TriggerGameOver();
		}
	}
}

#undef LOCTEXT_NAMESPACE
