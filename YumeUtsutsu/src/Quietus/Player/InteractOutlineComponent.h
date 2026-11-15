#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractOutlineComponent.generated.h"

class AActor;
class UMaterialInterface;
class UMaterialInstanceDynamic;

/**
 *  近くにある拾える物・ロッカー・キャビネットに白い輪郭を出す。
 *  ShowDistance 以内に入った相手のメッシュをカスタム深度に描かせ、ポストプロセス（OutlineMaterial）がその縁を白く塗る。
 *  壁の向こうにある物は縁が隠れるので描かれない。ロッカーに隠れている間は出さない。
 *  値はすべてこのコンポーネント（BP_FirstPersonCharacter）で調整する。プレイ中に変えても次の更新で反映される。
 */
UCLASS(ClassGroup=(Quietus), meta=(BlueprintSpawnableComponent))
class UInteractOutlineComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInteractOutlineComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 縁を塗るポストプロセスのマテリアル（M_InteractOutline） */
	UPROPERTY(EditAnywhere, Category="Outline")
	TObjectPtr<UMaterialInterface> OutlineMaterial;

	/** 輪郭を出す相手のクラス（派生クラスも含む。既定は拾える物・ロッカー・キャビネット） */
	UPROPERTY(EditAnywhere, Category="Outline")
	TArray<TSubclassOf<AActor>> TargetClasses;

	/** この距離以内に近づいた物に輪郭を出す (cm) */
	UPROPERTY(EditAnywhere, Category="Outline", meta = (ClampMin = 0, Units = "cm"))
	float ShowDistance = 350.0f;

	/** 線の太さ（画面のピクセル） */
	UPROPERTY(EditAnywhere, Category="Outline", meta = (ClampMin = 0.5, ClampMax = 8))
	float Thickness = 1.5f;

	UPROPERTY(EditAnywhere, Category="Outline")
	FLinearColor Color = FLinearColor::White;

	/** 明るさ。露出（目の慣れ）に関係なく、画面上でこの倍率の明るさになる */
	UPROPERTY(EditAnywhere, Category="Outline", meta = (ClampMin = 0))
	float Brightness = 3.0f;

	/** 近さを調べ直す間隔 */
	UPROPERTY(EditAnywhere, Category="Outline", meta = (ClampMin = 0.02, Units = "s"))
	float UpdateInterval = 0.15f;

private:
	void Refresh();
	void ApplyMaterialParameters() const;
	static void SetOutlined(AActor* Actor, bool bOn);

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> OutlineMID;

	TSet<TWeakObjectPtr<AActor>> Outlined;
	float SinceUpdate = 0.0f;
};
