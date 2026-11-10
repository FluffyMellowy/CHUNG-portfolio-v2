#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "FeedbackSubsystem.generated.h"

struct FSfxCue;
struct FSfxAttenuation;
class UAudioComponent;
class UPostProcessComponent;
class USoundAttenuation;
class USoundBase;
class USoundMix;
class APointLight;

DECLARE_LOG_CATEGORY_EXTERN(LogSfx, Log, All);

/**
 *  プレイ中の音と近接演出をまとめて受け持つ。設定は UDreamFeedbackSettings。
 *  - 効果音の再生口（Play2D / PlayAt）。候補からのランダム選択・ピッチの揺らぎ・距離減衰をここで付ける
 *  - ラウンド中の環境音（部屋鳴り・換気・近くの天井灯のうなり）
 *  - いちばん近いピエロまでの距離で決まる鼓動・追跡の地鳴り・画面効果・照明の明滅、追跡開始の一撃と揺れ
 *  メニューなどラウンド外のワールドでは環境音も近接演出も動かさない（再生口は使える）。
 */
UCLASS()
class UFeedbackSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static UFeedbackSubsystem* Get(const UObject* WorldContext);

	/** プレイヤー自身の音（定位なし）。bUISound はポーズ中でも鳴らすもの */
	static void Play2D(const UObject* WorldContext, const FSfxCue& Cue, const TCHAR* Tag, float VolumeScale = 1.0f, bool bUISound = false);

	/** 場所から鳴る音 */
	static void PlayAt(const UObject* WorldContext, const FSfxCue& Cue, const FSfxAttenuation& Attenuation, const FVector& Location,
		const TCHAR* Tag, float VolumeScale = 1.0f);

	/** 定位なしのループ。止まった状態で返す（FadeIn で鳴らす） */
	static UAudioComponent* CreateLoop2D(const UObject* WorldContext, const FSfxCue& Cue);

	/** 候補から 1 つ選ぶ（読み込みも行う） */
	static USoundBase* PickSound(const FSfxCue& Cue);

	/** 敵の足音が鳴った（近ければカメラを揺らす） */
	void NotifyEnemyFootstep(const FVector& Location);

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

private:
	USoundAttenuation* GetAttenuation(const FSfxAttenuation& Settings);

	void ApplySoundMix();
	void StartRoundAudio();
	void UpdateProximity(float DeltaTime, float NearestEnemy, bool bChased);
	void UpdateScreen(float DeltaTime, float NearestEnemy);
	void UpdateLights(float DeltaTime);
	void UpdateLightBuzz();
	void RefreshLights();
	void StartShake(TSubclassOf<class UCameraShakeBase> Shake, float Scale) const;

	/** 設定の減衰ごとに作った USoundAttenuation（設定はデフォルトオブジェクトの中なのでアドレスで引ける） */
	UPROPERTY(Transient)
	TArray<TObjectPtr<USoundAttenuation>> AttenuationObjects;
	TMap<const FSfxAttenuation*, USoundAttenuation*> AttenuationByKey;

	UPROPERTY(Transient)
	TObjectPtr<USoundMix> VolumeMix;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UAudioComponent>> AmbienceLoops;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> ChaseBedLoop;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UAudioComponent>> BuzzLoops;

	/** 近接の画面効果（プレイヤーに付ける。境界なしで全体にかかる） */
	UPROPERTY(Transient)
	TObjectPtr<UPostProcessComponent> ScreenEffect;

	struct FLightState
	{
		TWeakObjectPtr<APointLight> Light;
		float BaseIntensity = 0.0f;
		bool bDisturbed = false;
		float DropoutLeft = 0.0f;
	};
	TArray<FLightState> Lights;
	/** 唸りのループがどの照明に付いているか（BuzzLoops と同じ並び） */
	TArray<int32> BuzzLightIndex;

	bool bRoundStarted = false;
	bool bWasChased = false;
	float LastStingerTime = -1000.0f;
	float HeartbeatTimer = 0.0f;
	float HeartbeatPulse = 0.0f;
	float ChaseBedVolume = 0.0f;
	float ScreenAlpha = 0.0f;
	float LightTimer = 0.0f;
	float BuzzTimer = 0.0f;
	float RefreshTimer = 0.0f;
	float LogTimer = 0.0f;
};
