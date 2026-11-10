#include "Player/StaminaComponent.h"

UStaminaComponent::UStaminaComponent()
{
	// 持ち主が Update を呼んで進めるので自前のTickは不要
	PrimaryComponentTick.bCanEverTick = false;
}

void UStaminaComponent::BeginPlay()
{
	Super::BeginPlay();

	// BP側で調整された値を反映するため、初期化はコンストラクタではなくここで行う
	Stamina = MaxStamina;
}

void UStaminaComponent::Update(float DeltaSeconds, bool bDraining)
{
	if (bDraining)
	{
		Stamina = FMath::Max(Stamina - DrainRate * DeltaSeconds, 0.0f);

		// 枯渇したら満タンに回復するまでスプリントをロックする
		if (Stamina <= 0.0f && !bExhausted)
		{
			bExhausted = true;
			OnExhausted.Broadcast();
		}
		return;
	}

	Stamina = FMath::Min(Stamina + RecoveryRate * DeltaSeconds, MaxStamina);
	if (bExhausted && Stamina >= MaxStamina)
	{
		bExhausted = false;
	}
}
