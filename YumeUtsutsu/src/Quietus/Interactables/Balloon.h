#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Balloon.generated.h"

class USphereComponent;
class UStaticMeshComponent;

/**
 *  廊下に浮かぶ紐付きの風船。プレイヤーがぶつかるか、投げた物が当たると「パン」と割れて大きな音を立てる。
 *  その音は聴覚で追う赤いピエロを呼び寄せる（罠にも、遠くから注意を逸らす道具にもなる）。
 *  紐の下端が原点。ふわふわと揺れながら漂う。色は生成器がパレットから選ぶ。メッシュと大きさは BP で設定する。
 */
UCLASS()
class ABalloon : public AActor
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> Root;

	/** 風船と紐（原点 = 紐の下端）。揺れはこのメッシュを原点まわりに傾けて作る */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> Mesh;

	/** 割れる判定（風船の胴体に合わせる） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> PopTrigger;

protected:

	/** 胴体のマテリアルの色パラメータ名 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Balloon")
	FName ColorParameter = TEXT("Color");

	/** 胴体のマテリアルスロット番号 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Balloon")
	int32 BodyMaterialIndex = 0;

	/** 揺れの角度（度）と速さ */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Balloon")
	float SwayAngle = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Balloon")
	float SwaySpeed = 0.8f;

	/** 上下にふわっと漂う幅 (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Balloon", meta = (Units = "cm"))
	float BobAmplitude = 3.0f;

public:
	ABalloon();

	virtual void Tick(float DeltaSeconds) override;

	/** 胴体の色を決める（生成器がスポーン直後に呼ぶ） */
	void SetColor(const FLinearColor& Color);

	/** 割る。Instigator は音の主（プレイヤー）。ピエロはこの音をプレイヤーの立てた音として聞く */
	UFUNCTION(BlueprintCallable, Category="Balloon")
	void Pop(APawn* NoiseInstigator);

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnTriggerOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnTriggerHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		FVector NormalImpulse, const FHitResult& Hit);

private:
	FVector BaseMeshLocation = FVector::ZeroVector;
	float Age = 0.0f;
	float PhaseA = 0.0f;
	float PhaseB = 0.0f;
	bool bPopped = false;
};
