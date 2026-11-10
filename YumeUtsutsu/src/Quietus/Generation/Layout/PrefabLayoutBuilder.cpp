#include "Generation/Layout/PrefabLayoutBuilder.h"
#include "Generation/Layout/RoomGraph.h"
#include "Generation/Layout/RoomPrefabCatalog.h"
#include "Quietus.h"

namespace LevelGen
{
	bool FPrefabLayoutBuilder::TryBuild(FLayout& OutLayout, FRandomStream& Rand) const
	{
		OutLayout.Reset(Settings.GridSize);
		OutLayout.bSeparateRooms = true;

		return PlaceRooms(OutLayout, Rand)
			&& CollectDoorEdges(OutLayout)
			&& ConnectRooms(OutLayout)
			&& PickStartAndExit(OutLayout, Rand);
	}

	bool FPrefabLayoutBuilder::TryPlacePrefab(FLayout& Layout, int32 PrefabIdx, TArray<int32>& PlacedPerPrefab, FRandomStream& Rand) const
	{
		const FPrefabEntry& Entry = Catalog.Get(PrefabIdx);
		if (!Entry.IsValid())
		{
			return false;
		}
		// テーマ毎の配置上限（開始・脱出は1を想定）
		if (Entry.MaxInstances > 0 && PlacedPerPrefab[PrefabIdx] >= Entry.MaxInstances)
		{
			return false;
		}
		if (!FitsInGrid(Entry.Footprint.X, Entry.Footprint.Y))
		{
			return false;
		}

		FRoom Candidate = MakeRandomRoom(Entry.Footprint.X, Entry.Footprint.Y, Rand);
		Candidate.PrefabIndex = PrefabIdx;
		if (!TryCommitRoom(Layout, Candidate))
		{
			return false;
		}
		++PlacedPerPrefab[PrefabIdx];
		return true;
	}

	bool FPrefabLayoutBuilder::PlaceRooms(FLayout& Layout, FRandomStream& Rand) const
	{
		const int32 TargetCount = Rand.RandRange(Settings.RoomCountMin, Settings.RoomCountMax);

		TArray<int32> PlacedPerPrefab;
		PlacedPerPrefab.Init(0, Catalog.Num());

		// 必須プレハブ: 開始候補・脱出候補を必ず1部屋ずつ先に確保する。
		// これが無いと「Startプレハブ不在→任意の部屋が開始部屋に降格」
		// 「Exitプレハブ不在→レイアウト不成立で全リトライ消費」が起きる
		int32 MandStart = INDEX_NONE, MandExit = INDEX_NONE;
		for (int32 i = 0; i < Catalog.Num(); ++i)
		{
			const FPrefabEntry& Entry = Catalog.Get(i);
			if (!Entry.IsValid())
			{
				continue;
			}
			if (MandStart == INDEX_NONE && Entry.bCanBeStart)
			{
				MandStart = i;
			}
			else if (MandExit == INDEX_NONE && Entry.bCanBeExit)
			{
				MandExit = i;
			}
		}
		for (const int32 MandIdx : { MandStart, MandExit })
		{
			if (MandIdx == INDEX_NONE)
			{
				continue; // プール構成上その候補が無い場合は通常の抽選とリトライに任せる
			}
			bool bPlaced = false;
			for (int32 T = 0; T < 100 && !bPlaced; ++T)
			{
				bPlaced = TryPlacePrefab(Layout, MandIdx, PlacedPerPrefab, Rand);
			}
			if (!bPlaced)
			{
				return false; // 必須部屋が置けない盤面はシード再試行へ
			}
		}

		int32 Tries = 0;
		while (Layout.Rooms.Num() < TargetCount && Tries++ < 300)
		{
			TryPlacePrefab(Layout, Rand.RandRange(0, Catalog.Num() - 1), PlacedPerPrefab, Rand);
		}
		return Layout.Rooms.Num() >= Settings.RoomCountMin;
	}

