#include "Enemies/EnemyController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BrainComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Player/PlayerCharacter.h"
#include "Core/GameConstants.h"
#include "Core/RunGameInstance.h"
#include "Core/RunGameState.h"
#include "Quietus.h"

AEnemyController::AEnemyController()
{
	PatrolPointTag = GameNames::PatrolPointTag;

	PerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("Perception"));
	SetPerceptionComponent(*PerceptionComp);

	// 視覚: 敵味方関係なく全Pawnを検知対象にする（このゲームに陣営概念はない）
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	PerceptionComp->ConfigureSense(*SightConfig);

	// 聴覚: プレイヤー側の MakeNoise がそのまま刺激として届く
	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
	PerceptionComp->ConfigureSense(*HearingConfig);

	// 視覚と聴覚が同時に発生した場合は視覚を優先する
	PerceptionComp->SetDominantSense(SightConfig->GetSenseImplementation());
}

void AEnemyController::BeginPlay()
{
	// エディタで調整したチューニング値に難易度の倍率を掛けてセンス設定へ反映してから起動する
	const float Scale = URunGameInstance::GetParams(this).EnemyPerceptionScale;
	SightConfig->SightRadius = SightRadius * Scale;
	SightConfig->LoseSightRadius = LoseSightRadius * Scale;
	SightConfig->PeripheralVisionAngleDegrees = PeripheralVisionHalfAngle;
	PerceptionComp->ConfigureSense(*SightConfig);

	HearingConfig->HearingRange = HearingRange * Scale;
	PerceptionComp->ConfigureSense(*HearingConfig);

	Super::BeginPlay();

	PerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &AEnemyController::OnTargetPerceptionUpdated);

	if (ARunGameState* GameState = ARunGameState::Get(this))
	{
		GameState->OnRoundFinished.AddDynamic(this, &AEnemyController::HandleRoundFinished);
		GameState->OnSafeZoneEntered.AddDynamic(this, &AEnemyController::HandleSafeZoneEntered);
		GameState->OnLifeLost.AddDynamic(this, &AEnemyController::HandleLifeLost);
		GameState->OnPlayerTeleported.AddDynamic(this, &AEnemyController::HandlePlayerTeleported);
	}

	// レベル内のパトロールポイント（指定タグ付きアクター）を収集
	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), PatrolPointTag, Found);
	PatrolPoints.Reset();
	for (AActor* Point : Found)
	{
		PatrolPoints.Add(Point);
	}

	if (PatrolPoints.Num() == 0)
	{
		UE_LOG(LogQuietus, Warning, TEXT("%s: no patrol points found (tag=%s)"), *GetName(), *PatrolPointTag.ToString());
	}
}

void AEnemyController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (!BehaviorTreeAsset)
	{
		UE_LOG(LogQuietus, Error, TEXT("%s: BehaviorTreeAsset is not set — the enemy will not act"), *GetName());
		return;
	}
	RunBehaviorTree(BehaviorTreeAsset);
}

void AEnemyController::HandleRoundFinished(bool bEscaped)
{
	// 勝敗確定後は行動停止（余韻の演出は後続タスク）
	StopHunting(TEXT("RoundFinished"));
}

void AEnemyController::HandleSafeZoneEntered()
{
	// 扉の先は夢から覚めかけた場所。ピエロはもう追ってこない
	StopHunting(TEXT("SafeZone"));
}

void AEnemyController::HandlePlayerTeleported(FVector From)
{
	UBlackboardComponent* BB = GetBlackboardComponent();
	if (!BB || bStoppedHunting || !BB->GetValueAsObject(EnemyKeys::TargetActor))
	{
		return;
	}
	// 鍵で扉の向こうへ消えた。追跡をやめて、消えた扉のあたりを探す
	UE_LOG(LogQuietus, Log, TEXT("%s: player vanished through a door with a key -> Search"), *GetName());
	BB->ClearValue(EnemyKeys::TargetActor);
	BB->SetValueAsBool(EnemyKeys::SeeingTarget, false);
	BB->SetValueAsVector(EnemyKeys::LastKnownLocation, From);
}

void AEnemyController::HandleLifeLost(int32 LivesRemaining)
{
	StopHunting(TEXT("LifeLost"));
}

void AEnemyController::StopHunting(const TCHAR* Reason)
{
	bStoppedHunting = true;
	StopMovement();
	if (BrainComponent)
	{
		BrainComponent->StopLogic(Reason);
	}
}

bool AEnemyController::IsChasing() const
{
	const UBlackboardComponent* BB = GetBlackboardComponent();
	return !bStoppedHunting && BB && BB->GetValueAsObject(EnemyKeys::TargetActor) != nullptr;
}

void AEnemyController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	APlayerCharacter* Player = Cast<APlayerCharacter>(Actor);
	UBlackboardComponent* BB = GetBlackboardComponent();
	if (!Player || !BB || bStoppedHunting)
	{
		return;
	}

	const bool bChasing = BB->GetValueAsObject(EnemyKeys::TargetActor) != nullptr;
	const bool bIsSight = Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>();

	if (bIsSight)
	{
		if (Stimulus.WasSuccessfullySensed())
		{
			// 視認: 即追跡
			BB->SetValueAsVector(EnemyKeys::LastKnownLocation, Stimulus.StimulusLocation);
			BB->SetValueAsBool(EnemyKeys::SeeingTarget, true);
			if (!bChasing)
			{
				UE_LOG(LogQuietus, Log, TEXT("%s: player spotted -> Chase"), *GetName());
				BB->SetValueAsObject(EnemyKeys::TargetActor, Player);
			}
		}
		else
		{
			// 見失った: すぐには追跡をやめず、追跡タスクの猶予（MemoryTime）に任せる。
			// 体の向きの揺れによる視界明滅で行動が乱打されるのを防ぐ。
			// 追跡していない時の消失通知で捜索が始まらないよう、位置は追跡中だけ更新する
			BB->SetValueAsBool(EnemyKeys::SeeingTarget, false);
			if (bChasing)
			{
				BB->SetValueAsVector(EnemyKeys::LastKnownLocation, Stimulus.StimulusLocation);
			}
		}
	}
	else if (Stimulus.WasSuccessfullySensed() && !bChasing)
	{
		// 物音（聴覚）: 追跡中でなければ音の発生地点を調べに行く。
		// 足音は連続発生するため、捜索中の再通知はログを出さず目的地だけ更新する
		if (!BB->IsVectorValueSet(EnemyKeys::LastKnownLocation))
		{
			UE_LOG(LogQuietus, Log, TEXT("%s: heard noise -> Search"), *GetName());
		}
		BB->SetValueAsVector(EnemyKeys::LastKnownLocation, Stimulus.StimulusLocation);
	}
}
