#include "Generation/Construction/GeometryBuilder.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Generation/GenerationContext.h"
#include "NavigationSystem.h"
#include "Quietus.h"

namespace LevelGen
{
	bool FGeometryBuilder::Build(TArray<FDoorway>& OutDoorways) const
	{
		// キットが揃っていない部分はキューブで代用するので、その場合は TileMesh が必須
		const bool bNeedsCube = !Settings.KitFloorMesh || !Settings.KitWallMesh;
		if (bNeedsCube && !Settings.TileMesh)
		{
			UE_LOG(LogQuietus, Error, TEXT("%s: TileMesh is required unless both KitFloorMesh and KitWallMesh are set"), *Context.LogOwner);
			return false;
		}
		AssignMeshes();

		const FCellGrid& Grid = Context.Layout.Grid;
		for (int32 X = 0; X < Grid.GetSize().X; ++X)
		{
			for (int32 Y = 0; Y < Grid.GetSize().Y; ++Y)
			{
				const FIntPoint Cell(X, Y);
				if (!Grid.IsFilled(Cell))
				{
					continue;
				}
				AddFloorAndCeiling(Cell);
				AddWallsAround(Cell, OutDoorways);
			}
		}
		AddCeilingSkirt();

		// コンポーネント登録後にインスタンスを足しているため、ナビオクツリーへ明示的に反映させる
		// （これがないと動的NavMeshが生成床を認識せず、AIが移動できない）
		for (UInstancedStaticMeshComponent* ISM : Components.All())
		{
			UNavigationSystemV1::UpdateComponentInNavOctree(*ISM);
		}
		return true;
	}

	void FGeometryBuilder::AssignMeshes() const
	{
		if (Settings.TileMesh)
		{
			Components.CubeFloor->SetStaticMesh(Settings.TileMesh);
			Components.CubeWall->SetStaticMesh(Settings.TileMesh);
			if (Settings.FloorMaterial)
			{
				Components.CubeFloor->SetMaterial(0, Settings.FloorMaterial);
			}
			if (Settings.WallMaterial)
			{
				Components.CubeWall->SetMaterial(0, Settings.WallMaterial);
			}
		}
		if (Settings.KitFloorMesh)
		{
			Components.KitFloor->SetStaticMesh(Settings.KitFloorMesh);
		}
		if (Settings.KitWallMesh)
		{
			Components.KitWall->SetStaticMesh(Settings.KitWallMesh);
		}
		if (Settings.KitWallDoorMesh)
		{
			Components.KitWallDoor->SetStaticMesh(Settings.KitWallDoorMesh);
		}
		if (Settings.KitCeilingMesh)
		{
			Components.KitCeiling->SetStaticMesh(Settings.KitCeilingMesh);
		}
	}

	void FGeometryBuilder::AddFloorAndCeiling(const FIntPoint& Cell) const
	{
		const FVector Center = Context.CellCenter(Cell);

		// 床: キット床ピースを1枚（タイルサイズ・ピボット中央）、無ければ上面がZ=0になるよう沈めたキューブ
		if (Settings.KitFloorMesh)
		{
			Components.KitFloor->AddInstance(FTransform(FRotator::ZeroRotator, Center, FVector::OneVector), /*bWorldSpace=*/ true);
		}
		else
		{
			const float TileScale = Context.CellSize / 100.0f;
			Components.CubeFloor->AddInstance(FTransform(FRotator::ZeroRotator,
				Center + FVector(0, 0, -10.0f), FVector(TileScale, TileScale, 0.2f)), /*bWorldSpace=*/ true);
		}

		// 天井: 床と同じ規約のピースを壁上端の高さに1枚
		if (Settings.KitCeilingMesh)
		{
			Components.KitCeiling->AddInstance(FTransform(FRotator::ZeroRotator,
				Center + FVector(0, 0, Settings.WallHeight), FVector::OneVector), /*bWorldSpace=*/ true);
		}
	}

