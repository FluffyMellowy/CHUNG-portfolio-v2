#include "Generation/Layout/LayoutBuilder.h"

namespace LevelGen
{
	bool FLayoutBuilder::FitsInGrid(int32 W, int32 H) const
	{
		return W >= 1 && H >= 1 && Settings.GridSize.X - W - 2 >= 1 && Settings.GridSize.Y - H - 2 >= 1;
	}

	FRoom FLayoutBuilder::MakeRandomRoom(int32 W, int32 H, FRandomStream& Rand) const
	{
		FRoom Room;
		Room.W = W;
		Room.H = H;
		Room.X = Rand.RandRange(1, Settings.GridSize.X - W - 1);
		Room.Y = Rand.RandRange(1, Settings.GridSize.Y - H - 1);
		return Room;
	}

	bool FLayoutBuilder::TryCommitRoom(FLayout& Layout, const FRoom& Candidate) const
	{
		for (const FRoom& Other : Layout.Rooms)
		{
			if (Candidate.OverlapsWithGap(Other, Settings.RoomGap))
			{
				return false;
			}
		}
		Layout.Grid.StampRoom(Candidate, Layout.Rooms.Num());
		Layout.Rooms.Add(Candidate);
		return true;
	}

	bool FLayoutBuilder::IsExitPathClear(const FLayout& Layout, const FIntPoint& Cell, const FIntPoint& Dir) const
	{
		for (int32 k = 1; k <= Settings.ExitClearCells; ++k)
		{
			if (Layout.Grid.Get(Cell + Dir * k) != ECell::Empty)
			{
				return false;
			}
		}
		return true;
	}

	void FLayoutBuilder::ReserveExitPath(FLayout& Layout) const
	{
		Layout.ExitStairCells.Reset();
		for (int32 k = 1; k <= Settings.ExitClearCells; ++k)
		{
			Layout.ExitStairCells.Add(Layout.ExitDoorCell + Layout.ExitDoorDir * k);
		}
	}
}
