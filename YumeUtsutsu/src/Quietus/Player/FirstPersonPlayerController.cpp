
#include "FirstPersonPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Player/CameraManager.h"
#include "Blueprint/UserWidget.h"
#include "Quietus.h"
#include "UI/EndScreenWidget.h"
#include "Core/RunGameInstance.h"
#include "Core/RunGameState.h"
#include "UI/HUDWidget.h"
#include "UI/ModalOverlayWidget.h"
#include "UI/NoteWidget.h"
#include "UI/SeedOverlayWidget.h"
#include "Widgets/Input/SVirtualJoystick.h"
#include "EngineUtils.h"
#include "Interactables/AlarmClock.h"
#include "Interactables/Balloon.h"
#include "Interactables/Door.h"
#include "Interactables/Interactable.h"
#include "Interactables/Battery.h"
#include "Interactables/Cabinet.h"
#include "Interactables/Locker.h"
#include "Interactables/NoteItem.h"
#include "Interactables/KeyItem.h"
#include "Interactables/Throwable.h"
#include "Player/PlayerCharacter.h"
#include "Player/FlashlightComponent.h"
#include "Player/KillCamComponent.h"
#include "Enemies/KillSequence.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/SpotLightComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Enemies/EnemyAnimInstance.h"
#include "Enemies/EnemyCharacter.h"
#include "Enemies/EnemyController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Interactables/DreamShard.h"
#include "Interactables/ExitDoor.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"

AFirstPersonPlayerController::AFirstPersonPlayerController()
{
	// set the player camera manager class
	PlayerCameraManagerClass = ACameraManager::StaticClass();

	KillCam = CreateDefaultSubobject<UKillCamComponent>(TEXT("KillCam"));
}

void AFirstPersonPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// spawn the in-game HUD
	if (IsLocalPlayerController() && HUDWidgetClass)
	{
		HUDWidget = CreateWidget<UHUDWidget>(this, HUDWidgetClass);
		if (HUDWidget)
		{
			HUDWidget->AddToPlayerScreen(0);
		}
		else
		{
			UE_LOG(LogQuietus, Error, TEXT("Could not spawn HUD widget."));
		}
	}

	if (IsLocalPlayerController())
	{
		FadeInContinuedDream();
		ShowRunOverlays();
	}

	// only spawn touch controls on local player controllers
	if (IsLocalPlayerController() && ShouldUseTouchControls())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogQuietus, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}

	// 勝敗の確定は GameState からの通知で受け取る（毎フレームの問い合わせはしない）
	if (IsLocalPlayerController())
	{
		if (ARunGameState* GameState = ARunGameState::Get(this))
		{
			GameState->OnRoundFinished.AddDynamic(this, &AFirstPersonPlayerController::HandleRoundFinished);
			GameState->OnLifeLost.AddDynamic(this, &AFirstPersonPlayerController::HandleLifeLost);
		}
	}
}

void AFirstPersonPlayerController::ShowRunOverlays()
{
	URunGameInstance* GI = GetGameInstance<URunGameInstance>();
	if (!GI)
	{
		return;
	}

#if !UE_BUILD_SHIPPING
	// シードが記録されているラン（＝生成レベル）なら右上に表示する。開発用なので製品版と紹介動画（-NoSeedOverlay）には出さない
	if (GI->GetCurrentRunSeed() > 0 && SeedOverlayWidgetClass && !FParse::Param(FCommandLine::Get(), TEXT("NoSeedOverlay")))
	{
		if (UUserWidget* SeedOverlay = CreateWidget<USeedOverlayWidget>(this, SeedOverlayWidgetClass))
		{
			SeedOverlay->AddToViewport(10);
		}
	}
#endif

	// メニューから開始したランは、生成完了の合図としてキー入力待ちを挟む
	if (GI->ConsumeRunIntroFlag())
	{
		UUserWidget* Intro = RunIntroWidgetClass ? CreateWidget<UModalOverlayWidget>(this, RunIntroWidgetClass) : nullptr;
		if (Intro)
		{
			Intro->AddToViewport(20);
		}
		else
		{
			UE_LOG(LogQuietus, Error, TEXT("Could not spawn run intro widget."));
		}
	}
}

