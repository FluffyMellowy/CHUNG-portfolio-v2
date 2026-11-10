#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "DreamFeedbackSettings.generated.h"

class USoundBase;
class USoundClass;

/** 一つの効果音イベント。候補から毎回ランダムに 1 つ選ぶ */
USTRUCT()
struct FSfxCue
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="Sfx")
	TArray<TSoftObjectPtr<USoundBase>> Sounds;

	UPROPERTY(EditAnywhere, Category="Sfx", meta = (ClampMin = 0))
	float Volume = 1.0f;

	/** ピッチの揺らぎ（0.03 なら ±3%） */
	UPROPERTY(EditAnywhere, Category="Sfx", meta = (ClampMin = 0, ClampMax = 0.5))
	float PitchJitter = 0.0f;
};

/** 3D の効果音の距離減衰 (cm) */
USTRUCT()
struct FSfxAttenuation
{
	GENERATED_BODY()

	/** この距離までは減衰しない */
	UPROPERTY(EditAnywhere, Category="Sfx", meta = (ClampMin = 0, Units = "cm"))
	float InnerRadius = 150.0f;

	/** 聞こえなくなる距離 */
	UPROPERTY(EditAnywhere, Category="Sfx", meta = (ClampMin = 1, Units = "cm"))
	float FalloffEnd = 1800.0f;

	/** 壁越しをこもらせる */
	UPROPERTY(EditAnywhere, Category="Sfx")
	bool bOcclusion = false;

	UPROPERTY(EditAnywhere, Category="Sfx", meta = (EditCondition = "bOcclusion", ClampMin = 100, Units = "Hz"))
	float OcclusionLowPass = 1800.0f;

	UPROPERTY(EditAnywhere, Category="Sfx", meta = (EditCondition = "bOcclusion", ClampMin = 0, ClampMax = 1))
	float OcclusionVolume = 0.6f;
};

