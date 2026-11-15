#include "Player/InteractOutlineComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/MeshComponent.h"
#include "EngineUtils.h"
#include "Interactables/Cabinet.h"
#include "Interactables/Locker.h"
#include "Interactables/PickupBase.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Player/PlayerCharacter.h"

UInteractOutlineComponent::UInteractOutlineComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	TargetClasses = { APickupBase::StaticClass(), ALocker::StaticClass(), ACabinet::StaticClass() };
}

void UInteractOutlineComponent::BeginPlay()
{
	Super::BeginPlay();

	// 一人称カメラのポストプロセスに縁取りを足す
	UCameraComponent* Camera = GetOwner() ? GetOwner()->FindComponentByClass<UCameraComponent>() : nullptr;
	if (OutlineMaterial && Camera)
	{
		OutlineMID = UMaterialInstanceDynamic::Create(OutlineMaterial, this);
		ApplyMaterialParameters();
		Camera->PostProcessSettings.WeightedBlendables.Array.Add(FWeightedBlendable(1.0f, OutlineMID));
	}
	Refresh();
}

void UInteractOutlineComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (const TWeakObjectPtr<AActor>& Actor : Outlined)
	{
		SetOutlined(Actor.Get(), false);
	}
	Outlined.Reset();
	Super::EndPlay(EndPlayReason);
}

void UInteractOutlineComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	SinceUpdate += DeltaTime;
	if (SinceUpdate >= UpdateInterval)
	{
		SinceUpdate = 0.0f;
		Refresh();
	}
}

void UInteractOutlineComponent::ApplyMaterialParameters() const
{
	if (OutlineMID)
	{
		OutlineMID->SetScalarParameterValue(TEXT("Thickness"), Thickness);
		OutlineMID->SetScalarParameterValue(TEXT("Brightness"), Brightness);
		OutlineMID->SetVectorParameterValue(TEXT("OutlineColor"), Color);
	}
}

void UInteractOutlineComponent::Refresh()
{
	ApplyMaterialParameters();

	const APlayerCharacter* Player = Cast<APlayerCharacter>(GetOwner());
	TSet<TWeakObjectPtr<AActor>> Near;
	if (Player && !Player->IsHiddenInLocker())
	{
		const FVector From = Player->GetActorLocation();
		const float MaxDistSq = FMath::Square(ShowDistance);
		for (const TSubclassOf<AActor>& Class : TargetClasses)
		{
			if (!Class)
			{
				continue;
			}
			for (TActorIterator<AActor> It(GetWorld(), Class); It; ++It)
			{
				if (!It->IsHidden() && FVector::DistSquared(It->GetActorLocation(), From) <= MaxDistSq)
				{
					Near.Add(*It);
				}
			}
		}
	}

	for (const TWeakObjectPtr<AActor>& Actor : Outlined)
	{
		if (!Near.Contains(Actor))
		{
			SetOutlined(Actor.Get(), false);
		}
	}
	for (const TWeakObjectPtr<AActor>& Actor : Near)
	{
		if (!Outlined.Contains(Actor))
		{
			SetOutlined(Actor.Get(), true);
		}
	}
	Outlined = MoveTemp(Near);
}

void UInteractOutlineComponent::SetOutlined(AActor* Actor, bool bOn)
{
	if (!Actor)
	{
		return;
	}
	// 見た目のメッシュだけ（当たり判定の箱などは描かない）
	TArray<UMeshComponent*> Meshes;
	Actor->GetComponents(Meshes);
	for (UMeshComponent* Mesh : Meshes)
	{
		Mesh->SetRenderCustomDepth(bOn);
	}
}
