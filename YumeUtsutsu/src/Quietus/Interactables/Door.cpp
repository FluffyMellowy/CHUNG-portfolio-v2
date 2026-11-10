#include "Interactables/Door.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Enemies/EnemyCharacter.h"
#include "Interactables/RainbowDoorComponent.h"
#include "TimerManager.h"
#include "Core/DreamFeedbackSettings.h"
#include "Feedback/FeedbackSubsystem.h"

#define LOCTEXT_NAMESPACE "Interaction"

ADoor::ADoor()
{
	// 開閉補間のためTickを有効化
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	// 蝶番はルートに置き、メッシュ側をオフセットさせることでドアの端を軸に回転させる。
	// 開口150cmの左右端 (±75) に蝶番、パネルは中央へ向かって伸びる
	DoorPivot = CreateDefaultSubobject<USceneComponent>(TEXT("DoorPivot"));
	DoorPivot->SetupAttachment(Root);
	DoorPivot->SetRelativeLocation(FVector(-75.0f, 0.0f, 0.0f));

	DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
	DoorMesh->SetupAttachment(DoorPivot);

	DoorPivot2 = CreateDefaultSubobject<USceneComponent>(TEXT("DoorPivot2"));
	DoorPivot2->SetupAttachment(Root);
	DoorPivot2->SetRelativeLocation(FVector(75.0f, 0.0f, 0.0f));

	DoorMesh2 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh2"));
	DoorMesh2->SetupAttachment(DoorPivot2);

	// 閉扉は物理でだけ通行を止める。NavMeshは常に開口部を通したままにして、
	// 敵の経路自体は保ち「扉の前で開けるのを待つ」挙動にする
	for (UStaticMeshComponent* Mesh : { DoorMesh, DoorMesh2 })
	{
		Mesh->SetCollisionProfileName(FName("BlockAll"));
		Mesh->SetCanEverAffectNavigation(false);
	}

	// 開口部の両側をまたぐ検知ゾーン。Pawnオーバーラップ専用にし、
	// ライントレース（インタラクトの注視判定・検証トレース）には一切かからないようにする
	EnemyOpenZone = CreateDefaultSubobject<UBoxComponent>(TEXT("EnemyOpenZone"));
	EnemyOpenZone->SetupAttachment(Root);
	EnemyOpenZone->SetRelativeLocation(FVector(0.0f, 0.0f, 105.0f));
	EnemyOpenZone->SetBoxExtent(FVector(90.0f, 120.0f, 100.0f));
	EnemyOpenZone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	EnemyOpenZone->SetCollisionObjectType(ECC_WorldDynamic);
	EnemyOpenZone->SetCollisionResponseToAllChannels(ECR_Ignore);
	EnemyOpenZone->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void ADoor::BeginPlay()
{
	Super::BeginPlay();

	EnemyOpenZone->OnComponentBeginOverlap.AddDynamic(this, &ADoor::OnZoneBeginOverlap);

	// スポーン時の開き具合を即時反映（補間せず最初からその角度で立てる）
	bIsOpen = StartOpenRatio > KINDA_SMALL_NUMBER;
	const float InitialYaw = OpenAngle * StartOpenRatio;
	DoorPivot->SetRelativeRotation(FRotator(0.0f, InitialYaw, 0.0f));
	DoorPivot2->SetRelativeRotation(FRotator(0.0f, -InitialYaw, 0.0f));

	// Pawn遮断は閉状態のみ
	SetLeafPawnBlocking(!bIsOpen);
}

void ADoor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 現在の Yaw を目標角度へ一定速度で近づける（開閉どちらも同じ処理で済む）
	const float TargetYaw = bIsOpen ? OpenAngle : 0.0f;
	const float CurrentYaw = DoorPivot->GetRelativeRotation().Yaw;

	if (!FMath::IsNearlyEqual(CurrentYaw, TargetYaw, 0.01f))
	{
		const float NewYaw = FMath::FInterpConstantTo(CurrentYaw, TargetYaw, DeltaSeconds, OpenSpeed);
		DoorPivot->SetRelativeRotation(FRotator(0.0f, NewYaw, 0.0f));
		DoorPivot2->SetRelativeRotation(FRotator(0.0f, -NewYaw, 0.0f));

		// 閉じ切った瞬間にだけPawn遮断を入れる
		if (!bIsOpen && FMath::IsNearlyEqual(NewYaw, 0.0f, 0.5f))
		{
			SetLeafPawnBlocking(true);
		}
	}
}

void ADoor::SetLeafPawnBlocking(bool bBlock)
{
	for (UStaticMeshComponent* Mesh : { DoorMesh, DoorMesh2 })
	{
		Mesh->SetCollisionResponseToChannel(ECC_Pawn, bBlock ? ECR_Block : ECR_Ignore);
	}
}

