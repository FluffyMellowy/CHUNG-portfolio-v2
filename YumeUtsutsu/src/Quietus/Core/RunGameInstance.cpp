#include "RunGameInstance.h"
#include "Quietus.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Core/GameConstants.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "UI/ModalOverlayWidget.h"

#if !UE_BUILD_SHIPPING
#include "AudioDevice.h"
#include "AudioMixerBlueprintLibrary.h"
#include "AudioMixerDevice.h"
#include "AudioMixerPlatformNonRealtime.h"
#include "Containers/Ticker.h"
#include "HAL/IConsoleManager.h"
#include "Misc/App.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Player/PlayerCharacter.h"
#include "UnrealClient.h"

namespace
{
	/** 紹介動画の撮影中の状態 */
	FString CaptureFolder;
	int32 CaptureFrame = 0;
	FTSTicker::FDelegateHandle CaptureTicker;

	/** クック版では Python のハーネスが動かないので、パッケージ版の確認は -SmokeScript=<ファイル> で流す。
	 *  1 行 =「<待ち秒> <コンソールコマンド>」か「@<レベル名>」（そのレベルが開くまで待つ）か
	 *  「@#<番号>」（その番号の夢になるまで待つ）か「@walk」（SimWalkTo が終わるまで待つ）。# で始まる行は飛ばす */
	void StartSmokeScript(URunGameInstance* GameInstance)
	{
		FString Path;
		TArray<FString> Lines;
		if (!FParse::Value(FCommandLine::Get(), TEXT("SmokeScript="), Path) || !FFileHelper::LoadFileToStringArray(Lines, *Path))
		{
			return;
		}

		int32 Index = 0;
		float Wait = 0.0f;
		bool bWaitStarted = false;
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(GameInstance, [GameInstance, Lines, Index, Wait, bWaitStarted](float DeltaTime) mutable
		{
			if (Wait > 0.0f)
			{
				Wait -= DeltaTime;
				return true;
			}

			while (Lines.IsValidIndex(Index) && (Lines[Index].IsEmpty() || Lines[Index].StartsWith(TEXT("#"))))
			{
				++Index;
			}
			if (!Lines.IsValidIndex(Index))
			{
				return false;
			}

			const FString& Line = Lines[Index];
			if (Line.StartsWith(TEXT("@")))
			{
				const APlayerController* PC = GameInstance->GetFirstLocalPlayerController();
				const APlayerCharacter* Walker = PC ? Cast<APlayerCharacter>(PC->GetPawn()) : nullptr;
				const bool bReached = Line == TEXT("@walk") ? (!Walker || !Walker->IsDebugWalking())
					: Line.StartsWith(TEXT("@#")) ? GameInstance->GetDreamNumber() >= FCString::Atoi(*Line.RightChop(2))
					: UGameplayStatics::GetCurrentLevelName(GameInstance) == Line.RightChop(1);
				if (bReached)
				{
					UE_LOG(LogQuietus, Display, TEXT("SmokeScript: %s"), *Line);
					++Index;
				}
				return true;
			}

			FString Seconds, Command;
			Line.Split(TEXT(" "), &Seconds, &Command);
			if (!bWaitStarted)
			{
				// この行の待ちを始める
				bWaitStarted = true;
				Wait = FCString::Atof(*Seconds);
				return true;
			}

			UE_LOG(LogQuietus, Display, TEXT("SmokeScript: %s"), *Command);
			if (APlayerController* PC = GameInstance->GetFirstLocalPlayerController())
			{
				PC->ConsoleCommand(Command);
			}
			++Index;
			bWaitStarted = false;
			return true;
		}));
	}
}
#endif

void URunGameInstance::Init()
{
	Super::Init();

	// メニューを通らずにレベルを直接開いた場合（エディタのプレイなど）も命を持たせておく
	LivesRemaining = GetDifficultyParams().Lives;
	DreamNumber = 1;

#if !UE_BUILD_SHIPPING
	StartSmokeScript(this);
#endif
}

