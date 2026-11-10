#include "Feedback/FeedbackSubsystem.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/PostProcessComponent.h"
#include "Core/DreamFeedbackSettings.h"
#include "Core/RunGameState.h"
#include "Enemies/EnemyCharacter.h"
#include "Enemies/EnemyController.h"
#include "Engine/PointLight.h"
#include "EngineUtils.h"
#include "Feedback/FeedbackCameraShakes.h"
#include "Kismet/GameplayStatics.h"
#include "Player/PlayerCharacter.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"

DEFINE_LOG_CATEGORY(LogSfx);

namespace
{
	const TCHAR* ClassFolder = TEXT("/Game/Audio/Classes/");

	float Ramp(float Distance, float Far, float Near)
	{
		return Far > Near ? FMath::Clamp((Far - Distance) / (Far - Near), 0.0f, 1.0f) : (Distance <= Near ? 1.0f : 0.0f);
	}

	APlayerCharacter* GetRoundPlayer(const UWorld* World)
	{
		// ラウンド中（RunGameState があるワールド）だけ動く
		if (!World || !ARunGameState::Get(World))
		{
			return nullptr;
		}
		return Cast<APlayerCharacter>(UGameplayStatics::GetPlayerPawn(World, 0));
	}

	void SetLoopActive(UAudioComponent* Loop, bool bActive)
	{
		if (Loop && bActive != Loop->IsPlaying())
		{
			bActive ? Loop->Play() : Loop->Stop();
		}
	}
}

UFeedbackSubsystem* UFeedbackSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? World->GetSubsystem<UFeedbackSubsystem>() : nullptr;
}

bool UFeedbackSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World && (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE);
}

TStatId UFeedbackSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UFeedbackSubsystem, STATGROUP_Tickables);
}

USoundBase* UFeedbackSubsystem::PickSound(const FSfxCue& Cue)
{
	if (Cue.Sounds.Num() == 0)
	{
		return nullptr;
	}
	return Cue.Sounds[FMath::RandRange(0, Cue.Sounds.Num() - 1)].LoadSynchronous();
}

void UFeedbackSubsystem::Play2D(const UObject* WorldContext, const FSfxCue& Cue, const TCHAR* Tag, float VolumeScale, bool bUISound)
{
	USoundBase* Sound = PickSound(Cue);
	if (!Sound || VolumeScale <= 0.0f)
	{
		return;
	}
	const float Pitch = 1.0f + FMath::FRandRange(-Cue.PitchJitter, Cue.PitchJitter);
	UGameplayStatics::PlaySound2D(WorldContext, Sound, Cue.Volume * VolumeScale, Pitch, 0.0f, nullptr, nullptr, bUISound);
	UE_LOG(LogSfx, Verbose, TEXT("sfx %s 2D %s vol=%.2f"), Tag, *Sound->GetName(), Cue.Volume * VolumeScale);
}

void UFeedbackSubsystem::PlayAt(const UObject* WorldContext, const FSfxCue& Cue, const FSfxAttenuation& Attenuation, const FVector& Location,
	const TCHAR* Tag, float VolumeScale)
{
	UFeedbackSubsystem* Self = Get(WorldContext);
	USoundBase* Sound = PickSound(Cue);
	if (!Self || !Sound || VolumeScale <= 0.0f)
	{
		return;
	}
	const float Pitch = 1.0f + FMath::FRandRange(-Cue.PitchJitter, Cue.PitchJitter);
	UGameplayStatics::PlaySoundAtLocation(WorldContext, Sound, Location, FRotator::ZeroRotator, Cue.Volume * VolumeScale, Pitch, 0.0f,
		Self->GetAttenuation(Attenuation));
	UE_LOG(LogSfx, Verbose, TEXT("sfx %s 3D %s at (%.0f, %.0f, %.0f)"), Tag, *Sound->GetName(), Location.X, Location.Y, Location.Z);
}

UAudioComponent* UFeedbackSubsystem::CreateLoop2D(const UObject* WorldContext, const FSfxCue& Cue)
{
	USoundBase* Sound = PickSound(Cue);
	if (!Sound)
	{
		return nullptr;
	}
	UAudioComponent* Loop = UGameplayStatics::CreateSound2D(WorldContext, Sound, Cue.Volume, 1.0f, 0.0f, nullptr, false, false);
	if (Loop)
	{
		Loop->bIsUISound = false;
	}
	return Loop;
}