void AFirstPersonPlayerController::FadeInContinuedDream()
{
	URunGameInstance* GI = GetGameInstance<URunGameInstance>();
	if (!GI || !GI->ConsumeDreamFadeInFlag() || !PlayerCameraManager)
	{
		return;
	}
	// 捕まった夢の暗転から続けて、真っ暗な状態から明ける
	PlayerCameraManager->SetManualCameraFade(1.0f, FLinearColor::Black, false);
	GetWorldTimerManager().SetTimer(DreamFadeInTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		if (PlayerCameraManager)
		{
			PlayerCameraManager->StartCameraFade(1.0f, 0.0f, DreamFadeInTime, FLinearColor::Black, false, false);
		}
	}), FMath::Max(DreamFadeInDelay, 0.01f), false);
}

void AFirstPersonPlayerController::ShowNote(const FText& Title, const FText& Body)
{
	UNoteWidget* Note = NoteWidgetClass ? CreateWidget<UNoteWidget>(this, NoteWidgetClass) : nullptr;
	if (!Note)
	{
		UE_LOG(LogQuietus, Error, TEXT("Could not spawn note widget."));
		return;
	}
	Note->SetNote(Title, Body);
	Note->AddToViewport(30);
}

void AFirstPersonPlayerController::HandleRoundFinished(bool bEscaped)
{
	if (bEscaped && GetGameInstance<URunGameInstance>())
	{
		WakeUp();
		return;
	}

	// 捕まって終わったときは捕獲演出を見せてから終了画面へ
	if (!bEscaped && !EndScreenWidget)
	{
		SetIgnoreMoveInput(true);
		SetIgnoreLookInput(true);
		if (StartKillCam([this]() { ShowEndScreen(false); }))
		{
			return;
		}
	}
	ShowEndScreen(bEscaped);
}

void AFirstPersonPlayerController::WakeUp()
{
	SetIgnoreMoveInput(true);
	SetIgnoreLookInput(true);
	if (HUDWidget)
	{
		HUDWidget->SetVisibility(ESlateVisibility::Hidden);
	}
	if (PlayerCameraManager)
	{
		PlayerCameraManager->StartCameraFade(0.0f, 1.0f, WakeFadeTime, FLinearColor::Black, false, /*bHoldWhenFinished=*/ true);
	}
	GetWorldTimerManager().SetTimer(WakeTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		if (URunGameInstance* GI = GetGameInstance<URunGameInstance>())
		{
			GI->WakeUp();
		}
	}), FMath::Max(WakeFadeTime, 0.01f), false);
}

void AFirstPersonPlayerController::ShowEndScreen(bool bEscaped)
{
	// 一度だけ終了画面へ切り替える
	if (EndScreenWidget)
	{
		return;
	}

	if (HUDWidget)
	{
		HUDWidget->RemoveFromParent();
		HUDWidget = nullptr;
	}

	EndScreenWidget = CreateWidget<UEndScreenWidget>(this, EndScreenWidgetClass);
	if (!EndScreenWidget)
	{
		UE_LOG(LogQuietus, Error, TEXT("Could not spawn end screen widget."));
		return;
	}
	EndScreenWidget->SetResult(bEscaped);
	EndScreenWidget->AddToPlayerScreen(1);

	// マウスでボタンを押せるようにし、ゲームは停止する
	bShowMouseCursor = true;
	SetInputMode(FInputModeUIOnly());
	SetPause(true);
}

void AFirstPersonPlayerController::HandleLifeLost(int32 LivesRemaining)
{
	// 操作を止めて暗転し、残りの命を見せてから次の夢へ
	SetIgnoreMoveInput(true);
	SetIgnoreLookInput(true);

	// 捕獲演出がある敵なら、刃が届いて暗転し切ってから残りの命を出す
	if (StartKillCam([this, LivesRemaining]()
		{
			if (HUDWidget)
			{
				HUDWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
				HUDWidget->ShowLifeLost(LivesRemaining);
			}
			GetWorldTimerManager().SetTimer(DreamResetTimer, this, &AFirstPersonPlayerController::ResetDream, FMath::Max(LifeLostHoldTime, 0.01f), false);
		}))
	{
		return;
	}

	if (PlayerCameraManager)
	{
		PlayerCameraManager->StartCameraFade(0.0f, 1.0f, CaughtFadeTime, FLinearColor::Black, false, /*bHoldWhenFinished=*/ true);
	}
	if (HUDWidget)
	{
		HUDWidget->ShowLifeLost(LivesRemaining);
	}
	GetWorldTimerManager().SetTimer(DreamResetTimer, this, &AFirstPersonPlayerController::ResetDream, DreamResetDelay, false);
}

