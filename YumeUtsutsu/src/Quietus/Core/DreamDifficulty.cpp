#include "Core/DreamDifficulty.h"

#define LOCTEXT_NAMESPACE "Difficulty"

UDreamDifficultySettings::UDreamDifficultySettings()
{
	auto Set = [](FDreamDifficultyParams& P, int32 Lives, int32 Shards, int32 RainbowDoors, int32 Balloons, float Scale)
	{
		P.Lives = Lives;
		P.ShardCount = Shards;
		P.RainbowDoorCount = RainbowDoors;
		P.BalloonCount = Balloons;
		P.EnemySpeedScale = Scale;
		P.EnemyPerceptionScale = Scale;
	};
	Set(Yume, 3, 1, 3, 4, 0.85f);
	Set(Akumu, 2, 3, 2, 6, 1.0f);
	Set(Sarume, 1, 5, 1, 8, 1.2f);
}

const FDreamDifficultyParams& UDreamDifficultySettings::GetParams(EDreamDifficulty Difficulty)
{
	const UDreamDifficultySettings* Settings = GetDefault<UDreamDifficultySettings>();
	switch (Difficulty)
	{
	case EDreamDifficulty::Yume:   return Settings->Yume;
	case EDreamDifficulty::Sarume: return Settings->Sarume;
	default:                       return Settings->Akumu;
	}
}

FText UDreamDifficultySettings::GetDisplayName(EDreamDifficulty Difficulty)
{
	switch (Difficulty)
	{
	case EDreamDifficulty::Yume:   return LOCTEXT("Yume", "夢");
	case EDreamDifficulty::Sarume: return LOCTEXT("Sarume", "猿夢");
	default:                       return LOCTEXT("Akumu", "悪夢");
	}
}

#undef LOCTEXT_NAMESPACE