void URunGameInstance::StartRun(EDreamDifficulty InDifficulty)
{
	Difficulty = InDifficulty;
	LivesRemaining = GetDifficultyParams().Lives;
	DreamNumber = 1;
	bPendingDreamFadeIn = false;

	UE_LOG(LogQuietus, Log, TEXT("StartRun: difficulty=%s lives=%d shards=%d"),
		*UDreamDifficultySettings::GetDisplayName(Difficulty).ToString(), LivesRemaining, GetDifficultyParams().ShardCount);
	IssueSeedAndOpen(GameNames::GeneratedLevel);
}

void URunGameInstance::StartNewRun()
{
	StartRun(Difficulty);
}

bool URunGameInstance::LoseLife()
{
	LivesRemaining = FMath::Max(0, LivesRemaining - 1);
	UE_LOG(LogQuietus, Log, TEXT("Life lost: %d left"), LivesRemaining);
	return LivesRemaining > 0;
}

void URunGameInstance::ContinueDream()
{
	++DreamNumber;

	// 生成レベルなら新しいシードで別の構造に。手置きのレベルは同じレベルを開き直す
	// 捕まった暗転から続けて明けるので、開始前のキー入力待ちは出さない
	const UWorld* World = GetWorld();
	const FName Current = World ? FName(*UGameplayStatics::GetCurrentLevelName(World)) : GameNames::GeneratedLevel;
	bPendingDreamFadeIn = true;
	IssueSeedAndOpen(Current, /*bShowIntro=*/ false);
}

void URunGameInstance::WakeUp()
{
	UE_LOG(LogQuietus, Log, TEXT("Woke up: dream #%d"), DreamNumber);
	bPendingWakeUp = true;
	UGameplayStatics::OpenLevel(this, GameNames::MainMenuLevel);
}

bool URunGameInstance::ConsumeWakeUpFlag()
{
	const bool bResult = bPendingWakeUp;
	bPendingWakeUp = false;
	return bResult;
}

void URunGameInstance::SimButton(const FString& Name)
{
#if !UE_BUILD_SHIPPING
	TArray<UUserWidget*> Widgets;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this, Widgets, UUserWidget::StaticClass(), /*TopLevelOnly=*/ false);
	for (UUserWidget* Widget : Widgets)
	{
		// AnyKey = 「何かキーを押す」オーバーレイを閉じる
		if (UModalOverlayWidget* Overlay = Name == TEXT("AnyKey") ? Cast<UModalOverlayWidget>(Widget) : nullptr)
		{
			UE_LOG(LogQuietus, Log, TEXT("SimButton: dismiss %s"), *Widget->GetName());
			Overlay->Dismiss();
			return;
		}
		if (UButton* Button = Widget->WidgetTree ? Widget->WidgetTree->FindWidget<UButton>(FName(*Name)) : nullptr)
		{
			UE_LOG(LogQuietus, Log, TEXT("SimButton: %s.%s"), *Widget->GetName(), *Name);
			Button->OnClicked.Broadcast();
			return;
		}
	}
	UE_LOG(LogQuietus, Warning, TEXT("SimButton: %s not found"), *Name);
#endif
}

