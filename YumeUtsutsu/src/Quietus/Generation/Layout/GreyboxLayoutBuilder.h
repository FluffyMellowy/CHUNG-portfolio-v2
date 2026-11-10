#pragma once

#include "CoreMinimal.h"
#include "Generation/Layout/LayoutBuilder.h"

namespace LevelGen
{
	/**
	 *  グレイボックス用レイアウト（プレハブ未設定時のフォールバック）。
	 *  可変サイズの矩形部屋を置き、部屋中心同士をL字の廊下で直結する。
	 *  部屋と廊下は仕切らず連続した空間になる。
	 */
	class FGreyboxLayoutBuilder : public FLayoutBuilder
	{
	public:
		using FLayoutBuilder::FLayoutBuilder;

		virtual bool TryBuild(FLayout& OutLayout, FRandomStream& Rand) const override;
		virtual const TCHAR* GetModeName() const override { return TEXT("greybox"); }

	private:
		bool PlaceRooms(FLayout& Layout, FRandomStream& Rand) const;
		void CarveCorridors(FLayout& Layout, FRandomStream& Rand) const;

		/** 開始部屋をランダムに選び、そこから最遠の部屋を脱出部屋にして外壁に扉位置を決める */
		bool PickStartAndExit(FLayout& Layout, FRandomStream& Rand) const;

		/** 開始部屋から全部屋の中心へ到達できるか */
		bool ValidateConnectivity(const FLayout& Layout) const;
	};
}
