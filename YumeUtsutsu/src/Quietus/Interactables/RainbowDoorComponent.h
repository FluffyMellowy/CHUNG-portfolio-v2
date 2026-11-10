#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "RainbowDoorComponent.generated.h"

class ADoor;
class APlayerCharacter;
class UBoxComponent;
class UPointLightComponent;
class UMaterialInterface;
class UMeshComponent;

/**
 *  普通の扉を「虹色の扉」にする部品。生成器が選んだ扉に付ける。
 *  - 扉板に虹色のオーバーレイを重ね、まわりをほのかな光で照らす（色はゆっくり巡る）
 *  - 付いている間は閉じたまま。ピエロは開けられず、ナビメッシュ上も通れない
 *  - プレイヤーが開けると、画面が光で満ちて迷路の別の扉の前へ移る。使い終わると外れて普通の扉に戻る
 *  - 追われている最中に使うと、ピエロはプレイヤーを見失い、出発した扉のあたりを探す
 */
USTRUCT()
struct FRainbowSavedMaterials
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInterface>> Materials;
};

UCLASS(ClassGroup = (Quietus), meta = (BlueprintSpawnableComponent))
class URainbowDoorComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	URainbowDoorComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** プレイヤーが扉を開けようとした（扉の Interact から呼ばれる） */
	void Use(APlayerCharacter* Player);

	/** まだ使われていないか（使用中の演出の間も true） */
	bool IsCharged() const { return bCharged; }

protected:
	virtual void OnRegister() override;
	virtual void OnUnregister() override;

private:
	ADoor* GetDoor() const;
	TArray<UMeshComponent*> GetDoorMeshes() const;

	/** 扉板を虹色に塗り替える / 元のマテリアルに戻す */
	void ApplySurface();
	void RestoreSurface();

	/** 行き先の扉と出てくる位置・向きを選ぶ。見つからなければ false */
	bool PickArrival(const APlayerCharacter* Player, FVector& OutLocation, FRotator& OutRotation) const;

	void Teleport();
	void FinishReveal();

	/** 普通の扉に戻す（光・マテリアル・ナビの遮断を戻して部品ごと消える） */
	void Revert();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UPointLightComponent>> Glows;

	/** 扉の開口をナビメッシュ上で塞ぐ箱（当たり判定なし） */
	UPROPERTY(Transient)
	TObjectPtr<UBoxComponent> NavBlocker;

	/** 塗り替える前のマテリアル（メッシュごと・スロットごと） */
	UPROPERTY(Transient)
	TMap<TObjectPtr<UMeshComponent>, FRainbowSavedMaterials> SavedMaterials;

	TWeakObjectPtr<APlayerCharacter> Traveler;
	FTimerHandle StepTimer;
	float Age = 0.0f;
	bool bCharged = true;
	bool bInUse = false;
};
