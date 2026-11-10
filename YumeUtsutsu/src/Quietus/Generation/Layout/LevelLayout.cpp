#include "Generation/Layout/LevelLayout.h"

namespace LevelGen
{
	uint64 EdgeKey(const FIntPoint& Cell, const FIntPoint& Dir)
	{
		// 辺を「両側セルの順序なしペア」で正規化してエンコードする
		FIntPoint A = Cell;
		FIntPoint B = Cell + Dir;
		if (B.X < A.X || (B.X == A.X && B.Y < A.Y))
		{
			Swap(A, B);
		}
		return (uint64)(uint16)A.X << 48 | (uint64)(uint16)A.Y << 32 | (uint64)(uint16)B.X << 16 | (uint64)(uint16)B.Y;
	}

	bool FRoom::OverlapsWithGap(const FRoom& Other, int32 Gap) const
	{
		return X < Other.X + Other.W + Gap && Other.X < X + W + Gap &&
			Y < Other.Y + Other.H + Gap && Other.Y < Y + H + Gap;
	}

	void FCellGrid::Reset(const FIntPoint& InSize)
	{
		Size = InSize;
		Cells.Init(ECell::Empty, Size.X * Size.Y);
		CellRoom.Init(INDEX_NONE, Size.X * Size.Y);
	}

	bool FCellGrid::IsInBounds(const FIntPoint& Cell) const
	{
		return Cell.X >= 0 && Cell.X < Size.X && Cell.Y >= 0 && Cell.Y < Size.Y;
	}

	ECell FCellGrid::Get(const FIntPoint& Cell) const
	{
		return IsInBounds(Cell) ? Cells[ToIndex(Cell)] : ECell::Empty;
	}

	void FCellGrid::Set(const FIntPoint& Cell, ECell Value)
	{
		if (IsInBounds(Cell))
		{
			Cells[ToIndex(Cell)] = Value;
		}
	}

	int32 FCellGrid::GetRoomAt(const FIntPoint& Cell) const
	{
		return IsInBounds(Cell) ? CellRoom[ToIndex(Cell)] : INDEX_NONE;
	}

	void FCellGrid::StampRoom(const FRoom& Room, int32 RoomIdx)
	{
		for (int32 X = Room.X; X < Room.X + Room.W; ++X)
		{
			for (int32 Y = Room.Y; Y < Room.Y + Room.H; ++Y)
			{
				const FIntPoint Cell(X, Y);
				Set(Cell, ECell::Room);
				if (IsInBounds(Cell))
				{
					CellRoom[ToIndex(Cell)] = (int16)RoomIdx;
				}
			}
		}
	}

	void FCellGrid::CarveCorridor(const FIntPoint& Cell)
	{
		if (Get(Cell) == ECell::Empty)
		{
			Set(Cell, ECell::Corridor);
		}
	}

	TArray<int32> FCellGrid::ComputeDistances(const FIntPoint& From) const
	{
		TArray<int32> Dist;
		Dist.Init(-1, Cells.Num());
		if (!IsInBounds(From))
		{
			return Dist;
		}

		TArray<FIntPoint> Queue;
		Dist[ToIndex(From)] = 0;
		Queue.Add(From);
		for (int32 Head = 0; Head < Queue.Num(); ++Head)
		{
			const FIntPoint C = Queue[Head];
			for (const FIntPoint& D : NeighborDirs)
			{
				const FIntPoint Next = C + D;
				if (IsFilled(Next) && Dist[ToIndex(Next)] < 0)
				{
					Dist[ToIndex(Next)] = Dist[ToIndex(C)] + 1;
					Queue.Add(Next);
				}
			}
		}
		return Dist;
	}

	void FLayout::Reset(const FIntPoint& GridSize)
	{
		Grid.Reset(GridSize);
		Rooms.Reset();
		Connections.Reset();
		StartRoom = 0;
		ExitRoom = 0;
		ExitDoorCell = FIntPoint::ZeroValue;
		ExitDoorDir = FIntPoint::ZeroValue;
		ExitStairCells.Reset();
		bSeparateRooms = false;
		DoorEdges.Reset();
		UsedDoorKeys.Reset();
	}
}
