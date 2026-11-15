#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "DreamDifficulty.generated.h"

/** 難易度（夢の深さ）。最も深い 猿夢 は元になった怪談の名前 */
UENUM(BlueprintType)
enum class EDreamDifficulty : uint8
{
	Yume    UMETA(DisplayName = "夢"),
	Akumu   UMETA(DisplayName = "悪夢"),
	Sarume  UMETA(DisplayName = "猿夢"),
};

/** 難易度ごとの値 */
USTRUCT(BlueprintType)
struct FDreamDifficultyParams
{
	GENERATED_BODY()

	/** 1ランの命の数（捕まるたびに 1 減り、残っていれば夢を作り直して続ける） */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 1))
	int32 Lives = 2;

	/** 脱出扉の溝の数＝集める欠片の数（扉は 1/3/5 穴だけ用意してある） */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 1, ClampMax = 5))
	int32 ShardCount = 3;

	/** 迷路に落ちている鍵の数（1 本で 1 回、閉じた扉から別の扉の前へ移れる） */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 0))
	int32 KeyCount = 2;

	/** 廊下に浮かべる風船の数 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 0))
	int32 BalloonCount = 6;

	/** ピエロの移動速度の倍率 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 0.1))
	float EnemySpeedScale = 1.0f;

	/** ピエロの視覚・聴覚の範囲の倍率 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 0.1))
	float EnemyPerceptionScale = 1.0f;
};

/**
 *  難易度の調整値（プロジェクト設定 > Game > Dream Difficulty）。DefaultGame.ini に保存される。
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Dream Difficulty"))
class UDreamDifficultySettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UDreamDifficultySettings();

	UPROPERTY(Config, EditAnywhere, Category="Difficulty")
	FDreamDifficultyParams Yume;

	UPROPERTY(Config, EditAnywhere, Category="Difficulty")
	FDreamDifficultyParams Akumu;

	UPROPERTY(Config, EditAnywhere, Category="Difficulty")
	FDreamDifficultyParams Sarume;

	static const FDreamDifficultyParams& GetParams(EDreamDifficulty Difficulty);

	/** 画面に出す名前（夢 / 悪夢 / 猿夢） */
	static FText GetDisplayName(EDreamDifficulty Difficulty);

	virtual FName GetCategoryName() const override { return TEXT("Game"); }
};