void AFirstPersonPlayerController::ResetDream()
{
	if (URunGameInstance* GI = GetGameInstance<URunGameInstance>())
	{
		GI->ContinueDream();
	}
}

namespace
{
	template <typename T, typename TPred>
	T* FindNearest(const UWorld* World, const FVector& From, TPred Pred)
	{
		T* Best = nullptr;
		double BestDist = TNumericLimits<double>::Max();
		for (TActorIterator<T> It(World); It; ++It)
		{
			if (!Pred(*It))
			{
				continue;
			}
			const double Dist = FVector::DistSquared(It->GetActorLocation(), From);
			if (Dist < BestDist)
			{
				BestDist = Dist;
				Best = *It;
			}
		}
		return Best;
	}

	/** Target の前方 Distance に立って Target を向く */
	void StandFacing(APawn* Pawn, AController* Controller, const FVector& Target, const FVector& Away, float Distance)
	{
		const FVector Location = Target + Away.GetSafeNormal2D() * Distance + FVector(0.0f, 0.0f, Pawn->GetSimpleCollisionHalfHeight());
		const FRotator Rotation = (Target + FVector(0.0f, 0.0f, 120.0f) - Location).Rotation();
		Pawn->TeleportTo(Location, FRotator(0.0f, Rotation.Yaw, 0.0f));
		Controller->SetControlRotation(FRotator(Rotation.Pitch, Rotation.Yaw, 0.0f));
	}

#if !UE_BUILD_SHIPPING
	/** 確認用コマンドの Kind からいちばん近い相手を探す */
	AActor* FindSimTarget(const UWorld* World, const FVector& From, const FString& Kind)
	{
		if (Kind == TEXT("Door"))
		{
			return FindNearest<ADoor>(World, From, [](const ADoor*) { return true; });
		}
		if (Kind == TEXT("Locker"))
		{
			return FindNearest<ALocker>(World, From, [](const ALocker*) { return true; });
		}
		if (Kind == TEXT("Cabinet"))
		{
			return FindNearest<ACabinet>(World, From, [](const ACabinet*) { return true; });
		}
		if (Kind == TEXT("Clock"))
		{
			return FindNearest<AAlarmClock>(World, From, [](const AAlarmClock*) { return true; });
		}
		if (Kind == TEXT("Shard"))
		{
			return FindNearest<ADreamShard>(World, From, [](const ADreamShard*) { return true; });
		}
		if (Kind == TEXT("Balloon"))
		{
			return FindNearest<ABalloon>(World, From, [](const ABalloon*) { return true; });
		}
		if (Kind == TEXT("Exit"))
		{
			return FindNearest<AExitDoor>(World, From, [](const AExitDoor*) { return true; });
		}
		// 生成器が置いたメモ（SimPickup Note は本文のない新品を出すので、書体の確認にはこちらを使う）
		if (Kind == TEXT("Note"))
		{
			return FindNearest<ANoteItem>(World, From, [](const ANoteItem*) { return true; });
		}
		if (Kind == TEXT("Key"))
		{
			return FindNearest<AKeyItem>(World, From, [](const AKeyItem*) { return true; });
		}
		if (Kind == TEXT("Throwable"))
		{
			return FindNearest<AThrowable>(World, From, [](const AThrowable*) { return true; });
		}
		return nullptr;
	}
#endif
}

