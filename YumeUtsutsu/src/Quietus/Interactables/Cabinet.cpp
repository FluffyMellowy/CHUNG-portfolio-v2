#include "Interactables/Cabinet.h"
#include "Components/StaticMeshComponent.h"
#include "Interactables/NoteItem.h"
#include "Quietus.h"
#include "Core/DreamFeedbackSettings.h"
#include "Feedback/FeedbackSubsystem.h"

#define LOCTEXT_NAMESPACE "Interaction"

ACabinet::ACabinet()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(Root);
	BodyMesh->SetCollisionProfileName(FName("BlockAll"));

	// ボディ実測: 奥行50(±25)・幅60・高90、正面+X。引き出し(高38)は下段z5・上段z47.5、
	// 前面下端中央ピボットをボディ前面(x=+25)に合わせると閉状態になる
	DrawerLower = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DrawerLower"));
	DrawerLower->SetupAttachment(Root);
	DrawerLower->SetRelativeLocation(FVector(25.0f, 0.0f, 5.0f));
	DrawerLower->SetCollisionProfileName(FName("BlockAll"));

	DrawerUpper = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DrawerUpper"));
	DrawerUpper->SetupAttachment(Root);
	DrawerUpper->SetRelativeLocation(FVector(25.0f, 0.0f, 47.5f));
	DrawerUpper->SetCollisionProfileName(FName("BlockAll"));
}

void ACabinet::ConfigureLoot(const FCabinetLootTable& InLootTable,
	TSubclassOf<AActor> InBattery, TSubclassOf<AActor> InThrowable, TSubclassOf<AActor> InNote,
	const FText& InNoteTitle, const FText& InNoteBody, int32 InSeed)
{
	LootTable = InLootTable;
	BatteryClass = InBattery;
	ThrowableClass = InThrowable;
	NoteClass = InNote;
	CabinetNoteTitle = InNoteTitle;
	CabinetNoteBody = InNoteBody;
	LootSeed = InSeed;
}

void ACabinet::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 各引き出しの現在Xオフセットを目標へ補間する
	UStaticMeshComponent* Drawers[2] = { DrawerLower, DrawerUpper };
	bool bAnyMoving = false;
	for (int32 i = 0; i < 2; ++i)
	{
		const FVector Rel = Drawers[i]->GetRelativeLocation();
		const float Target = 25.0f + DrawerTargets[i];
		if (!FMath::IsNearlyEqual(Rel.X, Target, 0.05f))
		{
			const float NewX = FMath::FInterpConstantTo(Rel.X, Target, DeltaSeconds, SlideSpeed);
			Drawers[i]->SetRelativeLocation(FVector(NewX, Rel.Y, Rel.Z));
			bAnyMoving = true;
		}
	}
	if (!bAnyMoving && NextDrawer >= 2)
	{
		// 全段開き切ったら以後の補間は不要
		SetActorTickEnabled(false);
	}
}

void ACabinet::Interact_Implementation(APlayerCharacter* Interactor)
{
	if (NextDrawer >= 2)
	{
		return;
	}
	OpenAndRoll(NextDrawer == 0 ? DrawerLower : DrawerUpper);
	DrawerTargets[NextDrawer] = SlideDistance;
	++NextDrawer;
}

FText ACabinet::GetInteractionPrompt_Implementation() const
{
	return NextDrawer >= 2 ? FText::GetEmpty() : LOCTEXT("SearchCabinet", "調べる");
}

void ACabinet::OpenAndRoll(UStaticMeshComponent* Drawer)
{
	const UDreamFeedbackSettings& Sfx = UDreamFeedbackSettings::Get();
	UFeedbackSubsystem::PlayAt(this, Sfx.CabinetDrawer, Sfx.InteractAttenuation, Drawer ? Drawer->GetComponentLocation() : GetActorLocation(),
		TEXT("CabinetDrawer"));

	// シード決定論の抽選（キャビネット毎に配られたシード + 段番号）
	FRandomStream Roll(LootSeed + NextDrawer * 131);
	const float Total = LootTable.BatteryWeight + LootTable.ThrowableWeight + LootTable.NoteWeight + LootTable.EmptyWeight;
	float Pick = Roll.FRandRange(0.0f, FMath::Max(Total, KINDA_SMALL_NUMBER));

	TSubclassOf<AActor> ItemClass = nullptr;
	if ((Pick -= LootTable.BatteryWeight) < 0.0f)
	{
		ItemClass = BatteryClass;
	}
	else if ((Pick -= LootTable.ThrowableWeight) < 0.0f)
	{
		ItemClass = ThrowableClass;
	}
	else if ((Pick -= LootTable.NoteWeight) < 0.0f)
	{
		ItemClass = NoteClass;
	}
	// それ以外は空振り

	if (!ItemClass)
	{
		return;
	}

	// 開き切った引き出しの中に置く（前面ピボットの少し奥・引き出し床の少し上。
	// 開くのはこの後のTick補間なので、開いた後の前面位置 = 現在+SlideDistance で先取りする）
	const FVector SpawnPos = Drawer->GetComponentLocation()
		+ GetActorForwardVector() * (SlideDistance - 14.0f)
		+ FVector(0, 0, 14.0f);
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AActor* Item = GetWorld()->SpawnActor<AActor>(ItemClass, SpawnPos, GetActorRotation(), Params);
	if (!Item)
	{
		return;
	}
	if (ANoteItem* Note = Cast<ANoteItem>(Item))
	{
		Note->SetNoteText(CabinetNoteTitle, CabinetNoteBody);
	}
	UE_LOG(LogQuietus, Log, TEXT("%s: drawer %d yielded %s"), *GetName(), NextDrawer, *ItemClass->GetName());
}

#undef LOCTEXT_NAMESPACE
