#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "DreamLightingSettings.generated.h"

/**
 *  プレイ中の明るさと HUD の読みやすさの調整値（プロジェクト設定 > Game > Dream Lighting）。
 *  PIE 中に変えるとその場で反映される。
 *  懐中電灯の明るさは BP のコンポーネントではなくここが正。レベルの PostProcessVolume の露出もここの値で上書きされる。
 *  天井灯の明るさは生成プリセット（DA_LevelGen_*）の Lighting にある。
 */
UCLASS(Config = Game, DefaultConfig, BlueprintType, meta = (DisplayName = "Dream Lighting"))
class UDreamLightingSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static const UDreamLightingSettings& Get() { return *GetDefault<UDreamLightingSettings>(); }

	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	// ---- 懐中電灯 ----

	/** 明るさ（単位なし）。bFlashlightInverseSquare を入れたときは桁が大きく変わる */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="Flashlight", meta = (ClampMin = 0, UIMax = 40))
	float FlashlightIntensity = 9.0f;

	/** 距離の 2 乗で減衰させる（現実どおりだが、足元や目の前の壁が白く飛ぶ） */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="Flashlight")
	bool bFlashlightInverseSquare = false;

	/** 距離での落ち方。大きいほど遠くが早く暗くなる。近くの明るさは変わらない */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="Flashlight", meta = (EditCondition = "!bFlashlightInverseSquare", ClampMin = 0.1, UIMax = 16))
	float FlashlightFalloffExponent = 4.0f;

	/** 光が届く距離 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="Flashlight", meta = (ClampMin = 100, UIMax = 5000, Units = "cm"))
	float FlashlightRadius = 2500.0f;

	/** 光の芯の角度（度） */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="Flashlight", meta = (ClampMin = 0, ClampMax = 80))
	float FlashlightInnerCone = 14.0f;

	/** 光の外周の角度（度） */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="Flashlight", meta = (ClampMin = 1, ClampMax = 80))
	float FlashlightOuterCone = 38.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="Flashlight")
	FLinearColor FlashlightColor = FLinearColor::White;

	/** 照り返し（間接光）の強さ */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="Flashlight", meta = (ClampMin = 0, UIMax = 4))
	float FlashlightIndirect = 1.0f;

	/** 鏡面反射の強さ。下げるとテカりが減る */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="Flashlight", meta = (ClampMin = 0, UIMax = 1))
	float FlashlightSpecular = 1.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="Flashlight")
	bool bFlashlightShadows = true;

	// ---- 暗闇の補助光（懐中電灯を消しても周囲の形が分かるように、プレイヤーに付ける影なしの弱い光） ----

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="DarkFill")
	bool bDarkFill = true;

	/** 明るさ（単位なし） */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="DarkFill", meta = (EditCondition = "bDarkFill", ClampMin = 0, UIMax = 3))
	float DarkFillIntensity = 0.55f;

	/** 見える範囲。これより遠くは真っ暗のまま */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="DarkFill", meta = (EditCondition = "bDarkFill", ClampMin = 100, UIMax = 4000, Units = "cm"))
	float DarkFillRadius = 1500.0f;

	/** 範囲の端へ向かう落ち方。大きいほど近くだけ見える */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="DarkFill", meta = (EditCondition = "bDarkFill", ClampMin = 0.1, UIMax = 16))
	float DarkFillFalloffExponent = 2.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="DarkFill", meta = (EditCondition = "bDarkFill"))
	FLinearColor DarkFillColor = FLinearColor::White;

	/** 鏡面反射の強さ。0 ならカメラ正面の壁に光の点が出ない */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="DarkFill", meta = (EditCondition = "bDarkFill", ClampMin = 0, UIMax = 1))
	float DarkFillSpecular = 0.0f;

	// ---- 手に持ったアイテムの光（手元の物だけを照らす。周りは照らさない） ----
	// 懐中電灯の光源はカメラより前にあり、手元の物は一人称の縮尺でカメラ寄りに描かれるので、懐中電灯が届かない

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="HeldItem")
	bool bHeldItemLight = true;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="HeldItem", meta = (EditCondition = "bHeldItemLight", ClampMin = 0, UIMax = 20))
	float HeldItemLightIntensity = 3.0f;

	/** 光源の位置（カメラ基準。X=前、Y=右、Z=上） */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="HeldItem", meta = (EditCondition = "bHeldItemLight"))
	FVector HeldItemLightOffset = FVector(4.0f, -6.0f, 6.0f);

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="HeldItem", meta = (EditCondition = "bHeldItemLight", ClampMin = 10, UIMax = 200, Units = "cm"))
	float HeldItemLightRadius = 80.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="HeldItem", meta = (EditCondition = "bHeldItemLight"))
	FLinearColor HeldItemLightColor = FLinearColor(1.0f, 0.95f, 0.88f);

	// ---- 自動露出（EV100。小さいほど画面が明るくなる） ----

	/** 暗い所で明るくする上限。下げると暗闇がより見える */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="Exposure", meta = (UIMin = -6, UIMax = 8))
	float ExposureMinEV = 0.5f;

	/** 明るい所で暗くする上限 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="Exposure", meta = (UIMin = -6, UIMax = 12))
	float ExposureMaxEV = 4.0f;

	/** 全体の明るさの補正（+1 で 2 倍） */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="Exposure", meta = (UIMin = -4, UIMax = 4))
	float ExposureBias = 0.0f;

	/** 明るい所へ出たときの慣れの速さ */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="Exposure", meta = (ClampMin = 0.01, UIMax = 10))
	float ExposureSpeedUp = 3.0f;

	/** 暗い所へ入ったときの慣れの速さ */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="Exposure", meta = (ClampMin = 0.01, UIMax = 10))
	float ExposureSpeedDown = 1.0f;

	/** 露出を測るときに無視する暗い側の割合 (%) */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="Exposure", meta = (ClampMin = 0, ClampMax = 100))
	float ExposureLowPercent = 10.0f;

	/** 露出を測るときの明るい側の上限 (%)。下げると懐中電灯の光に露出が引っ張られにくい */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="Exposure", meta = (ClampMin = 0, ClampMax = 100))
	float ExposureHighPercent = 90.0f;

	// ---- HUD の文字（WBP_HUD のすべての TextBlock に掛かる） ----

	/** 文字の縁取りの太さ。0 で縁取りなし */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="HudText", meta = (ClampMin = 0, UIMax = 6))
	int32 HudOutlineSize = 2;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="HudText")
	FLinearColor HudOutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.9f);

	/** 影のずれ（ピクセル） */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="HudText")
	FVector2D HudShadowOffset = FVector2D(1.5, 1.5);

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="HudText")
	FLinearColor HudShadowColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.8f);
};
