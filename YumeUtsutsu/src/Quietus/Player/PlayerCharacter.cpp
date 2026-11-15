#include "PlayerCharacter.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "InputActionValue.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Interactables/Door.h"
#include "Interactables/Interactable.h"
#include "Player/FlashlightComponent.h"
#include "Player/InteractionComponent.h"
#include "Player/StaminaComponent.h"
#include "Player/ThrowInventoryComponent.h"
#include "Player/ItemSlotsComponent.h"
#include "Player/DoorWarpComponent.h"
#include "Player/InteractOutlineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Quietus.h"
#include "Components/AudioComponent.h"
#include "Core/DreamFeedbackSettings.h"
#include "Core/DreamLightingSettings.h"
#include "Feedback/FeedbackSubsystem.h"

APlayerCharacter::APlayerCharacter()
{
	// スタミナ／ノイズを毎フレーム更新するためTickを有効化
	PrimaryActorTick.bCanEverTick = true;

	// Create the first person mesh that will be viewed only by this character's owner
	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("First Person Mesh"));

	FirstPersonMesh->SetupAttachment(GetMesh());
	FirstPersonMesh->SetOnlyOwnerSee(true);
	FirstPersonMesh->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
	FirstPersonMesh->SetCollisionProfileName(FName("NoCollision"));

	// Create the Camera Component
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("First Person Camera"));
	FirstPersonCameraComponent->SetupAttachment(FirstPersonMesh, FName("head"));
	FirstPersonCameraComponent->SetRelativeLocationAndRotation(FVector(-2.8f, 5.89f, 0.0f), FRotator(0.0f, 90.0f, -90.0f));
	FirstPersonCameraComponent->bUsePawnControlRotation = true;
	FirstPersonCameraComponent->bEnableFirstPersonFieldOfView = true;
	FirstPersonCameraComponent->bEnableFirstPersonScale = true;
	FirstPersonCameraComponent->FirstPersonFieldOfView = 70.0f;
	FirstPersonCameraComponent->FirstPersonScale = 0.6f;

	// 懐中電灯：カメラに取り付けて視線に追従させる。少し右下にオフセットして手持ち感を出す
	// （バッテリー等の中身は UFlashlightComponent 側が持つ）
	Flashlight = CreateDefaultSubobject<UFlashlightComponent>(TEXT("Flashlight"));
	Flashlight->SetupAttachment(FirstPersonCameraComponent);
	Flashlight->SetRelativeLocation(FVector(20.0f, 15.0f, -10.0f));

	DarkFillLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("Dark Fill Light"));
	DarkFillLight->SetupAttachment(FirstPersonCameraComponent);
	DarkFillLight->SetMobility(EComponentMobility::Movable);
	DarkFillLight->SetCastShadows(false);

	ExposureOverride = CreateDefaultSubobject<UPostProcessComponent>(TEXT("Exposure Override"));
	ExposureOverride->SetupAttachment(RootComponent);
	ExposureOverride->bUnbound = true;
	ExposureOverride->Priority = 5.0f;

	// 視線インタラクション（トレース／フォーカス／実行）
	Interaction = CreateDefaultSubobject<UInteractionComponent>(TEXT("Interaction"));

	// スプリント用スタミナ、投擲物の所持と投擲
	StaminaComponent = CreateDefaultSubobject<UStaminaComponent>(TEXT("Stamina"));
	ThrowInventory = CreateDefaultSubobject<UThrowInventoryComponent>(TEXT("ThrowInventory"));

	// アイテム欄と鍵での移動。手に持った物はカメラに付け、一人称の描画（壁にめり込まない）で自分にだけ見せる
	ItemSlots = CreateDefaultSubobject<UItemSlotsComponent>(TEXT("ItemSlots"));
	DoorWarp = CreateDefaultSubobject<UDoorWarpComponent>(TEXT("DoorWarp"));
	InteractOutline = CreateDefaultSubobject<UInteractOutlineComponent>(TEXT("InteractOutline"));
	HeldItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Held Item Mesh"));
	HeldItemMesh->SetupAttachment(FirstPersonCameraComponent);
	HeldItemMesh->SetOnlyOwnerSee(true);
	HeldItemMesh->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
	HeldItemMesh->SetCollisionProfileName(FName("NoCollision"));
	HeldItemMesh->SetCastShadow(false);
	HeldItemMesh->SetVisibility(false);
	// 手元の物は世界の光（チャンネル 0）に加えて、自分専用の光（チャンネル 1）も受ける
	HeldItemMesh->SetLightingChannels(true, true, false);
	HeldItemLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("Held Item Light"));
	HeldItemLight->SetupAttachment(FirstPersonCameraComponent);
	HeldItemLight->SetMobility(EComponentMobility::Movable);
	HeldItemLight->SetCastShadows(false);
	HeldItemLight->SetLightingChannels(false, true, false);

	// configure the character comps
	GetMesh()->SetOwnerNoSee(true);
	GetMesh()->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::WorldSpaceRepresentation;

	GetCapsuleComponent()->SetCapsuleSize(34.0f, 96.0f);

	// Configure character movement
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;
	GetCharacterMovement()->AirControl = 0.5f;

	// 歩行が基本速度（スプリント開始/終了時にDoSprintStart/DoSprintEndで切り替える）
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	// ACharacter組み込みのCrouch()/UnCrouch()を使うため、しゃがみ許可フラグを有効化
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;

	// しゃがみ中の移動速度
	GetCharacterMovement()->MaxWalkSpeedCrouched = 140.0f;
}