USoundAttenuation* UFeedbackSubsystem::GetAttenuation(const FSfxAttenuation& Settings)
{
	if (USoundAttenuation** Found = AttenuationByKey.Find(&Settings))
	{
		return *Found;
	}
	USoundAttenuation* Attenuation = NewObject<USoundAttenuation>(this);
	FSoundAttenuationSettings& A = Attenuation->Attenuation;
	A.bAttenuate = true;
	A.bSpatialize = true;
	A.DistanceAlgorithm = EAttenuationDistanceModel::Linear;
	A.AttenuationShape = EAttenuationShape::Sphere;
	A.AttenuationShapeExtents = FVector(Settings.InnerRadius, 0.0f, 0.0f);
	A.FalloffDistance = FMath::Max(Settings.FalloffEnd - Settings.InnerRadius, 1.0f);
	A.bEnableOcclusion = Settings.bOcclusion;
	A.OcclusionTraceChannel = ECC_Visibility;
	A.OcclusionLowPassFilterFrequency = Settings.OcclusionLowPass;
	A.OcclusionVolumeAttenuation = Settings.OcclusionVolume;
	A.OcclusionInterpolationTime = 0.2f;
	AttenuationObjects.Add(Attenuation);
	AttenuationByKey.Add(&Settings, Attenuation);
	return Attenuation;
}

void UFeedbackSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	ApplySoundMix();
}

void UFeedbackSubsystem::ApplySoundMix()
{
	// Sound Class の倍率。親子で掛け合わせた値を末端のクラスへ直接入れる（子への伝播には頼らない）
	const UDreamFeedbackSettings& S = UDreamFeedbackSettings::Get();
	const float Sfx = S.MasterVolume * S.SfxVolume;
	const TPair<const TCHAR*, float> Leaves[] = {
		{TEXT("SC_Player"), Sfx * S.PlayerVolume},
		{TEXT("SC_Interact"), Sfx * S.InteractVolume},
		{TEXT("SC_Enemy"), Sfx * S.EnemyVolume},
		{TEXT("SC_Ambience"), S.MasterVolume * S.AmbienceVolume},
		{TEXT("SC_Chase"), S.MasterVolume * S.ChaseVolume},
		{TEXT("SC_UI"), S.MasterVolume * S.UIVolume},
	};
	VolumeMix = NewObject<USoundMix>(this);
	for (const TPair<const TCHAR*, float>& Leaf : Leaves)
	{
		const FString Path = FString::Printf(TEXT("%s%s.%s"), ClassFolder, Leaf.Key, Leaf.Key);
		if (USoundClass* Class = LoadObject<USoundClass>(nullptr, *Path))
		{
			UGameplayStatics::SetSoundMixClassOverride(this, VolumeMix, Class, Leaf.Value, 1.0f, 0.0f, false);
		}
	}
	UGameplayStatics::PushSoundMixModifier(this, VolumeMix);
}

void UFeedbackSubsystem::StartRoundAudio()
{
	const UDreamFeedbackSettings& S = UDreamFeedbackSettings::Get();
	for (const FSfxCue* Cue : {&S.RoomTone, &S.DarkVent})
	{
		if (UAudioComponent* Loop = CreateLoop2D(this, *Cue))
		{
			Loop->Play();
			AmbienceLoops.Add(Loop);
		}
	}
	// 地鳴りは音量 0 のまま回しておき、近さで上げる
	ChaseBedLoop = CreateLoop2D(this, S.ChaseBed);
	if (ChaseBedLoop)
	{
		ChaseBedLoop->SetVolumeMultiplier(0.0f);
	}
	bRoundStarted = true;
	UE_LOG(LogSfx, Log, TEXT("round audio started (ambience=%d)"), AmbienceLoops.Num());
}

