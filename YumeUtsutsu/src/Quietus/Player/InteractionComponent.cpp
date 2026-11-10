#include "InteractionComponent.h"
#include "Camera/CameraComponent.h"
#include "Interactables/Interactable.h"
#include "Player/PlayerCharacter.h"

UInteractionComponent::UInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	// 視点カメラをオーナーから探してキャッシュ（一人称キャラクターならFPカメラが見つかる）
	ViewCamera = GetOwner()->FindComponentByClass<UCameraComponent>();
}

void UInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateFocus();

#if WITH_EDITORONLY_DATA
	// 自動テスト用フック（エディタ専用）。実際のEキーと同じくキャラクターの
	// DoInteract 経路を通す（ロッカー内ルーティング等を含めた完全パリティ）
	if (bDebugTriggerInteract)
	{
		bDebugTriggerInteract = false;
		if (APlayerCharacter* OwnerChar = Cast<APlayerCharacter>(GetOwner()))
		{
			OwnerChar->DoInteract();
		}
		else
		{
			TryInteract();
		}
	}
#endif
}

void UInteractionComponent::UpdateFocus()
{
	FocusedInteractable = nullptr;

	if (!ViewCamera)
	{
		return;
	}

	// カメラ位置から視線方向へスフィアトレース。ライントレースより半径分だけ判定が緩く、
	// 細いオブジェクトでも狙いやすい
	const FVector Start = ViewCamera->GetComponentLocation();
	const FVector End = Start + ViewCamera->GetForwardVector() * InteractionDistance;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(InteractionTrace), /*bTraceComplex=*/ false, GetOwner());

	FHitResult Hit;
	const bool bHit = GetWorld()->SweepSingleByChannel(
		Hit, Start, End, FQuat::Identity, ECC_Visibility,
		FCollisionShape::MakeSphere(InteractionRadius), Params);

	if (bHit)
	{
		AActor* HitActor = Hit.GetActor();

		// Implements<UInteractable>() は C++ 実装・BP 実装の両方を検出できる
		if (HitActor && HitActor->Implements<UInteractable>())
		{
			FocusedInteractable = HitActor;
		}
	}
}

void UInteractionComponent::TryInteract()
{
	if (FocusedInteractable)
	{
		// BP実装にも届くよう、インターフェース関数は必ず Execute_ 経由で呼ぶ
		IInteractable::Execute_Interact(FocusedInteractable, Cast<APlayerCharacter>(GetOwner()));
	}
}

FText UInteractionComponent::GetInteractionPrompt() const
{
	if (FocusedInteractable)
	{
		return IInteractable::Execute_GetInteractionPrompt(FocusedInteractable);
	}
	return FText::GetEmpty();
}
