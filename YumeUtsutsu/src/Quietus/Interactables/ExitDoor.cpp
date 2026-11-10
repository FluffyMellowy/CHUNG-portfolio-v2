#include "Interactables/ExitDoor.h"
#include "Components/StaticMeshComponent.h"
#include "Core/RunGameState.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Quietus.h"

#define LOCTEXT_NAMESPACE "Interaction"

namespace
{
	/** 扉ローカルの蝶番位置（Y） */
	constexpr float HingeY = 100.0f;

	/** はめる順番。中央 → 上の左右 → 下の左右 */
	TArray<int32> FillOrder(int32 SlotCount)
	{
		switch (SlotCount)
		{
		case 1:  return { 0 };
		case 3:  return { 0, 1, 3 };
		default: return { 0, 1, 3, 2, 4 };
		}
	}

	FName SocketName(int32 SlotId)
	{
		return *FString::Printf(TEXT("Shard_%02d"), SlotId);
	}
}

AExitDoor::AExitDoor()
{
	// 開くアニメーションの間だけ Tick する
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	WallMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WallMesh"));
	WallMesh->SetupAttachment(Root);
	WallMesh->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));

	FrameMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FrameMesh"));
	FrameMesh->SetupAttachment(Root);

	LeafL = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeafL"));
	LeafL->SetupAttachment(Root);
	LeafL->SetRelativeLocation(FVector(0.0f, HingeY, 0.0f));

	LeafR = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeafR"));
	LeafR->SetupAttachment(Root);
	LeafR->SetRelativeLocation(FVector(0.0f, -HingeY, 0.0f));
}

void AExitDoor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// エディタ上でも溝の数に合った扉板を見せる
	ApplyLeafMeshes();
}

void AExitDoor::BeginPlay()
{
	Super::BeginPlay();

	// 生成器が SetSlotCount を呼ばない（レベルに直置きした）場合は既定の数で用意する
	if (!bSlotsBuilt)
	{
		SetSlotCount(SlotCount);
	}
}

void AExitDoor::SetSlotCount(int32 InSlotCount)
{
	SlotCount = InSlotCount;
	if (!FindVariant())
	{
		UE_LOG(LogQuietus, Warning, TEXT("%s: no door variant for %d slots"), *GetName(), SlotCount);
	}
	ApplyLeafMeshes();
	BuildSlots();

	if (ARunGameState* GameState = ARunGameState::Get(this))
	{
		GameState->SetRequiredShards(Slots.Num());
	}
}

const FExitDoorVariant* AExitDoor::FindVariant() const
{
	return Variants.FindByPredicate([this](const FExitDoorVariant& V) { return V.SlotCount == SlotCount; });
}

void AExitDoor::ApplyLeafMeshes()
{
	if (const FExitDoorVariant* Variant = FindVariant())
	{
		LeafL->SetStaticMesh(Variant->LeafL);
		LeafR->SetStaticMesh(Variant->LeafR);
	}
}

void AExitDoor::BuildSlots()
{
	for (const FSlotVisual& Slot : Slots)
	{
		for (UStaticMeshComponent* Shard : Slot.Shards)
		{
			Shard->DestroyComponent();
		}
	}
	Slots.Reset();
	RimMaterials.Reset();
	NumFilled = 0;

	// 溝の縁は扉ごとに MID を作り、Fill00..04 で個別に光らせる
	for (UStaticMeshComponent* Leaf : { LeafL.Get(), LeafR.Get() })
	{
		const int32 RimIndex = Leaf->GetMaterialIndex(RimMaterialSlot);
		if (RimIndex != INDEX_NONE)
		{
			RimMaterials.Add(Leaf->CreateDynamicMaterialInstance(RimIndex));
		}
	}

	for (const int32 Id : FillOrder(SlotCount))
	{
		FSlotVisual& Slot = Slots.AddDefaulted_GetRef();
		Slot.Id = Id;
		if (Id == 0)
		{
			// 中央は継ぎ目をまたぐので半分ずつ各扉に付ける
			Slot.Shards.Add(AddShard(LeafL, Id, ShardHalfL));
			Slot.Shards.Add(AddShard(LeafR, Id, ShardHalfR));
		}
		else
		{
			// 01/02 は左扉、03/04 は右扉
			Slot.Shards.Add(AddShard(Id <= 2 ? LeafL : LeafR, Id, ShardMesh));
		}
	}
	bSlotsBuilt = true;
}

