#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "DreamGimmicks.generated.h"

class USoundBase;

/**
 *  迷路の仕掛け（鍵で扉から移る・風船）の調整値（プロジェクト設定 > Game > Dream Gimmicks）。
 *  個数は難易度ごとなので Dream Difficulty 側にある。
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Dream Gimmicks"))
class UDreamGimmickSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UDreamGimmickSettings();

	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	// ---- 鍵（閉じた扉に使うと迷路の別の扉の前へ移る） ----
	// 行き先の選び方は仮。今は旧「虹色の扉」と同じ規則

	/** 行き先の扉から、この距離以内にピエロがいたら行き先にしない (cm) */
	UPROPERTY(Config, EditAnywhere, Category="KeyWarp", meta = (ClampMin = 0, Units = "cm"))
	float ArrivalEnemyExclusionRadius = 1500.0f;

	/** 鍵を使った扉からこの距離以内の扉は行き先にしない (cm) */
	UPROPERTY(Config, EditAnywhere, Category="KeyWarp", meta = (ClampMin = 0, Units = "cm"))
	float MinTravelDistance = 1200.0f;

	/** 行き先の扉から、出てくる位置までの距離 (cm)。扉を背にして立つ */
	UPROPERTY(Config, EditAnywhere, Category="KeyWarp", meta = (ClampMin = 0, Units = "cm"))
	float ArrivalOffset = 110.0f;

	/** 光で白く満ちるまで／光が引くまで（秒） */
	UPROPERTY(Config, EditAnywhere, Category="KeyWarp", meta = (ClampMin = 0.05, Units = "s"))
	float WhiteoutTime = 0.45f;

	UPROPERTY(Config, EditAnywhere, Category="KeyWarp", meta = (ClampMin = 0.05, Units = "s"))
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