void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	// BP側で調整された値を反映するため、初期化はコンストラクタではなくここで行う
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	ItemSlots->SetHeldMesh(HeldItemMesh);

	// 懐中電灯の揺れはスプリント速度で最大になる
	Flashlight->SetBobReferenceSpeed(SprintSpeed);

	// スタミナが尽きたらスプリントを強制終了する
	StaminaComponent->OnExhausted.AddUObject(this, &APlayerCharacter::DoSprintEnd);

	const UDreamFeedbackSettings& Sfx = UDreamFeedbackSettings::Get();
	BreathRunLoop = UFeedbackSubsystem::CreateLoop2D(this, Sfx.BreathRun);
	BreathExhaustedLoop = UFeedbackSubsystem::CreateLoop2D(this, Sfx.BreathExhausted);

	ApplyLightingSettings();
#if WITH_EDITOR
	LightingChangedHandle = GetMutableDefault<UDreamLightingSettings>()->OnSettingChanged().AddUObject(this, &APlayerCharacter::HandleLightingChanged);
#endif
}

void APlayerCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
#if WITH_EDITOR
	GetMutableDefault<UDreamLightingSettings>()->OnSettingChanged().Remove(LightingChangedHandle);
#endif
	Super::EndPlay(EndPlayReason);
}

void APlayerCharacter::ApplyLightingSettings()
{
	const UDreamLightingSettings& S = UDreamLightingSettings::Get();

	Flashlight->ApplyLightingSettings();

	DarkFillLight->SetIntensityUnits(ELightUnits::Unitless);
	DarkFillLight->SetUseInverseSquaredFalloff(false);
	DarkFillLight->SetLightFalloffExponent(S.DarkFillFalloffExponent);
	DarkFillLight->SetIntensity(S.DarkFillIntensity);
	DarkFillLight->SetAttenuationRadius(S.DarkFillRadius);
	DarkFillLight->SetLightColor(S.DarkFillColor);
	DarkFillLight->SetSpecularScale(S.DarkFillSpecular);
	DarkFillLight->SetIndirectLightingIntensity(0.0f);
	DarkFillLight->SetVolumetricScatteringIntensity(0.0f);
	DarkFillLight->SetVisibility(S.bDarkFill);

	HeldItemLight->SetRelativeLocation(S.HeldItemLightOffset);
	HeldItemLight->SetIntensityUnits(ELightUnits::Unitless);
	HeldItemLight->SetUseInverseSquaredFalloff(false);
	HeldItemLight->SetLightFalloffExponent(2.0f);
	HeldItemLight->SetIntensity(S.HeldItemLightIntensity);
	HeldItemLight->SetAttenuationRadius(S.HeldItemLightRadius);
	HeldItemLight->SetLightColor(S.HeldItemLightColor);
	HeldItemLight->SetIndirectLightingIntensity(0.0f);
	HeldItemLight->SetVolumetricScatteringIntensity(0.0f);
	HeldItemLight->SetVisibility(S.bHeldItemLight);

	FPostProcessSettings& PP = ExposureOverride->Settings;
	PP.bOverride_AutoExposureMinBrightness = true;
	PP.AutoExposureMinBrightness = S.ExposureMinEV;
	PP.bOverride_AutoExposureMaxBrightness = true;
	PP.AutoExposureMaxBrightness = S.ExposureMaxEV;
	PP.bOverride_AutoExposureBias = true;
	PP.AutoExposureBias = S.ExposureBias;
	PP.bOverride_AutoExposureSpeedUp = true;
	PP.AutoExposureSpeedUp = S.ExposureSpeedUp;
	PP.bOverride_AutoExposureSpeedDown = true;
	PP.AutoExposureSpeedDown = S.ExposureSpeedDown;
	PP.bOverride_AutoExposureLowPercent = true;
	PP.AutoExposureLowPercent = S.ExposureLowPercent;
	PP.bOverride_AutoExposureHighPercent = true;
	PP.AutoExposureHighPercent = S.ExposureHighPercent;
}

void APlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateStamina(DeltaSeconds);
	UpdateNoise(DeltaSeconds);
	UpdateFootsteps(DeltaSeconds);
	UpdateBreath();

#if !UE_BUILD_SHIPPING
	if (DebugMoveLeft > 0.0f)
	{
		DebugMoveLeft -= DeltaSeconds;
		// 壁に当たって止まったら向きを変えて歩き続ける
		DebugStuckTime = GetVelocity().Size2D() < 30.0f ? DebugStuckTime + DeltaSeconds : 0.0f;
		if (DebugStuckTime > 0.25f && GetController())
		{
			DebugStuckTime = 0.0f;
			const FRotator Turned(0.0f, GetActorRotation().Yaw + (FMath::RandBool() ? 90.0f : -90.0f), 0.0f);
			SetActorRotation(Turned);
			GetController()->SetControlRotation(Turned);
		}
		AddMovementInput(GetActorForwardVector(), 1.0f);
		if (DebugMoveLeft <= 0.0f)
		{
			DoSprintEnd();
			UnCrouch();
		}
	}
	TickDebugPath(DeltaSeconds);
#endif

#if WITH_EDITORONLY_DATA
	// 自動テスト用フック（エディタ専用）。外部ツールからフラグを立てると投擲を実行する
	if (bDebugThrow)
	{
		bDebugThrow = false;
		ThrowInventory->Throw(FirstPersonCameraComponent->GetComponentLocation(), FirstPersonCameraComponent->GetForwardVector());
		ItemSlots->RefreshHeld();
	}
#endif
}

