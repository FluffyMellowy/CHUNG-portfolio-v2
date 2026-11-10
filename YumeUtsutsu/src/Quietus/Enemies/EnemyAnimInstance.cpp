#include "Enemies/EnemyAnimInstance.h"
#include "Enemies/EnemyCharacter.h"

#if !UE_BUILD_SHIPPING
float UEnemyAnimInstance::DebugSpeedOverride = -1.0f;
#endif

void UEnemyAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (const APawn* Pawn = TryGetPawnOwner())
	{
		GroundSpeed = Pawn->GetVelocity().Size2D();

		const AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(Pawn);
		WeaponCarryAlpha = Enemy && Enemy->HasWeapon() ? 1.0f : 0.0f;
	}

#if !UE_BUILD_SHIPPING
	if (DebugSpeedOverride >= 0.0f)
	{
		GroundSpeed = DebugSpeedOverride;
	}
#endif
}
