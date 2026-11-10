#include "Core/DreamGimmicks.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"

UDreamGimmickSettings::UDreamGimmickSettings()
{
	RainbowSurfaceMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Interactables/RainbowDoor/M_RainbowDoor.M_RainbowDoor")));
	BalloonPopSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Interactables/Balloon/S_BalloonPop.S_BalloonPop")));

	// ゴム風船らしい少しくすんだ原色寄り
	BalloonPalette = {
		FLinearColor(0.55f, 0.04f, 0.04f),  // 赤
		FLinearColor(0.04f, 0.12f, 0.55f),  // 青
		FLinearColor(0.60f, 0.42f, 0.03f),  // 黄
		FLinearColor(0.05f, 0.35f, 0.10f),  // 緑
		FLinearColor(0.40f, 0.06f, 0.35f),  // 紫
		FLinearColor(0.75f, 0.70f, 0.62f),  // 白
	};
}