/**
 *  効果音と近接演出の調整値（プロジェクト設定 > Game > Dream Feedback）。
 *  音量はファイルに焼いてあるので各イベントは 1.0 から始め、全体の釣り合いは Sound Class の倍率で取る。
 *  元の仕様は ArtSource/Audio/Batch1/DELIVERY.txt。
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Dream Feedback"))
class UDreamFeedbackSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UDreamFeedbackSettings();

	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	static const UDreamFeedbackSettings& Get() { return *GetDefault<UDreamFeedbackSettings>(); }

	// ---- Sound Class の音量（あとで音量設定画面から触れるように、クラスごとに分ける） ----

	UPROPERTY(Config, EditAnywhere, Category="Volume")
	TSoftObjectPtr<USoundClass> MasterClass;

	UPROPERTY(Config, EditAnywhere, Category="Volume", meta = (ClampMin = 0, ClampMax = 2))
	float MasterVolume = 1.0f;

	UPROPERTY(Config, EditAnywhere, Category="Volume", meta = (ClampMin = 0, ClampMax = 2))
	float SfxVolume = 1.0f;

	UPROPERTY(Config, EditAnywhere, Category="Volume", meta = (ClampMin = 0, ClampMax = 2))
	float PlayerVolume = 1.0f;

	UPROPERTY(Config, EditAnywhere, Category="Volume", meta = (ClampMin = 0, ClampMax = 2))
	float InteractVolume = 1.0f;

	UPROPERTY(Config, EditAnywhere, Category="Volume", meta = (ClampMin = 0, ClampMax = 2))
	float EnemyVolume = 1.0f;

	UPROPERTY(Config, EditAnywhere, Category="Volume", meta = (ClampMin = 0, ClampMax = 2))
	float AmbienceVolume = 1.0f;

	UPROPERTY(Config, EditAnywhere, Category="Volume", meta = (ClampMin = 0, ClampMax = 2))
	float ChaseVolume = 1.0f;

	UPROPERTY(Config, EditAnywhere, Category="Volume", meta = (ClampMin = 0, ClampMax = 2))
	float UIVolume = 1.0f;

	// ---- プレイヤー ----

	UPROPERTY(Config, EditAnywhere, Category="Player")
	FSfxCue StepWalk;

	UPROPERTY(Config, EditAnywhere, Category="Player")
	FSfxCue StepRun;

	UPROPERTY(Config, EditAnywhere, Category="Player")
	FSfxCue StepCrouch;

	/** 足音 1 歩ぶんの移動距離 (cm) */
	UPROPERTY(Config, EditAnywhere, Category="Player", meta = (ClampMin = 10, Units = "cm"))
	float WalkStride = 160.0f;

	UPROPERTY(Config, EditAnywhere, Category="Player", meta = (ClampMin = 10, Units = "cm"))
	float RunStride = 230.0f;

	UPROPERTY(Config, EditAnywhere, Category="Player", meta = (ClampMin = 10, Units = "cm"))
	float CrouchStride = 100.0f;

	UPROPERTY(Config, EditAnywhere, Category="Player")
	FSfxCue BreathRun;

	UPROPERTY(Config, EditAnywhere, Category="Player")
	FSfxCue BreathExhausted;

	/** 息のループのフェードイン／アウト（秒） */
	UPROPERTY(Config, EditAnywhere, Category="Player", meta = (ClampMin = 0, Units = "s"))
	float BreathFadeIn = 0.6f;

	UPROPERTY(Config, EditAnywhere, Category="Player", meta = (ClampMin = 0, Units = "s"))
	float BreathFadeOut = 1.2f;

	/** 走りが途切れてもこの時間は走りの息を続ける（壁で一瞬止まっただけで切れないように） */
	UPROPERTY(Config, EditAnywhere, Category="Player", meta = (ClampMin = 0, Units = "s"))
	float BreathRunHold = 0.8f;

	UPROPERTY(Config, EditAnywhere, Category="Player")
	FSfxCue FlashlightOn;

	UPROPERTY(Config, EditAnywhere, Category="Player")
	FSfxCue FlashlightOff;

	// ---- 拾う・触る ----

	UPROPERTY(Config, EditAnywhere, Category="Interact")
	FSfxCue PickupBattery;

	UPROPERTY(Config, EditAnywhere, Category="Interact")
	FSfxCue PickupNote;

	UPROPERTY(Config, EditAnywhere, Category="Interact")
	FSfxCue PickupGeneric;

	UPROPERTY(Config, EditAnywhere, Category="Interact")
	FSfxCue DoorOpen;

	UPROPERTY(Config, EditAnywhere, Category="Interact")
	FSfxCue DoorClose;

	UPROPERTY(Config, EditAnywhere, Category="Interact")
	FSfxCue LockerOpen;

	UPROPERTY(Config, EditAnywhere, Category="Interact")
	FSfxCue LockerClose;

	UPROPERTY(Config, EditAnywhere, Category="Interact")
	FSfxCue LockerBust;

	/** ロッカーに出入りするとき、扉を開けてから閉じるまで（秒） */
	UPROPERTY(Config, EditAnywhere, Category="Interact", meta = (ClampMin = 0, Units = "s"))
	float LockerCloseDelay = 0.45f;

	UPROPERTY(Config, EditAnywhere, Category="Interact")
	FSfxCue CabinetDrawer;

	UPROPERTY(Config, EditAnywhere, Category="Interact")
	FSfxCue ThrowableImpact;

	/** 扉・ロッカー・引き出し */
	UPROPERTY(Config, EditAnywhere, Category="Interact")
	FSfxAttenuation InteractAttenuation;

	UPROPERTY(Config, EditAnywhere, Category="Interact")
	FSfxAttenuation ImpactAttenuation;

	// ---- ピエロ ----

	/** 1 歩 = Step と Bell を 1 つずつ、Squeak は確率で重ねる */
	UPROPERTY(Config, EditAnywhere, Category="Enemy")
	FSfxCue ClownStep;

	UPROPERTY(Config, EditAnywhere, Category="Enemy")
	FSfxCue ClownBell;

	UPROPERTY(Config, EditAnywhere, Category="Enemy")
	FSfxCue ClownSqueak;

	UPROPERTY(Config, EditAnywhere, Category="Enemy", meta = (ClampMin = 0, ClampMax = 1))
	float ClownSqueakChance = 0.35f;

	/** AI の聴覚 1600 より遠くからプレイヤーが先に聞こえるように */
	UPROPERTY(Config, EditAnywhere, Category="Enemy")
	FSfxAttenuation ClownStepAttenuation;

	// ---- 環境音 ----

	UPROPERTY(Config, EditAnywhere, Category="Ambience")
	FSfxCue RoomTone;

	UPROPERTY(Config, EditAnywhere, Category="Ambience")
	FSfxCue DarkVent;

	/** 点いている天井灯のうなり。近い順に MaxLightBuzz 個まで */
	UPROPERTY(Config, EditAnywhere, Category="Ambience")
	FSfxCue LightBuzz;

	UPROPERTY(Config, EditAnywhere, Category="Ambience")
	FSfxAttenuation LightBuzzAttenuation;

	UPROPERTY(Config, EditAnywhere, Category="Ambience", meta = (ClampMin = 0))
	int32 MaxLightBuzz = 3;

	// ---- 近接（いちばん近いピエロまでの距離で決まる） ----

	UPROPERTY(Config, EditAnywhere, Category="Proximity")
	FSfxCue Heartbeat;

	/** この距離より遠いと鼓動は鳴らない。Near で最大 */
	UPROPERTY(Config, EditAnywhere, Category="Proximity", meta = (ClampMin = 0, Units = "cm"))
	float HeartbeatFar = 2000.0f;

	UPROPERTY(Config, EditAnywhere, Category="Proximity", meta = (ClampMin = 0, Units = "cm"))
	float HeartbeatNear = 400.0f;

	/** 鼓動の間隔（遠い→近い）と音量（遠い→近い） */
	UPROPERTY(Config, EditAnywhere, Category="Proximity", meta = (ClampMin = 0.1, Units = "s"))
	float HeartbeatIntervalFar = 1.2f;

	UPROPERTY(Config, EditAnywhere, Category="Proximity", meta = (ClampMin = 0.1, Units = "s"))
	float HeartbeatIntervalNear = 0.6f;

	UPROPERTY(Config, EditAnywhere, Category="Proximity", meta = (ClampMin = 0, ClampMax = 1))
	float HeartbeatVolumeFar = 0.25f;

	UPROPERTY(Config, EditAnywhere, Category="Proximity", meta = (ClampMin = 0, ClampMax = 1))
	float HeartbeatVolumeNear = 1.0f;

	/** 追われている間の下限（間隔はこれ以下、音量はこれ以上） */
	UPROPERTY(Config, EditAnywhere, Category="Proximity", meta = (ClampMin = 0.1, Units = "s"))
	float ChaseHeartbeatMaxInterval = 0.7f;

	UPROPERTY(Config, EditAnywhere, Category="Proximity", meta = (ClampMin = 0, ClampMax = 1))
	float ChaseHeartbeatMinVolume = 0.8f;

	UPROPERTY(Config, EditAnywhere, Category="Proximity")
	FSfxCue ChaseStinger;

	/** 追跡が始まるたびに鳴らすが、続けて鳴らない間隔（秒） */
	UPROPERTY(Config, EditAnywhere, Category="Proximity", meta = (ClampMin = 0, Units = "s"))
	float ChaseStingerCooldown = 20.0f;

	UPROPERTY(Config, EditAnywhere, Category="Proximity")
	FSfxCue ChaseBed;

	UPROPERTY(Config, EditAnywhere, Category="Proximity", meta = (ClampMin = 0, Units = "cm"))
	float ChaseBedFar = 1500.0f;

	UPROPERTY(Config, EditAnywhere, Category="Proximity", meta = (ClampMin = 0, Units = "cm"))
	float ChaseBedNear = 300.0f;

	UPROPERTY(Config, EditAnywhere, Category="Proximity", meta = (ClampMin = 0, ClampMax = 1))
	float ChaseBedNearVolume = 0.7f;

	UPROPERTY(Config, EditAnywhere, Category="Proximity", meta = (ClampMin = 0, ClampMax = 1))
	float ChaseBedChaseVolume = 1.0f;

	/** 追跡の地鳴りの音量変化をならす時間（秒） */
	UPROPERTY(Config, EditAnywhere, Category="Proximity", meta = (ClampMin = 0, Units = "s"))
	float ChaseBedSmoothing = 1.5f;

	// ---- 近接の画面効果（ポストプロセス） ----

	UPROPERTY(Config, EditAnywhere, Category="Screen")
	bool bScreenEffects = true;

	UPROPERTY(Config, EditAnywhere, Category="Screen", meta = (ClampMin = 0, Units = "cm"))
	float ScreenFar = 1500.0f;

	UPROPERTY(Config, EditAnywhere, Category="Screen", meta = (ClampMin = 0, Units = "cm"))
	float ScreenNear = 300.0f;

	/** いちばん近いときの値。遠いときはシーンの値のまま（間をブレンドする） */
	UPROPERTY(Config, EditAnywhere, Category="Screen", meta = (ClampMin = 0, ClampMax = 1))
	float NearVignette = 0.6f;

	UPROPERTY(Config, EditAnywhere, Category="Screen", meta = (ClampMin = 0, ClampMax = 5))
	float NearChromaticAberration = 1.0f;

	UPROPERTY(Config, EditAnywhere, Category="Screen", meta = (ClampMin = 0, ClampMax = 2))
	float NearSaturation = 0.75f;

	UPROPERTY(Config, EditAnywhere, Category="Screen", meta = (ClampMin = 0, Units = "s"))
	float ScreenSmoothing = 0.5f;

	/** 鼓動に合わせてビネットを一瞬強める量と長さ */
	UPROPERTY(Config, EditAnywhere, Category="Screen", meta = (ClampMin = 0, ClampMax = 1))
	float HeartbeatVignettePulse = 0.1f;

	UPROPERTY(Config, EditAnywhere, Category="Screen", meta = (ClampMin = 0.01, Units = "s"))
	float HeartbeatPulseTime = 0.25f;

	// ---- カメラの揺れ ----

	/** 酔い対策。false で全部止まる */
	UPROPERTY(Config, EditAnywhere, Category="CameraShake")
	bool bCameraShake = true;

	/** 全体の強さ */
	UPROPERTY(Config, EditAnywhere, Category="CameraShake", meta = (ClampMin = 0, ClampMax = 2))
	float CameraShakeScale = 1.0f;

	UPROPERTY(Config, EditAnywhere, Category="CameraShake", meta = (ClampMin = 0, ClampMax = 2))
	float ChaseStartShake = 1.0f;

	/** この距離以内のピエロの足音で揺らす */
	UPROPERTY(Config, EditAnywhere, Category="CameraShake", meta = (ClampMin = 0, Units = "cm"))
	float FootstepShakeRadius = 500.0f;

	UPROPERTY(Config, EditAnywhere, Category="CameraShake", meta = (ClampMin = 0, ClampMax = 2))
	float FootstepShake = 0.25f;

	// ---- 照明の明滅 ----

	UPROPERTY(Config, EditAnywhere, Category="Lights")
	bool bLightFlicker = true;

	/** ピエロからこの距離以内の点いた照明を不安定にする */
	UPROPERTY(Config, EditAnywhere, Category="Lights", meta = (ClampMin = 0, Units = "cm"))
	float LightFlickerRadius = 600.0f;

	/** 明滅中の明るさ（元の光量に対する倍率の範囲）と、一瞬消える確率 */
	UPROPERTY(Config, EditAnywhere, Category="Lights", meta = (ClampMin = 0, ClampMax = 1))
	float LightFlickerMin = 0.15f;

	UPROPERTY(Config, EditAnywhere, Category="Lights", meta = (ClampMin = 0, ClampMax = 1))
	float LightFlickerMax = 0.7f;

	UPROPERTY(Config, EditAnywhere, Category="Lights", meta = (ClampMin = 0, ClampMax = 1))
	float LightDropoutChance = 0.12f;

	UPROPERTY(Config, EditAnywhere, Category="Lights", meta = (ClampMin = 0.01, Units = "s"))
	float LightFlickerInterval = 0.07f;
};
