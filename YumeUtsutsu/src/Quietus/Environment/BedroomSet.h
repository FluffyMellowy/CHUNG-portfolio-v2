#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BedroomSet.generated.h"

class ACameraActor;
class AStaticMeshActor;

/** 部屋のセットの場面 */
UENUM()
enum class EBedroomState : uint8
{
	/** タイトル。窓の冷たい光とスタンドの灯り */
	Night,
	/** 目覚め。窓の朝の光、スタンドは消える */
	Morning,
	/** 吹き出しの先の夢の中（難易度を選ぶ背景）。部屋から離れた場所に置いてある */
	Dream,
};

/**
 *  主人公の部屋（タイトルと目覚めの固定セット）の夜と朝、吹き出しの先の夢の背景を切り替える。
 *  メッシュとライトはレベルに直接置いてあり、ここは場面ごとの差分だけを持つ。
 *  配置は Tools/Bedroom/bedroom_map.py が組む。
 */
UCLASS()
class ABedroomSet : public AActor
{
	GENERATED_BODY()

public:
	ABedroomSet();

	void ApplyState(EBedroomState State);

	ACameraActor* GetCamera(EBedroomState State) const;

protected:
	/** 夜だけ出すもの（掛け布団、夜の灯り、夜空） */
	UPROPERTY(EditInstanceOnly, Category="Bedroom")
	TArray<TObjectPtr<AActor>> NightActors;

	/** 朝だけ出すもの（めくれた掛け布団、朝の灯り、朝の空） */
	UPROPERTY(EditInstanceOnly, Category="Bedroom")
	TArray<TObjectPtr<AActor>> MorningActors;

	/** 夢の背景だけで出すもの（ステージのキットで組んだ廊下とその灯り） */
	UPROPERTY(EditInstanceOnly, Category="Bedroom")
	TArray<TObjectPtr<AActor>> DreamActors;

	UPROPERTY(EditInstanceOnly, Category="Bedroom")
	TObjectPtr<ACameraActor> TitleCamera;

	UPROPERTY(EditInstanceOnly, Category="Bedroom")
	TObjectPtr<ACameraActor> MorningCamera;

	UPROPERTY(EditInstanceOnly, Category="Bedroom")
	TObjectPtr<ACameraActor> DreamCamera;

	/** 電球の発光を切り替えるスタンド */
	UPROPERTY(EditInstanceOnly, Category="Bedroom")
	TObjectPtr<AStaticMeshActor> Lamp;

	/** スタンドの電球のマテリアルスロット */
	UPROPERTY(EditAnywhere, Category="Bedroom")
	FName LampSlot = TEXT("M_Bedroom_LampEmit");

	/** 夜の電球の明るさ（マテリアルの EmissiveStrength） */
	UPROPERTY(EditAnywhere, Category="Bedroom", meta = (ClampMin = 0))
	float LampEmissionNight = 30.0f;
};
