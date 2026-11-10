#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WakeStairs.generated.h"

class AAlarmClock;
class UBoxComponent;
class UStaticMesh;
class UStaticMeshComponent;

/** 階段セットの1ピース（階段・踊り場・頂上）。位置はアクター原点からの相対 */
USTRUCT(BlueprintType)
struct FWakeStairModule
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UStaticMesh> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector Location = FVector::ZeroVector;
};

/**
 *  脱出扉の先にある「光の階段」。宙に浮いた階段を上った先のテーブルに目覚まし時計がある。
 *  - 入口の踊り場に入ると安全地帯（以後、敵は追ってこない）
 *  - 手すりは無いので、各ピースの両脇と終端に見えない壁を立てて落下を防ぐ
 *  - ピースごとに上から照らす灯りを置く
 *
 *  座標: 原点 = 脱出扉の開口中央の床、+X = 階段を上る向き（扉の裏側）。
 *  ピースの並びとメッシュは BP で設定する（納品の assembly.json の配置）。
 */
UCLASS()
class AWakeStairs : public AActor
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> Root;

	/** 安全地帯の判定（入口の踊り場） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> SafeZone;

	/** 目覚まし時計を置くテーブル（ソケット AlarmClock） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> Table;

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="WakeStairs")
	TArray<FWakeStairModule> Modules;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="WakeStairs")
	TSubclassOf<AAlarmClock> ClockClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="WakeStairs")
	FName ClockSocket = TEXT("AlarmClock");

	/** 見えない壁の高さ（ピースの上面から, cm） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="WakeStairs|Barrier", meta = (ClampMin = 0, Units = "cm"))
	float BarrierHeight = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="WakeStairs|Barrier", meta = (ClampMin = 1, Units = "cm"))
	float BarrierThickness = 10.0f;

	/** 灯りの光量 (cd)・色・ピース上面からの高さ */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="WakeStairs|Light", meta = (ClampMin = 0))
	float LightIntensity = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="WakeStairs|Light")
	FLinearColor LightColor = FLinearColor(1.0f, 0.93f, 0.82f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="WakeStairs|Light", meta = (Units = "cm"))
	float LightHeight = 350.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="WakeStairs|Light", meta = (ClampMin = 0, Units = "cm"))
	float LightRadius = 900.0f;

public:
	AWakeStairs();

	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnSafeZoneOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	/** ローカル座標の箱で見えない壁を足す */
	void AddBarrier(const FVector& Min, const FVector& Max);
};
