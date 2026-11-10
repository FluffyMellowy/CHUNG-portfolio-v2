#include "Generation/Spawn/GimmickSpawner.h"
#include "Core/DreamGimmicks.h"
#include "Generation/GenerationContext.h"
#include "Interactables/Balloon.h"
#include "Interactables/Door.h"
#include "Interactables/RainbowDoorComponent.h"

namespace LevelGen
{
	template <typename T>
	void FGimmickSpawner::Shuffle(TArray<T>& Items) const
	{
		for (int32 i = Items.Num() - 1; i > 0; --i)
		{
			Items.Swap(i, Context.Rand.RandRange(0, i));
		}
	}

	int32 FGimmickSpawner::SpawnRainbowDoors(const TArray<ADoor*>& Doors, int32 Count) const
	{
		TArray<ADoor*> Candidates;
		for (ADoor* Door : Doors)
		{
			if (Door && !Door->IsInExitRoom())
			{
				Candidates.Add(Door);
			}
		}
		Shuffle(Candidates);

		const int32 Num = FMath::Min(Count, Candidates.Num());
		for (int32 i = 0; i < Num; ++i)
		{
			ADoor* Door = Candidates[i];
			Door->ForceClose();
			URainbowDoorComponent* Rainbow = NewObject<URainbowDoorComponent>(Door, TEXT("RainbowDoor"));
			Rainbow->SetupAttachment(Door->GetRootComponent());
			Rainbow->RegisterComponent();
		}
		return Num;
	}

	int32 FGimmickSpawner::SpawnBalloons(int32 Count) const
	{
		if (!Settings.BalloonClass || Count <= 0)
		{
			return 0;
		}

		// 廊下のセル（出口の外の階段セットの通り道は除く）
		const FCellGrid& Grid = Context.Layout.Grid;
		TArray<FIntPoint> Cells;
		for (int32 X = 0; X < Grid.GetSize().X; ++X)
		{
			for (int32 Y = 0; Y < Grid.GetSize().Y; ++Y)
			{
				const FIntPoint Cell(X, Y);
				if (Grid.Get(Cell) == ECell::Corridor && !Context.Layout.ExitStairCells.Contains(Cell))
				{
					Cells.Add(Cell);
				}
			}
		}
		Shuffle(Cells);

		const TArray<FLinearColor>& Palette = UDreamGimmickSettings::Get().BalloonPalette;
		const float Jitter = Context.CellSize * 0.2f;
		int32 NumSpawned = 0;
		for (int32 i = 0; i < Cells.Num() && NumSpawned < Count; ++i)
		{
			const FVector Location = Context.CellCenter(Cells[i], Settings.BalloonFloatHeight)
				+ FVector(Context.Rand.FRandRange(-Jitter, Jitter), Context.Rand.FRandRange(-Jitter, Jitter), 0.0f);
			const FRotator Rotation(0.0f, Context.Rand.FRandRange(0.0f, 360.0f), 0.0f);
			ABalloon* Balloon = Context.SpawnActor<ABalloon>(Settings.BalloonClass, Location, Rotation);
			if (!Balloon)
			{
				continue;
			}
			if (Palette.Num() > 0)
			{
				Balloon->SetColor(Palette[Context.Rand.RandRange(0, Palette.Num() - 1)]);
			}
			++NumSpawned;
		}
		return NumSpawned;
	}
}