void UFeedbackSubsystem::Tick(float DeltaTime)
{
	UWorld* World = GetWorld();
	APlayerCharacter* Player = GetRoundPlayer(World);
	if (!Player)
	{
		return;
	}
	if (!bRoundStarted)
	{
		StartRoundAudio();
	}

	// いちばん近いピエロと、誰かに追われているか
	float Nearest = TNumericLimits<float>::Max();
	bool bChased = false;
	for (TActorIterator<AEnemyCharacter> It(World); It; ++It)
	{
		Nearest = FMath::Min(Nearest, static_cast<float>(FVector::Dist(It->GetActorLocation(), Player->GetActorLocation())));
		if (const AEnemyController* AI = Cast<AEnemyController>(It->GetController()))
		{
			bChased |= AI->IsChasing();
		}
	}

	UpdateProximity(DeltaTime, Nearest, bChased);
	UpdateScreen(DeltaTime, Nearest);
	UpdateLights(DeltaTime);

	LogTimer += DeltaTime;
	if (LogTimer >= 1.0f)
	{
		LogTimer = 0.0f;
		const int32 Disturbed = Lights.FilterByPredicate([](const FLightState& L) { return L.bDisturbed; }).Num();
		const int32 Buzzing = BuzzLoops.FilterByPredicate([](const UAudioComponent* A) { return A && A->IsPlaying(); }).Num();
		UE_LOG(LogSfx, Verbose, TEXT("proximity nearest=%.0f chased=%d bed=%.2f screen=%.2f lights=%d flicker=%d buzz=%d"),
			Nearest < TNumericLimits<float>::Max() ? Nearest : -1.0f, bChased, ChaseBedVolume, ScreenAlpha, Lights.Num(), Disturbed, Buzzing);
	}
}

void UFeedbackSubsystem::UpdateProximity(float DeltaTime, float NearestEnemy, bool bChased)
{
	const UDreamFeedbackSettings& S = UDreamFeedbackSettings::Get();
	const float Now = GetWorld()->GetTimeSeconds();

	// 追跡が始まった瞬間: 一撃と短い揺れ（続けて鳴らないよう間を空ける）
	if (bChased && !bWasChased && Now - LastStingerTime >= S.ChaseStingerCooldown)
	{
		LastStingerTime = Now;
		Play2D(this, S.ChaseStinger, TEXT("ChaseStinger"));
		StartShake(UChaseStartCameraShake::StaticClass(), S.ChaseStartShake);
	}
	bWasChased = bChased;

	// 鼓動。遠いと鳴らず、近いほど速く大きい。追われている間は下限を設ける。ロッカーの中でも止めない
	const float T = Ramp(NearestEnemy, S.HeartbeatFar, S.HeartbeatNear);
	const bool bHeartbeat = bChased || NearestEnemy < S.HeartbeatFar;
	if (bHeartbeat)
	{
		float Interval = FMath::Lerp(S.HeartbeatIntervalFar, S.HeartbeatIntervalNear, T);
		float Volume = FMath::Lerp(S.HeartbeatVolumeFar, S.HeartbeatVolumeNear, T);
		if (bChased)
		{
			Interval = FMath::Min(Interval, S.ChaseHeartbeatMaxInterval);
			Volume = FMath::Max(Volume, S.ChaseHeartbeatMinVolume);
		}
		HeartbeatTimer += DeltaTime;
		if (HeartbeatTimer >= Interval)
		{
			HeartbeatTimer = 0.0f;
			HeartbeatPulse = 1.0f;
			Play2D(this, S.Heartbeat, TEXT("Heartbeat"), Volume);
		}
	}
	else
	{
		// 範囲に入ったらすぐ 1 拍目が来るように
		HeartbeatTimer = S.HeartbeatIntervalFar;
	}
	HeartbeatPulse = FMath::Max(HeartbeatPulse - DeltaTime / S.HeartbeatPulseTime, 0.0f);

	// 追跡の地鳴り
	const float BedTarget = bChased ? S.ChaseBedChaseVolume : S.ChaseBedNearVolume * Ramp(NearestEnemy, S.ChaseBedFar, S.ChaseBedNear);
	ChaseBedVolume = S.ChaseBedSmoothing > 0.0f
		? FMath::FInterpConstantTo(ChaseBedVolume, BedTarget, DeltaTime, 1.0f / S.ChaseBedSmoothing)
		: BedTarget;
	if (ChaseBedLoop)
	{
		SetLoopActive(ChaseBedLoop, ChaseBedVolume > 0.001f);
		ChaseBedLoop->SetVolumeMultiplier(ChaseBedVolume);
	}
}

