#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "EnemyController.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;
class UBehaviorTree;

/** 敵AIのブラックボード（BB_Enemy）のキー名。コントローラが知覚結果を書き込み、ビヘイビアツリーが読む */
namespace EnemyKeys
{
	/** 追跡中のプレイヤー（Object）。セット中は追跡ブランチが動く */
	inline const FName TargetActor(TEXT("TargetActor"));

	/** 最後に知覚した位置（Vector）。セット中は捜索ブランチが動く */
	inline const FName LastKnownLocation(TEXT("LastKnownLocation"));

	/** 追跡対象が今視界内か（Bool） */
	inline const FName SeeingTarget(TEXT("SeeingTarget"));

	/** 現在の巡回先（Object） */
	inline const FName PatrolPoint(TEXT("PatrolPoint"));
}

/**
 *  敵のAIコントローラ。
 *  AIPerception（視覚＋聴覚）の結果をブラックボードへ書き込むのが役目で、
 *  行動（巡回・追跡・捜索）はビヘイビアツリー（BehaviorTreeAsset）が決める。
 *  聴覚はプレイヤー側の MakeNoise（移動ノイズ）をそのまま受け取る。
 *  勝敗確定（OnRoundFinished）とプレイヤーの安全地帯入り（OnSafeZoneEntered）は GameState の通知で受け取り、
 *  以後はツリーを止める。
 */
UCLASS()
class AEnemyController : public AAIController
{
	GENERATED_BODY()

	/** 知覚コンポーネント（視覚・聴覚の受け口） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAIPerceptionComponent> PerceptionComp;

	/** 視覚設定（コンストラクタで生成し、パラメータはBeginPlayで下のUPROPERTYから反映） */
	UPROPERTY()
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	/** 聴覚設定 */
	UPROPERTY()
	TObjectPtr<UAISenseConfig_Hearing> HearingConfig;

protected:

	/** 憑依時に実行するビヘイビアツリー（ブラックボードはツリー側の設定を使う） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy|AI")
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset;

	// ---------------------------------------------------------------
	// 知覚チューニング（行動側の数値はビヘイビアツリーの各ノードで設定する）
	// ---------------------------------------------------------------

	/** 視認距離 (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Perception", meta = (ClampMin = 0, Units = "cm"))
	float SightRadius = 1400.0f;

	/** 一度視認した対象を見失う距離 (cm)。SightRadiusより大きくする */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Perception", meta = (ClampMin = 0, Units = "cm"))
	float LoseSightRadius = 1800.0f;

	/** 視野の半角（度）。45なら左右45度=計90度の視野コーン */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Perception", meta = (ClampMin = 0, ClampMax = 90))
	float PeripheralVisionHalfAngle = 45.0f;

	/** 聴覚範囲 (cm)。プレイヤーのノイズLoudnessが距離に掛かる */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Perception", meta = (ClampMin = 0, Units = "cm"))
	float HearingRange = 1600.0f;

	/** このタグを持つアクターをパトロールポイントとして収集する */
	UPROPERTY(EditAnywhere, Category="Enemy|Patrol")
	FName PatrolPointTag;

private:

	/** 収集済みパトロールポイント（巡回順） */
	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> PatrolPoints;

	/** 勝敗確定・安全地帯入りの後は知覚を無視する */
	bool bStoppedHunting = false;

public:
	AEnemyController();

	/** 巡回先の一覧（BTTask_UpdatePatrolPoint が参照する） */
	const TArray<TObjectPtr<AActor>>& GetPatrolPoints() const { return PatrolPoints; }

	/** プレイヤーを追っている最中か（演出側が読むだけ。判断には使わない） */
	bool IsChasing() const;

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void BeginPlay() override;

	/** 知覚イベント（視覚・聴覚どちらもここに届く） */
	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	/** 勝敗確定の通知（GameState::OnRoundFinished を購読） */
	UFUNCTION()
	void HandleRoundFinished(bool bEscaped);

	/** プレイヤーが脱出扉の先の安全地帯に入った（GameState::OnSafeZoneEntered を購読） */
	UFUNCTION()
	void HandleSafeZoneEntered();

	/** プレイヤーが虹色の扉で移った（GameState::OnPlayerTeleported を購読）。追跡中なら見失い、出発地点を探す */
	UFUNCTION()
	void HandlePlayerTeleported(FVector From);

	/** プレイヤーを捕まえたが命が残っている（GameState::OnLifeLost を購読）。夢の作り直しまで止まる */
	UFUNCTION()
	void HandleLifeLost(int32 LivesRemaining);

	/** 移動とツリーを止めて、以後の知覚も無視する */
	void StopHunting(const TCHAR* Reason);
};