UStaticMeshComponent* AExitDoor::AddShard(UStaticMeshComponent* Leaf, int32 SlotId, UStaticMesh* Mesh)
{
	UStaticMeshComponent* Shard = NewObject<UStaticMeshComponent>(this);
	Shard->SetStaticMesh(Mesh);
	Shard->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Shard->SetupAttachment(Leaf, SocketName(SlotId));
	Shard->SetVisibility(false);
	Shard->RegisterComponent();
	return Shard;
}

void AExitDoor::FillSlot(const FSlotVisual& Slot)
{
	for (UStaticMeshComponent* Shard : Slot.Shards)
	{
		Shard->SetVisibility(true);
	}
	// 縁は Tick でふわっと灯す
	RimFades.Add({ *FString::Printf(TEXT("Fill%02d"), Slot.Id), 0.0f });
	SetActorTickEnabled(true);
}

void AExitDoor::TickRimFades(float DeltaSeconds)
{
	for (int32 i = RimFades.Num() - 1; i >= 0; --i)
	{
		FRimFade& Fade = RimFades[i];
		Fade.Elapsed += DeltaSeconds;
		const float T = FMath::Clamp(Fade.Elapsed / RimLightUpTime, 0.0f, 1.0f);
		// 0 → 1 に上がりながら途中で一度強く光る
		const float Value = T + RimFlash * FMath::Sin(T * PI);
		for (UMaterialInstanceDynamic* Rim : RimMaterials)
		{
			Rim->SetScalarParameterValue(Fade.Param, Value);
		}
		if (T >= 1.0f)
		{
			RimFades.RemoveAt(i);
		}
	}
}

void AExitDoor::Interact_Implementation(APlayerCharacter* Interactor)
{
	ARunGameState* GameState = ARunGameState::Get(this);
	if (!GameState || bOpening)
	{
		return;
	}

	const int32 Inserted = GameState->InsertShards();
	for (int32 i = 0; i < Inserted && NumFilled < Slots.Num(); ++i)
	{
		FillSlot(Slots[NumFilled++]);
	}

	if (GameState->IsExitOpen())
	{
		bOpening = true;
		OpenElapsed = 0.0f;
	}
}

FText AExitDoor::GetInteractionPrompt_Implementation() const
{
	const ARunGameState* GameState = ARunGameState::Get(this);
	if (!GameState || GameState->IsExitOpen())
	{
		return FText::GetEmpty();
	}
	if (GameState->GetHeldShards() > 0)
	{
		return FText::Format(LOCTEXT("ExitDoorInsert", "夢の欠片をはめる（{0}/{1}）"),
			GameState->GetInsertedShards(), GameState->GetRequiredShards());
	}
	return FText::Format(LOCTEXT("ExitDoorNeedShards", "夢の欠片が足りない（{0}/{1}）"),
		GameState->GetInsertedShards(), GameState->GetRequiredShards());
}

void AExitDoor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	TickRimFades(DeltaSeconds);

	bool bOpenDone = !bOpening;
	if (bOpening)
	{
		OpenElapsed += DeltaSeconds;
		const float Alpha = FMath::Clamp((OpenElapsed - OpenDelay) / OpenDuration, 0.0f, 1.0f);
		SetLeafAngle(OpenAngle * FMath::InterpEaseInOut(0.0f, 1.0f, Alpha, 2.0f));
		bOpenDone = Alpha >= 1.0f;
	}

	if (bOpenDone && RimFades.IsEmpty())
	{
		SetActorTickEnabled(false);
	}
}

void AExitDoor::SetLeafAngle(float Angle)
{
	// 裏側（-X）へ開く: 左は -yaw、右は +yaw。開き始めの OutsetAngle の間に外側へ少しずらす
	const float Outset = LeafOutset * FMath::Clamp(Angle / OutsetAngle, 0.0f, 1.0f);
	LeafL->SetRelativeLocationAndRotation(FVector(0.0f, HingeY + Outset, 0.0f), FRotator(0.0f, -Angle, 0.0f));
	LeafR->SetRelativeLocationAndRotation(FVector(0.0f, -HingeY - Outset, 0.0f), FRotator(0.0f, Angle, 0.0f));
}

#undef LOCTEXT_NAMESPACE