bool AFirstPersonPlayerController::StartKillCam(TFunction<void()> OnBlackout)
{
	APlayerCharacter* Victim = Cast<APlayerCharacter>(GetPawn());
	if (!Victim || !KillCam)
	{
		return false;
	}
	// 捕まえたのはいちばん近い敵（ロッカーをこじ開けた敵も含む）
	const FVector From = Victim->GetActorLocation();
	const float Range = KillCamRange;
	AEnemyCharacter* Killer = FindNearest<AEnemyCharacter>(GetWorld(), From,
		[From, Range](const AEnemyCharacter* Enemy) { return FVector::Dist(Enemy->GetActorLocation(), From) <= Range; });
	const UKillSequence* Sequence = Killer ? Killer->GetKillSequence() : nullptr;
	if (!Sequence || !Sequence->IsPlayable())
	{
		return false;
	}

	// ロッカーの中で捕まったら扉の前へ引きずり出し、敵はさらにその外側に立たせる（中だと扉と壁で何も見えない）
	FVector FromDirection = FVector::ZeroVector;
	if (ALocker* Locker = Cast<ALocker>(Victim->GetHiddenLocker()))
	{
		FromDirection = Locker->GetActorForwardVector();
		Locker->DragOut();
	}
	if (!KillCam->Play(Killer, Victim, FromDirection))
	{
		return false;
	}

	// 演出の間は HUD を隠す（暗転後に必要なら呼び出し側で戻す）
	if (HUDWidget)
	{
		HUDWidget->SetVisibility(ESlateVisibility::Hidden);
	}
	KillCam->OnBlackout = FSimpleDelegate::CreateWeakLambda(this, [Callback = MoveTemp(OnBlackout)]() { Callback(); });
	return true;
}

void AFirstPersonPlayerController::SimSlot(int32 Slot)
{
#if !UE_BUILD_SHIPPING
	if (APlayerCharacter* MyCharacter = Cast<APlayerCharacter>(GetPawn()))
	{
		MyCharacter->DoSelectItemSlot(Slot);
	}
#endif
}

void AFirstPersonPlayerController::SimUseItem()
{
#if !UE_BUILD_SHIPPING
	if (APlayerCharacter* MyCharacter = Cast<APlayerCharacter>(GetPawn()))
	{
		MyCharacter->DoUseItem();
	}
#endif
}

void AFirstPersonPlayerController::SimCloseDoor()
{
#if !UE_BUILD_SHIPPING
	APlayerCharacter* MyCharacter = Cast<APlayerCharacter>(GetPawn());
	ADoor* Door = MyCharacter ? FindNearest<ADoor>(GetWorld(), MyCharacter->GetActorLocation(), [](const ADoor*) { return true; }) : nullptr;
	if (Door && !Door->IsClosed())
	{
		IInteractable::Execute_Interact(Door, MyCharacter);
	}
	UE_LOG(LogQuietus, Display, TEXT("SimCloseDoor: %s closed=%d"), *GetNameSafe(Door), Door && Door->IsClosed());
#endif
}

void AFirstPersonPlayerController::GotoBalloon()
{
#if !UE_BUILD_SHIPPING
	APawn* MyPawn = GetPawn();
	if (const ABalloon* Balloon = MyPawn ? FindNearest<ABalloon>(GetWorld(), MyPawn->GetActorLocation(), [](const ABalloon*) { return true; }) : nullptr)
	{
		// 壁越しにならない向きを探して、そこから風船を見る
		const FVector Center = Balloon->GetActorLocation();
		FVector Away = FVector::XAxisVector;
		for (int32 i = 0; i < 8; ++i)
		{
			const FVector Dir = FVector::XAxisVector.RotateAngleAxis(45.0f * i, FVector::UpVector);
			FHitResult Hit;
			if (!GetWorld()->LineTraceSingleByChannel(Hit, Center, Center + Dir * 260.0f, ECC_Visibility))
			{
				Away = Dir;
				break;
			}
		}
		StandFacing(MyPawn, this, Center - FVector(0.0f, 0.0f, 40.0f), Away, 220.0f);
	}
#endif
}

void AFirstPersonPlayerController::PopBalloon()
{
#if !UE_BUILD_SHIPPING
	APawn* MyPawn = GetPawn();
	if (ABalloon* Balloon = MyPawn ? FindNearest<ABalloon>(GetWorld(), MyPawn->GetActorLocation(), [](const ABalloon*) { return true; }) : nullptr)
	{
		Balloon->Pop(MyPawn);
	}
#endif
}

