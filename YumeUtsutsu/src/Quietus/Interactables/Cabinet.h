#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interactables/CabinetLootTable.h"
#include "Interactables/Interactable.h"
#include "Cabinet.generated.h"

class UStaticMeshComponent;

/**
 *  検索できる収納キャビネット（GDD §13-②）。
 *  E で未捜索の引き出しを1段ずつ開け、ルートテーブル（バッテリー/投擲物/メモ/空）を
 *  抽選して引き出しの上にピックアップをスポーンする。全段捜索済みになるとプロンプトが消える。
 *  メッシュ規約: ボディ=底面中央ピボット・正面+X、引き出し=閉状態の前面下端中央ピボット・+Xへスライド。
 */
UCLASS()
class ACabinet : public AActor, public IInteractable
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	/** 下段・上段の引き出し */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> DrawerLower;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> DrawerUpper;

protected:

	/** 引き出しの最大スライド量 (cm)。メッシュ奥行48の~70% */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cabinet", meta = (ClampMin = 0, Units = "cm"))
	float SlideDistance = 34.0f;

	/** スライド速度 (cm/s) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cabinet", meta = (ClampMin = 1))
	float SlideSpeed = 90.0f;

	/** 生成器から渡される抽選表（生成プリセットのアイテム設定で調整する） */
	FCabinetLootTable LootTable;

	/** 生成器から渡されるルートのクラス（見た目は各クラスが持つ） */
	UPROPERTY(Transient) TSubclassOf<AActor> BatteryClass;
	UPROPERTY(Transient) TSubclassOf<AActor> ThrowableClass;
	UPROPERTY(Transient) TSubclassOf<AActor> NoteClass;
	FText CabinetNoteTitle;
	FText CabinetNoteBody;

	/** 抽選シード（生成器のストリームから分配され、シード再現性を保つ） */
	int32 LootSeed = 0;

	/** 次に開ける引き出し (0=下段, 1=上段, 2=全部捜索済み) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="Cabinet")
	int32 NextDrawer = 0;

	/** 各引き出しの目標スライド量（開けたら SlideDistance） */
	float DrawerTargets[2] = { 0.0f, 0.0f };

public:
	ACabinet();

	virtual void Tick(float DeltaSeconds) override;

	/** 生成器用: 抽選表・ルートのクラス・メモ本文・抽選シードを渡す */
	void ConfigureLoot(const FCabinetLootTable& InLootTable,
		TSubclassOf<AActor> InBattery, TSubclassOf<AActor> InThrowable, TSubclassOf<AActor> InNote,
		const FText& InNoteTitle, const FText& InNoteBody, int32 InSeed);

	//~ Begin IInteractable
	virtual void Interact_Implementation(APlayerCharacter* Interactor) override;
	virtual FText GetInteractionPrompt_Implementation() const override;
	//~ End IInteractable

protected:
	/** 引き出し1段を開けてルートを抽選・スポーンする */
	void OpenAndRoll(UStaticMeshComponent* Drawer);
};
