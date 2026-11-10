#include "Generation/Layout/RoomGraph.h"

namespace LevelGen
{
	TArray<TPair<int32, int32>> BuildRoomConnections(const TArray<FRoom>& Rooms, int32 ExtraLoops)
	{
		TArray<TPair<int32, int32>> Connections;
		const int32 N = Rooms.Num();
		if (N == 0)
		{
			return Connections;
		}

		auto DistSq = [&Rooms](int32 A, int32 B)
		{
			const FIntPoint D = Rooms[A].Center() - Rooms[B].Center();
			return D.X * D.X + D.Y * D.Y;
		};

		// プリム法: 木に含まれる部屋から最も近い未接続の部屋を1つずつ足す
		TArray<bool> InTree;
		InTree.Init(false, N);
		InTree[0] = true;
		for (int32 Count = 1; Count < N; ++Count)
		{
			int32 BestA = -1, BestB = -1, BestDist = MAX_int32;
			for (int32 A = 0; A < N; ++A)
			{
				if (!InTree[A]) continue;
				for (int32 B = 0; B < N; ++B)
				{
					if (InTree[B]) continue;
					const int32 D = DistSq(A, B);
					if (D < BestDist)
					{
						BestDist = D;
						BestA = A;
						BestB = B;
					}
				}
			}
			InTree[BestB] = true;
			Connections.Emplace(BestA, BestB);
		}

		// MSTに含まれない辺から短い順にループを足す
		TArray<TPair<int32, int32>> Candidates;
		for (int32 A = 0; A < N; ++A)
		{
			for (int32 B = A + 1; B < N; ++B)
			{
				const bool bExists = Connections.ContainsByPredicate([A, B](const TPair<int32, int32>& C)
				{
					return (C.Key == A && C.Value == B) || (C.Key == B && C.Value == A);
				});
				if (!bExists)
				{
					Candidates.Emplace(A, B);
				}
			}
		}
		Candidates.Sort([&DistSq](const TPair<int32, int32>& L, const TPair<int32, int32>& R)
		{
			return DistSq(L.Key, L.Value) < DistSq(R.Key, R.Value);
		});
		for (int32 i = 0; i < FMath::Min(ExtraLoops, Candidates.Num()); ++i)
		{
			Connections.Add(Candidates[i]);
		}
		return Connections;
	}
}