	bool FPrefabLayoutBuilder::CollectDoorEdges(FLayout& Layout) const
	{
		const float CellSize = Settings.CellSize;

		for (int32 i = 0; i < Layout.Rooms.Num(); ++i)
		{
			const FRoom& Room = Layout.Rooms[i];
			const FPrefabEntry& Entry = Catalog.Get(Room.PrefabIndex);
			const FVector RoomOrigin(Room.X * CellSize, Room.Y * CellSize, 0.0f);

			int32 NumValid = 0;
			for (const FPrefabSocket& Socket : Entry.Sockets)
			{
				// ソケットの外向きを軸方向に丸める
				FIntPoint Dir(0, 0);
				if (FMath::Abs(Socket.Forward.X) >= FMath::Abs(Socket.Forward.Y))
				{
					Dir.X = Socket.Forward.X >= 0 ? 1 : -1;
				}
				else
				{
					Dir.Y = Socket.Forward.Y >= 0 ? 1 : -1;
				}

				// ソケット位置から少し内側へ引いた点が属するセル＝部屋側セル
				const FVector Probe = RoomOrigin + Socket.LocalLocation - FVector(Dir.X, Dir.Y, 0) * (CellSize * 0.25f);
				const FIntPoint Inside(FMath::FloorToInt(Probe.X / CellSize), FMath::FloorToInt(Probe.Y / CellSize));
				const FIntPoint Outside = Inside + Dir;

				const bool bInsideOk = Room.Contains(Inside);
				const bool bOutsideOk = Layout.Grid.IsInBounds(Outside) && Layout.Grid.Get(Outside) == ECell::Empty;
				if (!bInsideOk || !bOutsideOk)
				{
					UE_LOG(LogQuietus, Warning, TEXT("%s: door socket '%s' on room %d maps to invalid edge (%d,%d)->(%d,%d)"),
						*LogOwner, *Socket.Name, i, Inside.X, Inside.Y, Outside.X, Outside.Y);
					continue;
				}

				FDoorEdge& Edge = Layout.DoorEdges.AddDefaulted_GetRef();
				Edge.Inside = Inside;
				Edge.Outside = Outside;
				Edge.Dir = Dir;
				Edge.RoomIdx = i;
				++NumValid;
			}
			if (NumValid == 0)
			{
				UE_LOG(LogQuietus, Warning, TEXT("%s: room %d (%s) has no usable door sockets"),
					*LogOwner, i, *Entry.ThemeName.ToString());
				return false;
			}
		}
		return true;
	}

	bool FPrefabLayoutBuilder::FindCorridorPath(const FCellGrid& Grid, const FIntPoint& From, const FIntPoint& To, TArray<FIntPoint>& OutPath)
	{
		auto Passable = [&Grid](const FIntPoint& C)
		{
			if (!Grid.IsInBounds(C))
			{
				return false;
			}
			const ECell Cell = Grid.Get(C);
			return Cell == ECell::Empty || Cell == ECell::Corridor;
		};
		if (!Passable(From) || !Passable(To))
		{
			return false;
		}

		TMap<FIntPoint, FIntPoint> Parent;
		TArray<FIntPoint> Queue;
		Parent.Add(From, From);
		Queue.Add(From);
		for (int32 Head = 0; Head < Queue.Num(); ++Head)
		{
			const FIntPoint C = Queue[Head];
			if (C == To)
			{
				break;
			}
			for (const FIntPoint& D : NeighborDirs)
			{
				const FIntPoint Next = C + D;
				if (Passable(Next) && !Parent.Contains(Next))
				{
					Parent.Add(Next, C);
					Queue.Add(Next);
				}
			}
		}
		if (!Parent.Contains(To))
		{
			return false;
		}

		OutPath.Reset();
		for (FIntPoint C = To; ; C = Parent[C])
		{
			OutPath.Add(C);
			if (C == From)
			{
				break;
			}
		}
		return true;
	}