void ADoor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (SingleLeafMesh)
	{
		ApplySingleLeaf();
	}
}

void ADoor::ApplySingleLeaf()
{
	// 規約上は原点=蝶番だが、実アセットのピボットが焼き込まれていない場合に備え、
	// バウンディングボックスから蝶番点（厚み中央・+Y端・下端）を求めて自前で原点合わせする。
	// ローカル-Y（ドア板の伸び）がアクター+X（開口中心方向）を向くよう Yaw +90 で取り付ける
	const FBox MeshBox = SingleLeafMesh->GetBoundingBox();
	const FVector HingePivot(MeshBox.GetCenter().X, MeshBox.Max.Y, MeshBox.Min.Z);
	const FRotator LeafRot(0.0f, 90.0f, 0.0f);
	const FVector PivotFix = LeafRot.RotateVector(HingePivot * SingleLeafScale) * -1.0f;

	DoorMesh->SetStaticMesh(SingleLeafMesh);
	DoorMesh->SetRelativeScale3D(SingleLeafScale);
	DoorMesh->SetRelativeLocation(PivotFix);
	DoorMesh->SetRelativeRotation(LeafRot);

	// 片開き運用: 右パネルは無効化
	DoorMesh2->SetStaticMesh(nullptr);
	DoorMesh2->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ADoor::ForceClose()
{
	bIsOpen = false;
	GetWorldTimerManager().ClearTimer(EnemyOpenTimer);
	DoorPivot->SetRelativeRotation(FRotator::ZeroRotator);
	DoorPivot2->SetRelativeRotation(FRotator::ZeroRotator);
	SetLeafPawnBlocking(true);
}

bool ADoor::IsRainbow() const
{
	const URainbowDoorComponent* Rainbow = FindComponentByClass<URainbowDoorComponent>();
	return Rainbow && Rainbow->IsCharged();
}

void ADoor::Interact_Implementation(APlayerCharacter* Interactor)
{
	// 虹色の扉は開かずに、別の扉の前へ運ぶ
	if (URainbowDoorComponent* Rainbow = FindComponentByClass<URainbowDoorComponent>())
	{
		if (Rainbow->IsCharged())
		{
			Rainbow->Use(Interactor);
			return;
		}
	}

	// 開閉トグル。実際の回転は Tick が目標角度へ追従することで行われる
	bIsOpen = !bIsOpen;
	PlayDoorSound(bIsOpen);

	if (bIsOpen)
	{
		// 開き始めた瞬間から素通し（Pawn遮断は閉じ切り時にTickが入れ直す）
		SetLeafPawnBlocking(false);
	}
	else
	{
		// 目の前で閉められた場合に備え、ゾーン内の敵がいれば開扉タイマーを回し直す
		GetWorldTimerManager().ClearTimer(EnemyOpenTimer);
		ArmEnemyTimerIfNeeded();
	}
}

FText ADoor::GetInteractionPrompt_Implementation() const
{
	if (IsRainbow())
	{
		return LOCTEXT("OpenRainbowDoor", "虹色の扉を開ける");
	}
	return bIsOpen ? LOCTEXT("CloseDoor", "閉める") : LOCTEXT("OpenDoor", "開ける");
}

void ADoor::OnZoneBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!bIsOpen && Cast<AEnemyCharacter>(OtherActor))
	{
		ArmEnemyTimerIfNeeded();
	}
}

void ADoor::ArmEnemyTimerIfNeeded()
{
	// 虹色の扉はピエロには開けられない
	if (bIsOpen || IsRainbow() || GetWorldTimerManager().IsTimerActive(EnemyOpenTimer))
	{
		return;
	}

	TArray<AActor*> Overlapping;
	EnemyOpenZone->GetOverlappingActors(Overlapping, AEnemyCharacter::StaticClass());
	if (Overlapping.Num() > 0)
	{
		GetWorldTimerManager().SetTimer(EnemyOpenTimer, this, &ADoor::OpenForEnemy, EnemyOpenDelay, false);
	}
}

void ADoor::PlayDoorSound(bool bOpening) const
{
	const UDreamFeedbackSettings& Sfx = UDreamFeedbackSettings::Get();
	UFeedbackSubsystem::PlayAt(this, bOpening ? Sfx.DoorOpen : Sfx.DoorClose, Sfx.InteractAttenuation,
		GetActorLocation() + FVector(0.0f, 0.0f, 100.0f), bOpening ? TEXT("DoorOpen") : TEXT("DoorClose"));
}

void ADoor::OpenForEnemy()
{
	bIsOpen = true;
	PlayDoorSound(true);
	SetLeafPawnBlocking(false);
}

#undef LOCTEXT_NAMESPACE
