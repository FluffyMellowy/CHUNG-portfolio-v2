#include "Generation/Layout/GreyboxLayoutBuilder.h"
#include "Generation/Layout/RoomGraph.h"
#include "Quietus.h"

namespace LevelGen
{
	bool FGreyboxLayoutBuilder::TryBuild(FLayout& OutLayout, FRandomStream& Rand) const
	{
		OutLayout.Reset(Settings.GridSize);
		OutLayout.bSeparateRooms = false;

		if (!PlaceRooms(OutLayout, Rand))
		{
			return false;
		}
		OutLayout.Connections = BuildRoomConnections(OutLayout.Rooms, Settings.ExtraLoops);
		CarveCorridors(OutLayout, Rand);
		if (!PickStartAndExit(OutLayout, Rand))
		{
			return false;
		}
		return ValidateConnectivity(OutLayout);
	}

	bool FGreyboxLayoutBuilder::PlaceRooms(FLayout& Layout, FRandomStream& Rand) const
	{
		const int32 TargetCount = Rand.RandRange(Settings.RoomCountMin, Settings.RoomCountMax);

		int32 Tries = 0;
		while (Layout.Rooms.Num() < TargetCount && Tries++ < 300)
		{
			int32 W = Rand.RandRange(Settings.RoomSizeMin.X, Settings.RoomSizeMax.X);
			int32 H = Rand.RandRange(Settings.RoomSizeMin.Y, Settings.RoomSizeMax.Y);
			if (Rand.FRand() < 0.5f)
			{
				Swap(W, H); // 縦横をランダムに入れ替えてバリエーションを出す
			}
			if (!FitsInGrid(W, H))
			{
				continue;
			}
			TryCommitRoom(Layout, MakeRandomRoom(W, H, Rand));
		}
		return Layout.Rooms.Num() >= Settings.RoomCountMin;
	}

	void FGreyboxLayoutBuilder::CarveCorridors(FLayout& Layout, FRandomStream& Rand) const
	{
		for (const TPair<int32, int32>& Edge : Layout.Connections)
		{
			FIntPoint Cur = Layout.Rooms[Edge.Key].Center();
			const FIntPoint Goal = Layout.Rooms[Edge.Value].Center();

			// L字経路。曲がる順序はランダム
			const bool bXFirst = Rand.FRand() < 0.5f;
			auto WalkX = [&]()
			{
				while (Cur.X != Goal.X)
				{
					Cur.X += (Goal.X > Cur.X) ? 1 : -1;
					Layout.Grid.CarveCorridor(Cur);
				}
			};
			auto WalkY = [&]()
			{
				while (Cur.Y != Goal.Y)
				{
					Cur.Y += (Goal.Y > Cur.Y) ? 1 : -1;
					Layout.Grid.CarveCorridor(Cur);
				}
			};
			if (bXFirst) { WalkX(); WalkY(); }
			else         { WalkY(); WalkX(); }
		}
	}

	bool FGreyboxLayoutBuilder::PickStartAndExit(FLayout& Layout, FRandomStream& Rand) const
	{
		const int32 N = Layout.Rooms.Num();
		Layout.StartRoom = Rand.RandRange(0, N - 1);

		// 開始部屋から最遠（タイルBFS距離）の部屋を脱出部屋にする
		const TArray<int32> Dist = Layout.Grid.ComputeDistances(Layout.Rooms[Layout.StartRoom].Center());
		Layout.ExitRoom = Layout.StartRoom;
		int32 BestDist = -1;
		for (int32 i = 0; i < N; ++i)
		{
			const int32 D = Dist[Layout.Grid.ToIndex(Layout.Rooms[i].Center())];
			if (i != Layout.StartRoom && D > BestDist)
			{
				BestDist = D;
				Layout.ExitRoom = i;
			}
		}
		if (Layout.ExitRoom == Layout.StartRoom)
		{
			return false; // 到達可能な他の部屋がない＝レイアウト不成立
		}

		// 脱出部屋の外周から、外側に階段セットが通る空きが続く辺をランダムに選んで扉位置にする
		const FRoom& Exit = Layout.Rooms[Layout.ExitRoom];
		TArray<TPair<FIntPoint, FIntPoint>> Edges;
		for (int32 X = Exit.X; X < Exit.X + Exit.W; ++X)
		{
			for (int32 Y = Exit.Y; Y < Exit.Y + Exit.H; ++Y)
			{
				for (const FIntPoint& D : NeighborDirs)
				{
					const FIntPoint Outside(X + D.X, Y + D.Y);
					if (Layout.Grid.IsInBounds(Outside) && IsExitPathClear(Layout, FIntPoint(X, Y), D))
					{
						Edges.Emplace(FIntPoint(X, Y), D);
					}
				}
			}
		}
		if (Edges.Num() == 0)
		{
			return false;
		}
		const TPair<FIntPoint, FIntPoint>& Chosen = Edges[Rand.RandRange(0, Edges.Num() - 1)];
		Layout.ExitDoorCell = Chosen.Key;
		Layout.ExitDoorDir = Chosen.Value;
		ReserveExitPath(Layout);
		return true;
	}

	bool FGreyboxLayoutBuilder::ValidateConnectivity(const FLayout& Layout) const
	{
		const TArray<int32> Dist = Layout.Grid.ComputeDistances(Layout.Rooms[Layout.StartRoom].Center());
		for (int32 i = 0; i < Layout.Rooms.Num(); ++i)
		{
			if (Dist[Layout.Grid.ToIndex(Layout.Rooms[i].Center())] < 0)
			{
				UE_LOG(LogQuietus, Warning, TEXT("%s: room %d unreachable, retrying"), *LogOwner, i);
				return false;
			}
		}
		return true;
	}
}