void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &APlayerCharacter::DoJumpStart);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &APlayerCharacter::DoJumpEnd);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlayerCharacter::MoveInput);

		// Looking/Aiming
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &APlayerCharacter::LookInput);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &APlayerCharacter::LookInput);

		// スプリント（ホールド式：押した瞬間と離した瞬間をバインド）
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &APlayerCharacter::DoSprintStart);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &APlayerCharacter::DoSprintEnd);

		// しゃがみ（トグル式）
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this, &APlayerCharacter::DoCrouchToggle);

		// 懐中電灯（トグル式）
		EnhancedInputComponent->BindAction(FlashlightAction, ETriggerEvent::Started, this, &APlayerCharacter::DoFlashlightToggle);

		// インタラクト（単発）
		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &APlayerCharacter::DoInteract);

		// アイテム欄（番号キーで持つ / R で使う）。アセット未割り当てでも起動できるようガード
		if (ItemSlot1Action)
		{
			EnhancedInputComponent->BindAction(ItemSlot1Action, ETriggerEvent::Started, this, &APlayerCharacter::SelectItemSlot1);
		}
		if (ItemSlot2Action)
		{
			EnhancedInputComponent->BindAction(ItemSlot2Action, ETriggerEvent::Started, this, &APlayerCharacter::SelectItemSlot2);
		}
		if (UseItemAction)
		{
			EnhancedInputComponent->BindAction(UseItemAction, ETriggerEvent::Started, this, &APlayerCharacter::DoUseItem);
		}
	}
	else
	{
		UE_LOG(LogQuietus, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}


void APlayerCharacter::MoveInput(const FInputActionValue& Value)
{
	// get the Vector2D move axis
	FVector2D MovementVector = Value.Get<FVector2D>();

	// pass the axis values to the move input
	DoMove(MovementVector.X, MovementVector.Y);

}

void APlayerCharacter::LookInput(const FInputActionValue& Value)
{
	// get the Vector2D look axis
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// pass the axis values to the aim input
	DoAim(LookAxisVector.X, LookAxisVector.Y);

}

void APlayerCharacter::DoAim(float Yaw, float Pitch)
{
	if (GetController())
	{
		// pass the rotation inputs
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void APlayerCharacter::DoMove(float Right, float Forward)
{
	if (GetController())
	{
		// pass the move inputs
		AddMovementInput(GetActorRightVector(), Right);
		AddMovementInput(GetActorForwardVector(), Forward);
	}
}

void APlayerCharacter::DoJumpStart()
{
	// pass Jump to the character
	Jump();
}

void APlayerCharacter::DoJumpEnd()
{
	// pass StopJumping to the character
	StopJumping();
}

void APlayerCharacter::DoSprintStart()
{
	// 枯渇リカバリ中は満タンまで不可。通常時も閾値未満なら開始しない
	if (bIsSprinting || !StaminaComponent->CanStartSprint())
	{
		return;
	}

	// しゃがみ中はスプリント開始で立ち上がる
	if (bIsCrouched)
	{
		UnCrouch();
	}

	bIsSprinting = true;
	GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
}

void APlayerCharacter::DoSprintEnd()
{
	if (!bIsSprinting)
	{
		return;
	}

	bIsSprinting = false;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void APlayerCharacter::DoCrouchToggle()
{
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		// スプリント中にしゃがんだ場合はスプリントを先に終了する
		DoSprintEnd();
		Crouch();
	}
}

void APlayerCharacter::DoFlashlightToggle()
{
	Flashlight->Toggle();
}

void APlayerCharacter::DoInteract()
{
	// ロッカー内では視線トレースが成立しないため、隠れているロッカーへ直接届ける
	if (bHiddenInLocker && HiddenLocker && HiddenLocker->Implements<UInteractable>())
	{
		IInteractable::Execute_Interact(HiddenLocker, this);
		return;
	}
	Interaction->TryInteract();
}

void APlayerCharacter::UpdateStamina(float DeltaSeconds)
{
	// 実際に走っている間だけ消費する（キーを押したまま静止・壁に当たっている場合は消費しない）
	const bool bActuallyRunning = bIsSprinting && GetVelocity().SizeSquared2D() > FMath::Square(WalkSpeed * 0.5f);
	StaminaComponent->Update(DeltaSeconds, bActuallyRunning);
}

void APlayerCharacter::UpdateNoise(float DeltaSeconds)
{
	// NoiseIntervalごとに1回だけノイズイベントを発行する
	NoiseTimer += DeltaSeconds;
	if (NoiseTimer < NoiseInterval)
	{
		return;
	}
	NoiseTimer = 0.0f;

	float Loudness = GetCurrentNoiseLevel();

#if WITH_EDITORONLY_DATA
	// 自動テスト用: 静止したままでもスプリント相当のノイズを発生させる
	if (bDebugEmitNoise)
	{
		Loudness = SprintLoudness;
	}
#endif

	if (Loudness > 0.0f)
	{
		// MakeNoiseはAI Perceptionの聴覚（AISense_Hearing）に伝わる。
		// Loudnessは敵AI側の聴覚半径に掛かる倍率（1.0 = 基準半径そのまま）
		MakeNoise(Loudness, this, GetActorLocation());
	}
}

#if !UE_BUILD_SHIPPING
void APlayerCharacter::DebugMove(int32 Mode, float Seconds)
{
	DoSprintEnd();
	UnCrouch();
	if (Mode == 1)
	{
		DoSprintStart();
	}
	else if (Mode == 2)
	{
		Crouch();
	}
	DebugMoveLeft = Seconds;
}

void APlayerCharacter::DebugWalkPath(const TArray<FVector>& Points, int32 Mode, const FVector& LookAt)
{
	DoSprintEnd();
	UnCrouch();
	if (Mode == 1)
	{
		DoSprintStart();
	}
	else if (Mode == 2)
	{
		Crouch();
	}
	DebugPath = Points;
	DebugPathLookAt = LookAt;
	DebugPathStuckTotal = 0.0f;
	DebugStuckTime = 0.0f;
	DebugLookTarget.Reset();
}

void APlayerCharacter::DebugLook(const FRotator& Rotation, float Speed)
{
	DebugLookTarget = Rotation;
	DebugLookSpeed = Speed;
}

void APlayerCharacter::TickDebugPath(float DeltaSeconds)
{
	AController* MyController = GetController();
	if (!MyController)
	{
		return;
	}

	if (DebugPath.Num() > 0)
	{
		const FVector To = DebugPath[0] - GetActorLocation();
		if (To.SizeSquared2D() < FMath::Square(DebugPath.Num() > 1 ? 80.0f : 30.0f))
		{
			DebugPath.RemoveAt(0);
			if (DebugPath.IsEmpty())
			{
				DoSprintEnd();
				UnCrouch();
				DebugLook((DebugPathLookAt - GetPawnViewLocation()).Rotation(), 3.0f);
			}
		}
		else
		{
			AddMovementInput(To.GetSafeNormal2D(), 1.0f);
			DebugLookTarget.Reset();
			MyController->SetControlRotation(FMath::RInterpTo(MyController->GetControlRotation(), FRotator(0.0f, To.Rotation().Yaw, 0.0f), DeltaSeconds, 2.5f));

			// 閉じた扉に当たったら開ける。開けても進めなければあきらめる
			DebugStuckTime = GetVelocity().Size2D() < 30.0f ? DebugStuckTime + DeltaSeconds : 0.0f;
			DebugPathStuckTotal = GetVelocity().Size2D() < 30.0f ? DebugPathStuckTotal + DeltaSeconds : DebugPathStuckTotal;
			if (DebugStuckTime > 0.4f)
			{
				DebugStuckTime = 0.0f;
				ADoor* Nearest = nullptr;
				for (TActorIterator<ADoor> It(GetWorld()); It; ++It)
				{
					if (FVector::Dist2D(It->GetActorLocation(), GetActorLocation()) < 260.0f)
					{
						Nearest = *It;
						break;
					}
				}
				if (Nearest)
				{
					IInteractable::Execute_Interact(Nearest, this);
				}
			}
			if (DebugPathStuckTotal > 4.0f)
			{
				UE_LOG(LogQuietus, Warning, TEXT("DebugWalkPath: stuck, giving up"));
				DebugPath.Reset();
				DoSprintEnd();
			}
		}
	}

	if (DebugLookTarget.IsSet())
	{
		const FRotator Now = FMath::RInterpTo(MyController->GetControlRotation(), *DebugLookTarget, DeltaSeconds, DebugLookSpeed);
		MyController->SetControlRotation(Now);
		if (Now.Equals(*DebugLookTarget, 0.3f))
		{
			DebugLookTarget.Reset();
		}
	}
}
#endif

void APlayerCharacter::UpdateFootsteps(float DeltaSeconds)
{
	const UDreamFeedbackSettings& Sfx = UDreamFeedbackSettings::Get();
	const float Stride = bIsCrouched ? Sfx.CrouchStride : (bIsSprinting ? Sfx.RunStride : Sfx.WalkStride);

	// AI へのノイズと同じ条件（ロッカー内・ほぼ静止では鳴らない）。空中でも鳴らさない
	if (GetCurrentNoiseLevel() <= 0.0f || !GetCharacterMovement()->IsMovingOnGround())
	{
		// 歩き出してすぐ 1 歩目が鳴るように半歩ぶん進めておく
		StepDistance = Stride * 0.5f;
		return;
	}

	StepDistance += GetVelocity().Size2D() * DeltaSeconds;
	if (StepDistance >= Stride)
	{
		StepDistance = FMath::Fmod(StepDistance, Stride);
		if (bIsCrouched)
		{
			UFeedbackSubsystem::Play2D(this, Sfx.StepCrouch, TEXT("StepCrouch"));
		}
		else if (bIsSprinting)
		{
			UFeedbackSubsystem::Play2D(this, Sfx.StepRun, TEXT("StepRun"));
		}
		else
		{
			UFeedbackSubsystem::Play2D(this, Sfx.StepWalk, TEXT("StepWalk"));
		}
	}
}

void APlayerCharacter::UpdateBreath()
{
	const UDreamFeedbackSettings& Sfx = UDreamFeedbackSettings::Get();
	auto Fade = [&Sfx](UAudioComponent* Loop, bool& bState, bool bWant, const TCHAR* Tag)
	{
		if (!Loop || bWant == bState)
		{
			return;
		}
		bState = bWant;
		if (bWant)
		{
			Loop->FadeIn(Sfx.BreathFadeIn);
		}
		else
		{
			Loop->FadeOut(Sfx.BreathFadeOut, 0.0f);
		}
		UE_LOG(LogSfx, Verbose, TEXT("sfx %s %s"), Tag, bWant ? TEXT("in") : TEXT("out"));
	};

	// 走っている息はスタミナを使っている間だけ。壁で一瞬止まった程度では切らない。息切れは尽きてから回復するまで
	const bool bRunning = bIsSprinting && !bHiddenInLocker && GetVelocity().SizeSquared2D() > FMath::Square(WalkSpeed * 0.5f);
	BreathRunHold = bRunning ? Sfx.BreathRunHold : FMath::Max(BreathRunHold - GetWorld()->GetDeltaSeconds(), 0.0f);
	Fade(BreathRunLoop, bBreathRun, BreathRunHold > 0.0f && !StaminaComponent->IsExhausted(), TEXT("BreathRun"));
	Fade(BreathExhaustedLoop, bBreathExhausted, StaminaComponent->IsExhausted(), TEXT("BreathExhausted"));
}

float APlayerCharacter::GetCurrentNoiseLevel() const
{
	// ロッカー内は無音。ほぼ静止していても無音
	if (bHiddenInLocker || GetVelocity().SizeSquared2D() < FMath::Square(10.0f))
	{
		return 0.0f;
	}

	// しゃがみ < 歩行 < スプリント（しゃがみとスプリントは同時に成立しない）
	if (bIsCrouched)
	{
		return CrouchLoudness;
	}
	if (bIsSprinting)
	{
		return SprintLoudness;
	}
	return WalkLoudness;
}

float APlayerCharacter::GetStamina() const
{
	return StaminaComponent->GetStamina();
}

float APlayerCharacter::GetMaxStamina() const
{
	return StaminaComponent->GetMaxStamina();
}

bool APlayerCharacter::IsStaminaExhausted() const
{
	return StaminaComponent->IsExhausted();
}

float APlayerCharacter::GetBattery() const
{
	return Flashlight->GetBattery();
}

float APlayerCharacter::GetMaxBattery() const
{
	return Flashlight->GetMaxBattery();
}

FText APlayerCharacter::GetInteractionPrompt() const
{
	return Interaction->GetInteractionPrompt();
}

void APlayerCharacter::DoSelectItemSlot(int32 Slot)
{
	ItemSlots->SelectSlot(Slot);
}

void APlayerCharacter::DoUseItem()
{
	ItemSlots->UseSelected(Interaction->GetFocusedInteractable());
}

void APlayerCharacter::AddThrowable(TSubclassOf<AThrowable> PickedClass)
{
	ThrowInventory->Add(PickedClass);
	ItemSlots->RefreshHeld();
}

int32 APlayerCharacter::GetThrowableCount() const
{
	return ThrowInventory->GetCount();
}