void UFeedbackSubsystem::UpdateScreen(float DeltaTime, float NearestEnemy)
{
	const UDreamFeedbackSettings& S = UDreamFeedbackSettings::Get();
	APlayerCharacter* Player = GetRoundPlayer(GetWorld());
	if (!Player)
	{
		return;
	}
	if (!ScreenEffect || ScreenEffect->GetOwner() != Player)
	{
		ScreenEffect = NewObject<UPostProcessComponent>(Player);
		ScreenEffect->bUnbound = true;
		ScreenEffect->Priority = 10.0f;
		ScreenEffect->SetupAttachment(Player->GetRootComponent());
		ScreenEffect->RegisterComponent();
	}

	const float Target = S.bScreenEffects ? Ramp(NearestEnemy, S.ScreenFar, S.ScreenNear) : 0.0f;
	ScreenAlpha = S.ScreenSmoothing > 0.0f ? FMath::FInterpConstantTo(ScreenAlpha, Target, DeltaTime, 1.0f / S.ScreenSmoothing) : Target;

	// 近いときの値へブレンドする。遠い（重み 0）ときはシーンの値のまま
	FPostProcessSettings& PP = ScreenEffect->Settings;
	PP.bOverride_VignetteIntensity = true;
	PP.VignetteIntensity = S.NearVignette + S.HeartbeatVignettePulse * HeartbeatPulse;
	PP.bOverride_SceneFringeIntensity = true;
	PP.SceneFringeIntensity = S.NearChromaticAberration;
	PP.bOverride_ColorSaturation = true;
	PP.ColorSaturation = FVector4(S.NearSaturation, S.NearSaturation, S.NearSaturation, 1.0f);
	ScreenEffect->BlendWeight = ScreenAlpha;
	ScreenEffect->bEnabled = ScreenAlpha > 0.001f;
}

void UFeedbackSubsystem::RefreshLights()
{
	// 生成器が置いた天井灯（PointLight アクター）を拾う。夢の作り直しで入れ替わるので時々拾い直す
	Lights.RemoveAll([](const FLightState& L) { return !L.Light.IsValid(); });
	for (TActorIterator<APointLight> It(GetWorld()); It; ++It)
	{
		const UPointLightComponent* Comp = Cast<UPointLightComponent>(It->GetLightComponent());
		if (!Comp || Comp->Mobility != EComponentMobility::Movable || Comp->Intensity <= 0.0f)
		{
			continue;
		}
		if (!Lights.ContainsByPredicate([&It](const FLightState& L) { return L.Light.Get() == *It; }))
		{
			FLightState State;
			State.Light = *It;
			State.BaseIntensity = Comp->Intensity;
			Lights.Add(State);
		}
	}
}

void UFeedbackSubsystem::UpdateLights(float DeltaTime)
{
	const UDreamFeedbackSettings& S = UDreamFeedbackSettings::Get();
	RefreshTimer -= DeltaTime;
	if (RefreshTimer <= 0.0f)
	{
		RefreshTimer = 3.0f;
		RefreshLights();
	}

	LightTimer += DeltaTime;
	if (LightTimer >= S.LightFlickerInterval)
	{
		const float Step = LightTimer;
		LightTimer = 0.0f;

		TArray<FVector> Enemies;
		for (TActorIterator<AEnemyCharacter> It(GetWorld()); It; ++It)
		{
			Enemies.Add(It->GetActorLocation());
		}
		for (FLightState& L : Lights)
		{
			APointLight* Light = L.Light.Get();
			if (!Light)
			{
				continue;
			}
			const FVector Pos = Light->GetActorLocation();
			// 天井灯なので高さは無視して床の上での距離で見る
			const bool bNear = S.bLightFlicker && Enemies.ContainsByPredicate([&](const FVector& E)
				{ return FVector::DistSquared2D(E, Pos) < FMath::Square(S.LightFlickerRadius); });
			if (bNear)
			{
				L.bDisturbed = true;
				L.DropoutLeft -= Step;
				float Scale;
				if (L.DropoutLeft > 0.0f)
				{
					Scale = 0.0f;
				}
				else if (FMath::FRand() < S.LightDropoutChance)
				{
					L.DropoutLeft = FMath::FRandRange(0.08f, 0.3f);
					Scale = 0.0f;
				}
				else
				{
					Scale = FMath::FRandRange(S.LightFlickerMin, S.LightFlickerMax);
				}
				Light->GetLightComponent()->SetIntensity(L.BaseIntensity * Scale);
			}
			else if (L.bDisturbed)
			{
				L.bDisturbed = false;
				L.DropoutLeft = 0.0f;
				Light->GetLightComponent()->SetIntensity(L.BaseIntensity);
			}
		}
	}

	BuzzTimer -= DeltaTime;
	if (BuzzTimer <= 0.0f)
	{
		BuzzTimer = 0.25f;
		UpdateLightBuzz();
	}
}

