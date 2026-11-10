#include "Generation/Construction/LightingBuilder.h"
#include "Components/PointLightComponent.h"
#include "Engine/PointLight.h"
#include "Generation/GenerationContext.h"

namespace LevelGen
{
	int32 FLightingBuilder::Build() const
	{
		if (!Settings.bSpawnRoomLights)
		{
			return 0;
		}

		int32 NumLights = 0;
		const float LightZ = CeilingHeight - Settings.LightDropFromCeiling;
		for (const FRoom& Room : Context.Layout.Rooms)
		{
			NumLights += SpawnLight(Context.CellCenter(Room.Center(), LightZ), Settings.RoomLightIntensity, Settings.RoomLightRadius);
		}

		// 廊下: 直線廊下では X+Y が1ずつ進むため、正確に Interval タイル毎になる
		if (Settings.CorridorLightIntensity > 0.0f)
		{
			const FCellGrid& Grid = Context.Layout.Grid;
			for (int32 X = 0; X < Grid.GetSize().X; ++X)
			{
				for (int32 Y = 0; Y < Grid.GetSize().Y; ++Y)
				{
					const FIntPoint Cell(X, Y);
					if (Grid.Get(Cell) == ECell::Corridor && (X + Y) % Settings.CorridorLightInterval == 0)
					{
						NumLights += SpawnLight(Context.CellCenter(Cell, LightZ), Settings.CorridorLightIntensity, Settings.CorridorLightRadius);
					}
				}
			}
		}
		return NumLights;
	}

	bool FLightingBuilder::SpawnLight(const FVector& Location, float Intensity, float Radius) const
	{
		APointLight* Light = Context.SpawnActor<APointLight>(APointLight::StaticClass(), Location, FRotator::ZeroRotator);
		if (!Light)
		{
			return false;
		}
		UPointLightComponent* Comp = CastChecked<UPointLightComponent>(Light->GetLightComponent());
		Comp->SetMobility(EComponentMobility::Movable);
		Comp->SetIntensity(Intensity);
		Comp->SetLightColor(Settings.RoomLightColor);
		Comp->SetAttenuationRadius(Radius);
		Comp->SetCastShadows(true);
		return true;
	}
}