void AFirstPersonPlayerController::GotoClown(float Distance, float Yaw)
{
#if !UE_BUILD_SHIPPING
	APawn* MyPawn = GetPawn();
	if (const AEnemyCharacter* Enemy = MyPawn ? FindNearest<AEnemyCharacter>(GetWorld(), MyPawn->GetActorLocation(), [](const AEnemyCharacter*) { return true; }) : nullptr)
	{
		// 指定の向きから順に、敵との間に壁がなく足元に床がある位置を探す（迷路の壁の向こうへ出ないように）
		const FVector Feet = Enemy->GetActorLocation() - FVector(0.0f, 0.0f, Enemy->GetSimpleCollisionHalfHeight());
		const FVector Eye = Feet + FVector(0.0f, 0.0f, 150.0f);
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(Enemy);
		Params.AddIgnoredActor(MyPawn);
		FVector Away = Enemy->GetActorForwardVector().RotateAngleAxis(Yaw, FVector::UpVector);
		for (int32 i = 0; i < 12; ++i)
		{
			// 0, +20, -20, +40, ... の順（視界に入りやすい正面寄りから試す）
			const float Offset = (i % 2 == 0 ? 1.0f : -1.0f) * 20.0f * ((i + 1) / 2);
			const FVector Dir = Enemy->GetActorForwardVector().RotateAngleAxis(Yaw + Offset, FVector::UpVector);
			const FVector Spot = Feet + Dir.GetSafeNormal2D() * Distance;
			FHitResult Hit;
			const bool bBlocked = GetWorld()->LineTraceSingleByChannel(Hit, Eye, Spot + FVector(0.0f, 0.0f, 150.0f), ECC_Visibility, Params);
			const bool bFloor = GetWorld()->LineTraceSingleByChannel(Hit, Spot + FVector(0.0f, 0.0f, 100.0f), Spot - FVector(0.0f, 0.0f, 100.0f), ECC_Visibility, Params);
			if (!bBlocked && bFloor)
			{
				Away = Dir;
				break;
			}
		}
		StandFacing(MyPawn, this, Feet, Away, Distance);
	}
#endif
}

void AFirstPersonPlayerController::WatchClown(float Distance, float Yaw, float Height, float LookHeight)
{
#if !UE_BUILD_SHIPPING
	if (Distance <= 0.0f)
	{
		SetViewTarget(GetPawn());
		return;
	}
	APawn* MyPawn = GetPawn();
	AEnemyCharacter* Enemy = MyPawn ? FindNearest<AEnemyCharacter>(GetWorld(), MyPawn->GetActorLocation(), [](const AEnemyCharacter*) { return true; }) : nullptr;
	if (!Enemy)
	{
		return;
	}

	// 敵に付けたカメラ。敵が向きを変えても同じ角度から見える
	ACameraActor* Watcher = GetWorld()->SpawnActor<ACameraActor>();
	Watcher->GetCameraComponent()->SetFieldOfView(70.0f);
	Watcher->GetCameraComponent()->bConstrainAspectRatio = false;
	Watcher->AttachToActor(Enemy, FAttachmentTransformRules::KeepRelativeTransform);
	const FVector Offset = FVector::XAxisVector.RotateAngleAxis(Yaw, FVector::UpVector) * Distance + FVector(0.0f, 0.0f, Height - Enemy->GetSimpleCollisionHalfHeight());
	const FVector LookAt(0.0f, 0.0f, LookHeight - Enemy->GetSimpleCollisionHalfHeight());
	Watcher->SetActorRelativeLocation(Offset);
	Watcher->SetActorRelativeRotation((LookAt - Offset).Rotation());

	// プレイヤーの懐中電灯と同じ灯りで照らす
	if (const APlayerCharacter* MyCharacter = Cast<APlayerCharacter>(MyPawn))
	{
		if (const UFlashlightComponent* Flashlight = MyCharacter->GetFlashlight())
		{
			USpotLightComponent* Light = NewObject<USpotLightComponent>(Watcher);
			Light->SetupAttachment(Watcher->GetRootComponent());
			Light->SetRelativeLocation(FVector(0.0f, 15.0f, -10.0f));
			Light->RegisterComponent();
			Light->SetIntensityUnits(Flashlight->IntensityUnits);
			Light->SetUseInverseSquaredFalloff(Flashlight->bUseInverseSquaredFalloff);
			Light->SetLightFalloffExponent(Flashlight->LightFalloffExponent);
			Light->SetIntensity(Flashlight->Intensity);
			Light->SetInnerConeAngle(Flashlight->InnerConeAngle);
			Light->SetOuterConeAngle(Flashlight->OuterConeAngle);
			Light->SetAttenuationRadius(Flashlight->AttenuationRadius);
			Light->SetLightColor(Flashlight->GetLightColor());
		}
	}
	SetViewTarget(Watcher);
#endif
}

