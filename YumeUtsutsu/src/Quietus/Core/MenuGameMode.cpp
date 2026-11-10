#include "MenuGameMode.h"
#include "Blueprint/UserWidget.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Core/GameConstants.h"
#include "Core/RunGameInstance.h"
#include "EngineUtils.h"
#include "Environment/BedroomSet.h"
#include "GameFramework/SpectatorPawn.h"
#include "Kismet/GameplayStatics.h"
#include "Quietus.h"
#include "TimerManager.h"
#include "UI/EndScreenWidget.h"
#include "UI/MainMenuWidget.h"
#include "UI/ThoughtBubbleWidget.h"

AMenuGameMode::AMenuGameMode()
{
	// メニューでは操作対象を出さない
	DefaultPawnClass = ASpectatorPawn::StaticClass();
	bStartPlayersAsSpectators = true;

	// ズーム中だけ動かす
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
}

void AMenuGameMode::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC)
	{
		return;
	}

	for (TActorIterator<ABedroomSet> It(GetWorld()); It; ++It)
	{
		Bedroom = *It;
		break;
	}

	URunGameInstance* GI = GetGameInstance<URunGameInstance>();
	if (GI && GI->ConsumeWakeUpFlag() && Bedroom)
	{
		StartMorning(PC);
		return;
	}
	StartTitle(PC);
}

void AMenuGameMode::ShowScene(APlayerController* PC, ACameraActor* Camera, TFunction<void()> OnShown)
{
	if (Camera)
	{
		PC->SetViewTarget(Camera);
	}

	const float Delay = FMath::Max(FadeInDelay, 0.01f);
	if (PC->PlayerCameraManager)
	{
		PC->PlayerCameraManager->SetManualCameraFade(1.0f, FLinearColor::Black, false);
		TWeakObjectPtr<APlayerController> WeakPC(PC);
		const float Time = FadeInTime;
		GetWorldTimerManager().SetTimer(FadeTimer, FTimerDelegate::CreateWeakLambda(this, [WeakPC, Time]()
		{
			if (WeakPC.IsValid() && WeakPC->PlayerCameraManager)
			{
				WeakPC->PlayerCameraManager->StartCameraFade(1.0f, 0.0f, Time, FLinearColor::Black, false, false);
			}
		}), Delay, false);
	}

	if (OnShown)
	{
		GetWorldTimerManager().SetTimer(ShownTimer, FTimerDelegate::CreateWeakLambda(this, MoveTemp(OnShown)), Delay + FadeInTime, false);
	}
}

void AMenuGameMode::StartTitle(APlayerController* PC)
{
	if (Bedroom)
	{
		Bedroom->ApplyState(EBedroomState::Night);
		ShowScene(PC, Bedroom->GetCamera(EBedroomState::Night), nullptr);
		UE_LOG(LogQuietus, Log, TEXT("Bedroom: title"));

		if (ThoughtBubbleWidgetClass)
		{
			ThoughtBubble = CreateWidget<UThoughtBubbleWidget>(PC, ThoughtBubbleWidgetClass);
			if (ThoughtBubble)
			{
				ThoughtBubble->AddToViewport(0);
			}
		}
	}
	else
	{
		// レベルに演出用カメラがあればそこから背景シーンを映す（無ければスペクテイター視点のまま）
		for (TActorIterator<ACameraActor> It(GetWorld()); It; ++It)
		{
			PC->SetViewTarget(*It);
			break;
		}
	}

	if (!MainMenuWidgetClass)
	{
		UE_LOG(LogQuietus, Error, TEXT("%s: MainMenuWidgetClass is not set"), *GetName());
		return;
	}

	Menu = CreateWidget<UMainMenuWidget>(PC, MainMenuWidgetClass);
	if (Menu)
	{
		// 部屋があれば、はじめからのあとの台詞とズーム、夢の背景での難易度の選択をこちらで進める
		if (Bedroom)
		{
			Menu->OnStartRequested.BindUObject(this, &AMenuGameMode::HandleStartRequested);
			Menu->OnBackRequested.BindUObject(this, &AMenuGameMode::HandleBackRequested);
		}
		Menu->AddToViewport(1);
		FocusMenu(PC);
	}
}

void AMenuGameMode::FocusMenu(APlayerController* PC)
{
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(Menu->TakeWidget());
	PC->SetInputMode(InputMode);
	PC->bShowMouseCursor = true;
}

void AMenuGameMode::StartMorning(APlayerController* PC)
{
	UE_LOG(LogQuietus, Log, TEXT("Bedroom: morning"));
	Bedroom->ApplyState(EBedroomState::Morning);

	// 操作するものが無い場面なのでカーソルは出さない（クリア画面で出す）
	PC->SetInputMode(FInputModeUIOnly());
	PC->bShowMouseCursor = false;

	ShowScene(PC, Bedroom->GetCamera(EBedroomState::Morning), [this]()
	{
		BeginDialogue(EBedroomDialogue::AfterWaking);
	});
}

void AMenuGameMode::HandleStartRequested()
{
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		PC->bShowMouseCursor = false;
	}
	BeginDialogue(EBedroomDialogue::BeforeDream);
}