	bool FPrefabLayoutBuilder::ConnectRooms(FLayout& Layout) const
	{
		Layout.Connections = BuildRoomConnections(Layout.Rooms, Settings.ExtraLoops);
		TArray<FDoorEdge>& DoorEdges = Layout.DoorEdges;

		for (const TPair<int32, int32>& Connection : Layout.Connections)
		{
			// 両部屋の扉の組を「未使用優先 → 外側セル間の距離順」で試す
			struct FPairCand { int32 A; int32 B; int32 Score; };
			TArray<FPairCand> Cands;
			for (int32 a = 0; a < DoorEdges.Num(); ++a)
			{
				if (DoorEdges[a].RoomIdx != Connection.Key)
				{
					continue;
				}
				for (int32 b = 0; b < DoorEdges.Num(); ++b)
				{
					if (DoorEdges[b].RoomIdx != Connection.Value)
					{
						continue;
					}
					const int32 Dist = FMath::Abs(DoorEdges[a].Outside.X - DoorEdges[b].Outside.X)
						+ FMath::Abs(DoorEdges[a].Outside.Y - DoorEdges[b].Outside.Y);
					const int32 UsedPenalty = (DoorEdges[a].bUsed ? 100 : 0) + (DoorEdges[b].bUsed ? 100 : 0);
					Cands.Add({ a, b, Dist + UsedPenalty });
				}
			}
			Cands.Sort([](const FPairCand& L, const FPairCand& R) { return L.Score < R.Score; });

			bool bConnected = false;
			const int32 MaxPairTries = FMath::Min(6, Cands.Num());
			for (int32 t = 0; t < MaxPairTries; ++t)
			{
				FDoorEdge& EA = DoorEdges[Cands[t].A];
				FDoorEdge& EB = DoorEdges[Cands[t].B];
				TArray<FIntPoint> Path;
				if (!FindCorridorPath(Layout.Grid, EA.Outside, EB.Outside, Path))
				{
					continue;
				}
				for (const FIntPoint& C : Path)
				{
					Layout.Grid.CarveCorridor(C);
				}
				EA.bUsed = true;
				EB.bUsed = true;
				Layout.UsedDoorKeys.Add(EA.Key());
				Layout.UsedDoorKeys.Add(EB.Key());
				bConnected = true;
				break;
			}
			if (!bConnected)
			{
				UE_LOG(LogQuietus, Warning, TEXT("%s: failed to connect rooms %d-%d, retrying layout"),
					*LogOwner, Connection.Key, Connection.Value);
				return false;
			}
		}
		return true;
	}

	bool FPrefabLayoutBuilder::PickStartAndExit(FLayout& Layout, FRandomStream& Rand) const
	{
		const int32 N = Layout.Rooms.Num();
		auto EntryOf = [&](int32 RoomIdx) -> const FPrefabEntry& { return Catalog.Get(Layout.Rooms[RoomIdx].PrefabIndex); };

		// 開始部屋: 候補フラグ付きの中からランダム（候補が無ければ全部屋から）
		TArray<int32> StartCands;
		for (int32 i = 0; i < N; ++i)
		{
			if (EntryOf(i).bCanBeStart)
			{
				StartCands.Add(i);
			}
		}
		if (StartCands.Num() == 0)
		{
			for (int32 i = 0; i < N; ++i)
			{
				StartCands.Add(i);
			}
		}
		Layout.StartRoom = StartCands[Rand.RandRange(0, StartCands.Num() - 1)];

		// 脱出部屋: 開始部屋から最遠（中心距離）の候補
		Layout.ExitRoom = INDEX_NONE;
		int32 BestDist = -1;
		const FIntPoint StartCenter = Layout.Rooms[Layout.StartRoom].Center();
		for (int32 i = 0; i < N; ++i)
		{
			if (i == Layout.StartRoom || !EntryOf(i).bCanBeExit)
			{
				continue;
			}
			const FIntPoint D = Layout.Rooms[i].Center() - StartCenter;
			const int32 Dist = D.X * D.X + D.Y * D.Y;
			if (Dist > BestDist)
			{
				BestDist = Dist;
				Layout.ExitRoom = i;
			}
		}
		if (Layout.ExitRoom == INDEX_NONE)
		{
			return false;
		}

		// 脱出扉: 脱出部屋の未使用扉のうち、外側に階段セットが通る空きが続くもの
		TArray<int32> ExitCands;
		for (int32 e = 0; e < Layout.DoorEdges.Num(); ++e)
		{
			const FDoorEdge& Edge = Layout.DoorEdges[e];
			if (Edge.RoomIdx == Layout.ExitRoom && !Edge.bUsed && IsExitPathClear(Layout, Edge.Inside, Edge.Dir))
			{
				ExitCands.Add(e);
			}
		}
		if (ExitCands.Num() == 0)
		{
			UE_LOG(LogQuietus, Warning, TEXT("%s: exit room %d has no free door socket, retrying layout"), *LogOwner, Layout.ExitRoom);
			return false;
		}
		FDoorEdge& ExitEdge = Layout.DoorEdges[ExitCands[Rand.RandRange(0, ExitCands.Num() - 1)]];
		ExitEdge.bUsed = true;
		Layout.UsedDoorKeys.Add(ExitEdge.Key());
		Layout.ExitDoorCell = ExitEdge.Inside;
		Layout.ExitDoorDir = ExitEdge.Dir;
		ReserveExitPath(Layout);
		return true;
	}
}