void AFirstPersonPlayerController::PoseClowns(float Speed)
{
#if !UE_BUILD_SHIPPING
	UEnemyAnimInstance::DebugSpeedOverride = Speed;
	for (TActorIterator<AEnemyCharacter> It(GetWorld()); It; ++It)
	{
		AAIController* AI = Cast<AAIController>(It->GetController());
		UBrainComponent* Brain = AI ? AI->GetBrainComponent() : nullptr;
		if (Speed >= 0.0f)
		{
			if (AI)
			{
				AI->StopMovement();
			}
			if (Brain)
			{
				Brain->PauseLogic(TEXT("PoseClowns"));
			}
			It->GetCharacterMovement()->DisableMovement();
		}
		else
		{
			It->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
			if (Brain)
			{
				Brain->ResumeLogic(TEXT("PoseClowns"));
			}
		}
	}
#endif
}

void AFirstPersonPlayerController::SimMove(int32 Mode, float Seconds)
{
#if !UE_BUILD_SHIPPING
	if (APlayerCharacter* MyCharacter = Cast<APlayerCharacter>(GetPawn()))
	{
		MyCharacter->DebugMove(Mode, Seconds);
	}
#endif
}

void AFirstPersonPlayerController::SimFlashlight()
{
#if !UE_BUILD_SHIPPING
	if (const APlayerCharacter* MyCharacter = Cast<APlayerCharacter>(GetPawn()))
	{
		MyCharacter->GetFlashlight()->Toggle();
	}
#endif
}

void AFirstPersonPlayerController::SimPickup(const FString& Kind)
{
#if !UE_BUILD_SHIPPING
	APlayerCharacter* MyCharacter = Cast<APlayerCharacter>(GetPawn());
	const TCHAR* Path = Kind == TEXT("Battery") ? TEXT("/Game/Interactables/BP_Battery.BP_Battery_C")
		: Kind == TEXT("Note") ? TEXT("/Game/Interactables/BP_NoteItem.BP_NoteItem_C")
		: Kind == TEXT("Key") ? TEXT("/Game/Interactables/BP_KeyItem.BP_KeyItem_C")
		: TEXT("/Game/Interactables/BP_Throwable.BP_Throwable_C");
	UClass* Class = LoadClass<AActor>(nullptr, Path);
	if (!MyCharacter || !Class)
	{
		return;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AActor* Item = GetWorld()->SpawnActor<AActor>(Class, MyCharacter->GetActorLocation() + MyCharacter->GetActorForwardVector() * 60.0f, FRotator::ZeroRotator, Params))
	{
		IInteractable::Execute_Interact(Item, MyCharacter);
	}
#endif
}

void AFirstPersonPlayerController::SimInteract(const FString& Kind)
{
#if !UE_BUILD_SHIPPING
	APlayerCharacter* MyCharacter = Cast<APlayerCharacter>(GetPawn());
	if (!MyCharacter)
	{
		return;
	}
	if (AActor* Target = FindSimTarget(GetWorld(), MyCharacter->GetActorLocation(), Kind))
	{
		IInteractable::Execute_Interact(Target, MyCharacter);
	}
#endif
}

void AFirstPersonPlayerController::SimWalkTo(const FString& Kind, int32 Mode, float StopDistance)
{
#if !UE_BUILD_SHIPPING
	APlayerCharacter* MyCharacter = Cast<APlayerCharacter>(GetPawn());
	const AActor* Target = MyCharacter ? FindSimTarget(GetWorld(), MyCharacter->GetActorLocation(), Kind) : nullptr;
	UNavigationSystemV1* Nav = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!Target || !Nav)
	{
		UE_LOG(LogQuietus, Warning, TEXT("SimWalkTo: %s not found"), *Kind);
		return;
	}

	// 風船のように宙に浮くものもあるので、ナビ上の点に落としてから経路を引く
	const FVector Goal = Target->GetActorLocation();
	FNavLocation OnNav;
	const bool bProjected = Nav->ProjectPointToNavigation(Goal, OnNav, FVector(250.0f, 250.0f, 500.0f));
	const UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(GetWorld(), MyCharacter->GetActorLocation(), bProjected ? OnNav.Location : Goal, MyCharacter);
	if (!Path || !Path->IsValid() || Path->PathPoints.Num() < 2)
	{
		UE_LOG(LogQuietus, Warning, TEXT("SimWalkTo: no path to %s"), *Kind);
		return;
	}
	UE_LOG(LogQuietus, Display, TEXT("SimWalkTo: %s %.0f cm, %d points, partial=%d"), *Kind, Path->GetPathLength(), Path->PathPoints.Num(), Path->IsPartial() ? 1 : 0);

	// 終点から StopDistance ぶん手前で止まる
	TArray<FVector> Points = Path->PathPoints;
	float Remaining = StopDistance;
	while (Points.Num() >= 2)
	{
		const FVector Last = Points.Last();
		const FVector Prev = Points[Points.Num() - 2];
		const float Length = FVector::Dist2D(Last, Prev);
		if (Length > Remaining)
		{
			Points.Last() = Last + (Prev - Last).GetSafeNormal2D() * Remaining;
			break;
		}
		Remaining -= Length;
		Points.Pop();
	}
	MyCharacter->DebugWalkPath(Points, Mode, Goal);