void AMenuGameMode::HandleBackRequested()
{
	// 夢の背景から部屋へ戻る。ズームで動かしたカメラごと元に戻るよう開き直す
	UGameplayStatics::OpenLevel(this, GameNames::MainMenuLevel);
}

void AMenuGameMode::BeginDialogue(EBedroomDialogue Point)
{
	ActiveDialogue = Point;
	bDialogueActive = true;
	UE_LOG(LogQuietus, Log, TEXT("Bedroom: dialogue %s"), *UEnum::GetValueAsString(Point));
	PlayDialogue(Point);
}

void AMenuGameMode::PlayDialogue_Implementation(EBedroomDialogue Point)
{
	const float Wait = Point == EBedroomDialogue::BeforeDream ? BeforeDreamDialogueWait : AfterWakingDialogueWait;
	GetWorldTimerManager().SetTimer(DialogueTimer, this, &AMenuGameMode::FinishDialogue, FMath::Max(Wait, 0.01f), false);
}

void AMenuGameMode::FinishDialogue()
{
	if (!bDialogueActive)
	{
		return;
	}
	bDialogueActive = false;
	GetWorldTimerManager().ClearTimer(DialogueTimer);

	if (ActiveDialogue == EBedroomDialogue::BeforeDream)
	{
		StartZoom();
	}
	else
	{
		ShowEndScreen();
	}
}

void AMenuGameMode::StartZoom()
{
	UE_LOG(LogQuietus, Log, TEXT("Bedroom: zoom into the bubble"));
	ZoomCamera = Bedroom ? Bedroom->GetCamera(EBedroomState::Night) : nullptr;
	if (ZoomCamera && ThoughtBubble)
	{
		ZoomStartFOV = ZoomCamera->GetCameraComponent()->FieldOfView;
		ZoomStartRotation = ZoomCamera->GetActorQuat();

		// 吹き出しの中心の方向へカメラも向ける（奥の壁が吹き出しと一緒に寄って見える）
		const FVector2D Center = ThoughtBubble->GetCenter();
		const FVector2D Viewport = ThoughtBubble->GetCachedGeometry().GetLocalSize();
		const double Aspect = Viewport.X > 0.0 ? Viewport.Y / Viewport.X : 9.0 / 16.0;
		const double TanHalf = FMath::Tan(FMath::DegreesToRadians(ZoomStartFOV * 0.5));
		const FVector Direction(1.0, (Center.X - 0.5) * 2.0 * TanHalf, (0.5 - Center.Y) * 2.0 * TanHalf * Aspect);
		ZoomEndRotation = ZoomStartRotation * Direction.GetSafeNormal().ToOrientationQuat();
	}
	ZoomElapsed = 0.0f;
	SetActorTickEnabled(true);
}

void AMenuGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	ZoomElapsed += DeltaSeconds;
	const float Time = FMath::Max(ZoomTime, 0.01f);
	const float Alpha = FMath::SmoothStep(0.0f, 1.0f, FMath::Min(ZoomElapsed / Time, 1.0f));

	if (ThoughtBubble)
	{
		ThoughtBubble->SetZoom(Alpha);
		if (ZoomCamera)
		{
			const double TanHalf = FMath::Tan(FMath::DegreesToRadians(ZoomStartFOV * 0.5)) / ThoughtBubble->GetZoomScale(Alpha);
			ZoomCamera->GetCameraComponent()->SetFieldOfView(FMath::RadiansToDegrees(FMath::Atan(TanHalf)) * 2.0f);
			ZoomCamera->SetActorRotation(FQuat::Slerp(ZoomStartRotation, ZoomEndRotation, Alpha));
		}
	}

	if (ZoomElapsed >= Time)
	{
		SetActorTickEnabled(false);
		ShowDreamBackdrop();
	}
}

void AMenuGameMode::ShowDreamBackdrop()
{
	UE_LOG(LogQuietus, Log, TEXT("Bedroom: dream backdrop"));

	// 吹き出しの中身が画面を覆った瞬間に切り替える。暗い下地から続けて明ける
	if (ThoughtBubble)
	{
		ThoughtBubble->RemoveFromParent();
		ThoughtBubble = nullptr;
	}
	Bedroom->ApplyState(EBedroomState::Dream);

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	ShowScene(PC, Bedroom->GetCamera(EBedroomState::Dream), [this, PC]()
	{
		UE_LOG(LogQuietus, Log, TEXT("Bedroom: difficulty select"));
		if (Menu)
		{
			Menu->ShowDifficulty();
			FocusMenu(PC);
		}
	});
}

void AMenuGameMode::ShowEndScreen()
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	UEndScreenWidget* EndScreen = PC && EndScreenWidgetClass ? CreateWidget<UEndScreenWidget>(PC, EndScreenWidgetClass) : nullptr;
	if (!EndScreen)
	{
		UE_LOG(LogQuietus, Error, TEXT("%s: could not spawn the end screen"), *GetName());
		return;
	}
	UE_LOG(LogQuietus, Log, TEXT("Bedroom: end screen"));
	EndScreen->SetResult(/*bWon=*/ true);
	EndScreen->AddToViewport(1);

	PC->SetInputMode(FInputModeUIOnly());
	PC->bShowMouseCursor = true;
}
