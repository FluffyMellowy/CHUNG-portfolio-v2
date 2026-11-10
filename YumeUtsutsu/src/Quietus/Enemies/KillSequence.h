#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "KillSequence.generated.h"

class UAnimSequence;

/**
 *  敵がプレイヤーを捕まえたときの演出データ。敵のアニメと、それに合わせて動かすプレイヤー視点の軌跡を組で持つ。
 *  軌跡は敵メッシュの空間（cm）で FrameRate ごとに 1 キー。アニメの 0 秒と最初のキーが同じ瞬間。
 *  作り方は Tools/EnemyImport/death_attack_import.py（モデリングの Blender 案から書き出したもの）。
 */
UCLASS(BlueprintType)
class UKillSequence : public UDataAsset
{
	GENERATED_BODY()

public:
	/** 敵の本体メッシュに単体で流すアニメ */
	UPROPERTY(EditAnywhere, Category="Kill")
	TObjectPtr<UAnimSequence> Animation;

	/** プレイヤー視点の軌跡（敵メッシュの空間） */
	UPROPERTY(EditAnywhere, Category="Kill")
	TArray<FTransform> CameraTrack;

	UPROPERTY(EditAnywhere, Category="Kill", meta = (ClampMin = 1))
	float FrameRate = 30.0f;

	/** 暗転を始める時刻（アニメの再生位置。刃が視点に届く少し前） */
	UPROPERTY(EditAnywhere, Category="Kill", meta = (ClampMin = 0, Units = "s"))
	float ImpactTime = 3.5f;

	/** 暗転し切るまで */
	UPROPERTY(EditAnywhere, Category="Kill", meta = (ClampMin = 0, Units = "s"))
	float BlackoutTime = 0.08f;

	/** 捕まった瞬間の視点から軌跡へ移るまで */
	UPROPERTY(EditAnywhere, Category="Kill", meta = (ClampMin = 0, Units = "s"))
	float CameraBlendTime = 0.35f;

	bool IsPlayable() const { return Animation && CameraTrack.Num() >= 2; }

	/** Time 秒の視点（敵メッシュの空間）。キーの間は補間し、範囲の外は端のキー */
	FTransform SampleCamera(float Time) const;
};