#endif
}

void AFirstPersonPlayerController::SimLook(float Yaw, float Pitch, float Speed)
{
#if !UE_BUILD_SHIPPING
	if (APlayerCharacter* MyCharacter = Cast<APlayerCharacter>(GetPawn()))
	{
		MyCharacter->DebugLook(FRotator(Pitch, GetControlRotation().Yaw + Yaw, 0.0f), Speed);
	}
#endif
}

void AFirstPersonPlayerController::GotoObject(const FString& Kind, float Distance, float LookHeight)
{
#if !UE_BUILD_SHIPPING
	APawn* MyPawn = GetPawn();
	const AActor* Target = MyPawn ? FindSimTarget(GetWorld(), MyPawn->GetActorLocation(), Kind) : nullptr;
	if (!Target)
	{
		UE_LOG(LogQuietus, Warning, TEXT("GotoObject: %s not found"), *Kind);
		return;
	}

	// 相手との間に壁がなく、足元に床がある向きを正面から順に探す。扉の正面は通り抜ける向き（ローカル Y）
	const FVector Center = Target->GetActorLocation() + FVector(0.0f, 0.0f, LookHeight);
	const FVector Front = Target->IsA<ADoor>() ? Target->GetActorRightVector() : Target->GetActorForwardVector();
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Target);
	Params.AddIgnoredActor(MyPawn);
	for (const float Scale : { 1.0f, 0.8f, 0.6f, 0.45f, 0.3f })
	{
		for (int32 i = 0; i < 16; ++i)
		{
			const float Offset = (i % 2 == 0 ? 1.0f : -1.0f) * 22.5f * ((i + 1) / 2);
			const FVector Dir = Front.RotateAngleAxis(Offset, FVector::UpVector).GetSafeNormal2D();
			const FVector Spot = Center + Dir * Distance * Scale;
			// 壁際の物（ロッカーなど）は中心が壁に掛かるので、少し手前から見通しを調べる
			const FVector From = Center + Dir * FMath::Min(70.0f, Distance * Scale * 0.5f);
			FHitResult Hit;
			if (GetWorld()->LineTraceSingleByChannel(Hit, From, Spot, ECC_Visibility, Params))
			{
				continue;
			}
			if (!GetWorld()->LineTraceSingleByChannel(Hit, Spot, Spot - FVector(0.0f, 0.0f, 400.0f), ECC_Visibility, Params))
			{
				continue;
			}
			const FVector Location = Hit.Location + FVector(0.0f, 0.0f, MyPawn->GetSimpleCollisionHalfHeight() + 2.0f);
			const FRotator Rotation = (Center - (Location + FVector(0.0f, 0.0f, MyPawn->BaseEyeHeight))).Rotation();
			MyPawn->TeleportTo(Location, FRotator(0.0f, Rotation.Yaw, 0.0f));
			SetControlRotation(FRotator(Rotation.Pitch, Rotation.Yaw, 0.0f));
			UE_LOG(LogQuietus, Display, TEXT("GotoObject: %s at %.0f cm"), *Kind, Distance * Scale);
			return;
		}
	}
	UE_LOG(LogQuietus, Warning, TEXT("GotoObject: no spot around %s"), *Kind);
#endif
}