void UFeedbackSubsystem::UpdateLightBuzz()
{
	const UDreamFeedbackSettings& S = UDreamFeedbackSettings::Get();
	const APlayerCharacter* Player = GetRoundPlayer(GetWorld());
	if (!Player || S.MaxLightBuzz <= 0)
	{
		return;
	}

	// 聞こえる距離にある点いた照明を近い順に MaxLightBuzz 個
	const FVector Ear = Player->GetActorLocation();
	TArray<TPair<float, int32>> Candidates;
	for (int32 i = 0; i < Lights.Num(); ++i)
	{
		if (const APointLight* Light = Lights[i].Light.Get())
		{
			const float Dist = FVector::Dist(Light->GetActorLocation(), Ear);
			if (Dist < S.LightBuzzAttenuation.FalloffEnd)
			{
				Candidates.Emplace(Dist, i);
			}
		}
	}
	Candidates.Sort([](const TPair<float, int32>& A, const TPair<float, int32>& B) { return A.Key < B.Key; });
	TArray<int32> Wanted;
	for (int32 i = 0; i < FMath::Min(Candidates.Num(), S.MaxLightBuzz); ++i)
	{
		Wanted.Add(Candidates[i].Value);
	}

	while (BuzzLoops.Num() < S.MaxLightBuzz)
	{
		USoundBase* Sound = PickSound(S.LightBuzz);
		UAudioComponent* Loop = Sound ? UGameplayStatics::SpawnSoundAtLocation(this, Sound, Ear, FRotator::ZeroRotator, S.LightBuzz.Volume,
			1.0f, FMath::FRand() * 2.0f, GetAttenuation(S.LightBuzzAttenuation), nullptr, false) : nullptr;
		if (!Loop)
		{
			return;
		}
		Loop->Stop();
		BuzzLoops.Add(Loop);
		BuzzLightIndex.Add(INDEX_NONE);
	}

	// すでに付いている照明はそのまま、外れたものだけ空いた照明へ付け替える
	for (int32 i = 0; i < BuzzLoops.Num(); ++i)
	{
		if (BuzzLightIndex[i] != INDEX_NONE && !Wanted.Remove(BuzzLightIndex[i]))
		{
			BuzzLightIndex[i] = INDEX_NONE;
		}
	}
	for (int32 i = 0; i < BuzzLoops.Num(); ++i)
	{
		UAudioComponent* Loop = BuzzLoops[i];
		if (BuzzLightIndex[i] == INDEX_NONE && Wanted.Num() > 0)
		{
			BuzzLightIndex[i] = Wanted.Pop();
			Loop->SetWorldLocation(Lights[BuzzLightIndex[i]].Light->GetActorLocation());
		}
		const int32 Index = BuzzLightIndex[i];
		SetLoopActive(Loop, Index != INDEX_NONE);
		if (Index != INDEX_NONE && Lights[Index].Light.IsValid())
		{
			// 明滅で暗くなった照明はうなりも弱く
			const float Ratio = Lights[Index].BaseIntensity > 0.0f
				? Lights[Index].Light->GetLightComponent()->Intensity / Lights[Index].BaseIntensity : 1.0f;
			Loop->SetVolumeMultiplier(FMath::Clamp(Ratio, 0.0f, 1.0f));
		}
	}
}

void UFeedbackSubsystem::NotifyEnemyFootstep(const FVector& Location)
{
	const UDreamFeedbackSettings& S = UDreamFeedbackSettings::Get();
	const APlayerCharacter* Player = GetRoundPlayer(GetWorld());
	if (Player && FVector::Dist(Location, Player->GetActorLocation()) < S.FootstepShakeRadius)
	{
		StartShake(UFootstepCameraShake::StaticClass(), S.FootstepShake);
	}
}

void UFeedbackSubsystem::StartShake(TSubclassOf<UCameraShakeBase> Shake, float Scale) const
{
	const UDreamFeedbackSettings& S = UDreamFeedbackSettings::Get();
	if (!S.bCameraShake || Scale * S.CameraShakeScale <= 0.0f)
	{
		return;
	}
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		if (PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StartCameraShake(Shake, Scale * S.CameraShakeScale);
			UE_LOG(LogSfx, Verbose, TEXT("shake %s x%.2f"), *Shake->GetName(), Scale * S.CameraShakeScale);
		}
	}
}