	void FGeometryBuilder::AddWallsAround(const FIntPoint& Cell, TArray<FDoorway>& OutDoorways) const
	{
		const FLayout& Layout = Context.Layout;
		for (const FIntPoint& D : NeighborDirs)
		{
			const FIntPoint Next = Cell + D;
			// 脱出扉の辺は扉アクターが開口付きの壁ごと受け持つ
			if (Cell == Layout.ExitDoorCell && D == Layout.ExitDoorDir)
			{
				continue;
			}
			if (!Layout.Grid.IsFilled(Next))
			{
				AddWallEdge(Cell, D, /*bDoorEdge=*/ false, OutDoorways);
				continue;
			}
			if (!Layout.bSeparateRooms || D.X < 0 || D.Y < 0)
			{
				continue; // 内部辺は仕切る場合だけ、正方向側の走査で1回だけ処理
			}
			if (Layout.Grid.GetRoomAt(Cell) == Layout.Grid.GetRoomAt(Next))
			{
				continue; // 同じ部屋の内部、または廊下同士
			}
			AddWallEdge(Cell, D, Layout.UsedDoorKeys.Contains(EdgeKey(Cell, D)), OutDoorways);
		}
	}

	void FGeometryBuilder::AddWallEdge(const FIntPoint& Cell, const FIntPoint& Dir, bool bDoorEdge, TArray<FDoorway>& OutDoorways) const
	{
		const FVector EdgeCenter = Context.EdgeCenter(Cell, Dir);

		if (Settings.KitWallMesh)
		{
			// キットモジュールを辺に沿って並べる。扉辺は中央モジュールだけ扉付きピースにする
			const FVector EdgeAxis(-Dir.Y, Dir.X, 0);
			const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(EdgeAxis.Y, EdgeAxis.X));
			const int32 NumModules = FMath::Max(1, FMath::RoundToInt(Context.CellSize / Settings.KitModuleSize));
			for (int32 m = 0; m < NumModules; ++m)
			{
				const float Offset = (m + 0.5f) * Settings.KitModuleSize - Context.CellSize * 0.5f;
				const FVector Pos = EdgeCenter + EdgeAxis * Offset;
				const bool bDoorModule = bDoorEdge && m == NumModules / 2;
				UInstancedStaticMeshComponent* ISM = bDoorModule ? Components.KitWallDoor : Components.KitWall;
				// 壁ピースは両面ジオメトリなので1枚でよい
				ISM->AddInstance(FTransform(FRotator(0, Yaw, 0), Pos, FVector::OneVector), /*bWorldSpace=*/ true);
				if (bDoorModule)
				{
					OutDoorways.Add({ EdgeKey(Cell, Dir), FTransform(FRotator(0, Yaw, 0), Pos) });
				}
			}
			return;
		}

		// キューブ: 扉辺は開口のまま、それ以外は1枚壁
		if (bDoorEdge)
		{
			return;
		}
		const float TileScale = Context.CellSize / 100.0f;
		const FVector WallScale = (Dir.X != 0)
			? FVector(0.2f, TileScale, Settings.WallHeight / 100.0f)
			: FVector(TileScale, 0.2f, Settings.WallHeight / 100.0f);
		Components.CubeWall->AddInstance(FTransform(FRotator::ZeroRotator,
			EdgeCenter + FVector(0, 0, Settings.WallHeight * 0.5f), WallScale), /*bWorldSpace=*/ true);
	}

	void FGeometryBuilder::AddCeilingSkirt() const
	{
		if (!Settings.KitCeilingMesh)
		{
			return;
		}
		const FCellGrid& Grid = Context.Layout.Grid;
		for (int32 X = 0; X < Grid.GetSize().X; ++X)
		{
			for (int32 Y = 0; Y < Grid.GetSize().Y; ++Y)
			{
				// 階段セットの上は空けておく（上り切ると天井の高さを超える）
				if (Grid.IsFilled(FIntPoint(X, Y)) || Context.Layout.ExitStairCells.Contains(FIntPoint(X, Y)))
				{
					continue;
				}
				bool bNearFilled = false;
				for (int32 dx = -1; dx <= 1 && !bNearFilled; ++dx)
				{
					for (int32 dy = -1; dy <= 1 && !bNearFilled; ++dy)
					{
						bNearFilled = (dx != 0 || dy != 0) && Grid.IsFilled(FIntPoint(X + dx, Y + dy));
					}
				}
				if (bNearFilled)
				{
					Components.KitCeiling->AddInstance(FTransform(FRotator::ZeroRotator,
						Context.CellCenter(FIntPoint(X, Y), Settings.WallHeight), FVector::OneVector), /*bWorldSpace=*/ true);
				}
			}
		}
	}
}