void AFirstPersonPlayerController::SummonClown(float Distance)
{
#if !UE_BUILD_SHIPPING
	APlayerCharacter* MyCharacter = Cast<APlayerCharacter>(GetPawn());
	AEnemyCharacter* Enemy = MyCharacter ? FindNearest<AEnemyCharacter>(GetWorld(), MyCharacter->GetActorLocation(), [](const AEnemyCharacter*) { return true; }) : nullptr;
	if (!Enemy)
	{
		return;
	}

	// 見通しの長い方向を探す（今の向きに近いほど優先）
	const FVector Eye = MyCharacter->GetPawnViewLocation();
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(MyCharacter);
	Params.AddIgnoredActor(Enemy);
	FVector BestDir = MyCharacter->GetActorForwardVector();
	float BestFree = 0.0f;
	float BestScore = -1.0f;
	for (int32 i = 0; i < 16; ++i)
	{
		const float Angle = 22.5f * i - (i > 8 ? 360.0f : 0.0f);
		const FVector Dir = MyCharacter->GetActorForwardVector().RotateAngleAxis(Angle, FVector::UpVector).GetSafeNormal2D();
		FHitResult Hit;
		const float Free = GetWorld()->LineTraceSingleByChannel(Hit, Eye, Eye + Dir * Distance, ECC_Visibility, Params) ? Hit.Distance : Distance;
		const float Score = Free - FMath::Abs(Angle) * 1.5f;
		if (Score > BestScore)
		{
			BestScore = Score;
			BestDir = Dir;
			BestFree = Free;
		}
	}

	// 見通しの先に立たせ、こちらを向かせて追跡させる
	FVector Spot = MyCharacter->GetActorLocation() + BestDir * FMath::Max(BestFree - 100.0f, 200.0f);
	if (const UNavigationSystemV1* Nav = UNavigationSystemV1::GetCurrent(GetWorld()))
	{
		FNavLocation OnNav;
		if (Nav->ProjectPointToNavigation(Spot, OnNav, FVector(150.0f, 150.0f, 300.0f)))
		{
			Spot = OnNav.Location + FVector(0.0f, 0.0f, Enemy->GetSimpleCollisionHalfHeight());
		}
	}
	PoseClowns(-1.0f);
	Enemy->TeleportTo(Spot, (-BestDir).Rotation());
	MyCharacter->DebugLook(FRotator(-3.0f, BestDir.Rotation().Yaw, 0.0f), 4.0f);

	if (AAIController* AI = Cast<AAIController>(Enemy->GetController()))
	{
		if (UBlackboardComponent* BB = AI->GetBlackboardComponent())
		{
			BB->SetValueAsObject(EnemyKeys::TargetActor, MyCharacter);
			BB->SetValueAsBool(EnemyKeys::SeeingTarget, true);
			BB->SetValueAsVector(EnemyKeys::LastKnownLocation, MyCharacter->GetActorLocation());
		}
	}
#endif
}

void AFirstPersonPlayerController::SimBustLocker()
{
#if !UE_BUILD_SHIPPING
	const APawn* MyPawn = GetPawn();
	if (ALocker* Locker = MyPawn ? FindNearest<ALocker>(GetWorld(), MyPawn->GetActorLocation(), [](const ALocker*) { return true; }) : nullptr)
	{
		Locker->BustOpen();
	}
#endif
}

void AFirstPersonPlayerController::SimCaught(float Distance)
{
#if !UE_BUILD_SHIPPING
	GotoClown(Distance, 0.0f);
	if (ARunGameState* GameState = ARunGameState::Get(this))
	{
		GameState->TriggerGameOver();
	}
#endif
}

void AFirstPersonPlayerController::SimThrow()
{
#if !UE_BUILD_SHIPPING
	APawn* MyPawn = GetPawn();
	UClass* Class = LoadClass<AThrowable>(nullptr, TEXT("/Game/Interactables/BP_Throwable.BP_Throwable_C"));
	if (!MyPawn || !Class)
	{
		return;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FVector Start = MyPawn->GetPawnViewLocation() + MyPawn->GetActorForwardVector() * 60.0f;
	if (AThrowable* Bottle = GetWorld()->SpawnActor<AThrowable>(Class, Start, FRotator::ZeroRotator, Params))
	{
		Bottle->Launch(MyPawn, MyPawn->GetActorForwardVector() * 500.0f + FVector(0.0f, 0.0f, 150.0f));
	}
#endif
}

void AFirstPersonPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Context
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}
	
}

bool AFirstPersonPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}