void URunGameInstance::CaptureStart(const FString& Folder, int32 Every)
{
#if !UE_BUILD_SHIPPING
	// 連番 PNG と全体の音を同時に録る。-BENCHMARK -FPS=30 -DeterministicAudio で起動すると、
	// 描画や保存にどれだけ時間がかかっても映像と音が同じ歩調で進む。音はスピーカーへ出ない
	if (CaptureTicker.IsValid())
	{
		return;
	}
	CaptureFolder = Folder;
	CaptureFrame = 0;
	IFileManager::Get().MakeDirectory(*CaptureFolder, /*Tree=*/ true);

	// ムービーレンダーキューと同じ手順。音はこちらで 1 フレームずつ進め、裏にあっても消音・停止させない
	FApp::SetUnfocusedVolumeMultiplier(1.0f);
	for (const TPair<const TCHAR*, int32>& Setting : { TPair<const TCHAR*, int32>(TEXT("au.nrt.RenderEveryTick"), 0),
		TPair<const TCHAR*, int32>(TEXT("au.NeverMuteNonRealtimeAudioDevices"), 1),
		TPair<const TCHAR*, int32>(TEXT("au.NeverDisableSubmixes"), 1) })
	{
		if (IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(Setting.Key))
		{
			Variable->Set(Setting.Value, ECVF_SetByConsole);
		}
	}
	UAudioMixerBlueprintLibrary::StartRecordingOutput(this, 300.0f);

	CaptureTicker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [this, Every](float)
	{
		if (Audio::FMixerDevice* Mixer = FAudioDeviceManager::GetAudioMixerDeviceFromWorldContext(this))
		{
			Audio::IAudioMixerPlatformInterface* Platform = Mixer->GetAudioMixerPlatform();
			if (Platform && Platform->IsNonRealtime())
			{
				Mixer->Update(/*bGameTicking=*/ true);
				static_cast<Audio::FMixerPlatformNonRealtime*>(Platform)->RenderAudio(FApp::GetDeltaTime());
			}
		}

		// 番号は経過フレームそのもの。描画されなかったフレームは欠番になり、動画にするときに前のコマで埋める
		// Every > 1 は段取り確認用の間引き
		if (CaptureFrame % FMath::Max(Every, 1) == 0)
		{
			FScreenshotRequest::RequestScreenshot(FString::Printf(TEXT("%s/frame_%06d.png"), *CaptureFolder, CaptureFrame), /*bShowUI=*/ true, /*bAddFilenameSuffix=*/ false);
		}
		++CaptureFrame;
		return true;
	}));
	const TSharedPtr<SWindow> Window = GEngine && GEngine->GameViewport ? GEngine->GameViewport->GetWindow() : nullptr;
	UE_LOG(LogQuietus, Display, TEXT("CaptureStart: %s window=\"%s\""), *CaptureFolder, Window.IsValid() ? *Window->GetTitle().ToString() : TEXT(""));
#endif
}

void URunGameInstance::CaptureStop()
{
#if !UE_BUILD_SHIPPING
	if (!CaptureTicker.IsValid())
	{
		return;
	}
	FTSTicker::RemoveTicker(CaptureTicker);
	CaptureTicker.Reset();
	UAudioMixerBlueprintLibrary::StopRecordingOutput(this, EAudioRecordingExportType::WavFile, TEXT("audio"), CaptureFolder);
	UE_LOG(LogQuietus, Display, TEXT("CaptureStop: %d frames"), CaptureFrame);
#endif
}

void URunGameInstance::IssueSeedAndOpen(FName LevelName, bool bShowIntro)
{
	// FMath::Rand は分解能が低いので時刻ベースでシードを作る
	PendingSeed = (int32)(FDateTime::Now().GetTicks() & 0x7FFFFFFF);
	if (PendingSeed <= 0)
	{
		PendingSeed = 1;
	}
#if !UE_BUILD_SHIPPING
	// 紹介動画の撮り直しで同じ迷路にするため
	int32 FixedSeed = 0;
	if (FParse::Value(FCommandLine::Get(), TEXT("RunSeed="), FixedSeed) && FixedSeed > 0)
	{
		PendingSeed = FixedSeed + DreamNumber;
	}
#endif
	bPendingRunIntro = bShowIntro;

	UE_LOG(LogQuietus, Log, TEXT("Open %s: dream #%d seed=%d"), *LevelName.ToString(), DreamNumber, PendingSeed);
	UGameplayStatics::OpenLevel(this, LevelName);
}

bool URunGameInstance::ConsumePendingSeed(int32& OutSeed)
{
	if (PendingSeed <= 0)
	{
		return false;
	}
	OutSeed = PendingSeed;
	PendingSeed = 0;
	return true;
}

bool URunGameInstance::ConsumeDreamFadeInFlag()
{
	const bool bResult = bPendingDreamFadeIn;
	bPendingDreamFadeIn = false;
	return bResult;
}

bool URunGameInstance::ConsumeRunIntroFlag()
{
	const bool bResult = bPendingRunIntro;
	bPendingRunIntro = false;
	return bResult;
}

const FDreamDifficultyParams& URunGameInstance::GetParams(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (const URunGameInstance* GI = World ? World->GetGameInstance<URunGameInstance>() : nullptr)
	{
		return GI->GetDifficultyParams();
	}
	return UDreamDifficultySettings::GetParams(EDreamDifficulty::Akumu);
}
