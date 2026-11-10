#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "DreamGimmicks.generated.h"

class UMaterialInterface;
class USoundBase;

/**
 *  迷路の仕掛け（虹色の扉・風船）の調整値（プロジェクト設定 > Game > Dream Gimmicks）。
 *  個数は難易度ごとなので Dream Difficulty 側にある。
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Dream Gimmicks"))
class UDreamGimmickSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UDreamGimmickSettings();

	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	// ---- 虹色の扉 ----

	/** 虹色の間だけ扉板に貼るマテリアル（使うと元のマテリアルに戻る）。
	 *  半透明の膜を重ねる方式だと懐中電灯で白飛びした板に埋もれるので、板そのものを塗り替える */
	UPROPERTY(Config, EditAnywhere, Category="RainbowDoor")
	TSoftObjectPtr<UMaterialInterface> RainbowSurfaceMaterial;

	/** 扉のまわりをほのかに照らす灯り（cd）と届く範囲 (cm) */
	UPROPERTY(Config, EditAnywhere, Category="RainbowDoor", meta = (ClampMin = 0))
	float RainbowLightIntensity = 4.0f;

	UPROPERTY(Config, EditAnywhere, Category="RainbowDoor", meta = (ClampMin = 0, Units = "cm"))
	float RainbowLightRadius = 350.0f;

	/** 行き先の扉から、この距離以内にピエロがいたら行き先にしない (cm) */
	UPROPERTY(Config, EditAnywhere, Category="RainbowDoor", meta = (ClampMin = 0, Units = "cm"))
	float ArrivalEnemyExclusionRadius = 1500.0f;

	/** 出発した扉からこの距離以内の扉は行き先にしない (cm) */
	UPROPERTY(Config, EditAnywhere, Category="RainbowDoor", meta = (ClampMin = 0, Units = "cm"))
	float MinTravelDistance = 1200.0f;

	/** 行き先の扉から、出てくる位置までの距離 (cm)。扉を背にして立つ */
	UPROPERTY(Config, EditAnywhere, Category="RainbowDoor", meta = (ClampMin = 0, Units = "cm"))
	float ArrivalOffset = 110.0f;

	/** 光で白く満ちるまで／光が引くまで（秒） */
	UPROPERTY(Config, EditAnywhere, Category="RainbowDoor", meta = (ClampMin = 0.05, Units = "s"))
	float WhiteoutTime = 0.45f;

	UPROPERTY(Config, EditAnywhere, Category="RainbowDoor", meta = (ClampMin = 0.05, Units = "s"))
	float RevealTime = 1.0f;

	// ---- 風船 ----

	/** 割れた音の大きさ（聴覚範囲への倍率。2 ならピエロの聴覚範囲の 2 倍先まで届く） */
	UPROPERTY(Config, EditAnywhere, Category="Balloon", meta = (ClampMin = 0))
	float BalloonNoiseLoudness = 2.0f;

	/** 風船の色の候補（スポーンごとに 1 色選ぶ） */
	UPROPERTY(Config, EditAnywhere, Category="Balloon")
	TArray<FLinearColor> BalloonPalette;

	/** 割れた音（仮音。差し替え予定） */
	UPROPERTY(Config, EditAnywhere, Category="Balloon")
	TSoftObjectPtr<USoundBase> BalloonPopSound;

	static const UDreamGimmickSettings& Get() { return *GetDefault<UDreamGimmickSettings>(); }
};
