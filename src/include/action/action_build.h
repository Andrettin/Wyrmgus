//       _________ __                 __
//      /   _____//  |_____________ _/  |______     ____  __ __  ______
//      \_____  \\   __\_  __ \__  \\   __\__  \   / ___\|  |  \/  ___/
//      /        \|  |  |  | \// __ \|  |  / __ \_/ /_/  >  |  /\___ |
//     /_______  /|__|  |__|  (____  /__| (____  /\___  /|____//____  >
//             \/                  \/          \//_____/            \/
//  ______________________                           ______________________
//                        T H E   W A R   B E G I N S
//         Stratagus - A free fantasy real time strategy game engine
//
/**@name action_build.h - The actions headerfile. */
//
//      (c) Copyright 1998-2012 by Lutz Sammer and Jimmy Salmon
//
//      This program is free software; you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation; only version 2 of the License.
//
//      This program is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU General Public License for more details.
//
//      You should have received a copy of the GNU General Public License
//      along with this program; if not, write to the Free Software
//      Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
//      02111-1307, USA.

#pragma once

#include "actions.h"

namespace wyrmgus {
	class site;
}

class COrder_Build final : public COrder
{
	friend std::unique_ptr<COrder> COrder::NewActionBuild(const CUnit &builder, const Vec2i &pos, const wyrmgus::unit_type &building, int z, const wyrmgus::site *settlement);
public:
	COrder_Build();
	virtual ~COrder_Build() override;

	virtual std::unique_ptr<COrder> Clone() const override
	{
		return std::make_unique<COrder_Build>(*this);
	}

	virtual bool IsValid() const override;

	virtual void Save(CFile &file, const CUnit &unit) const override;
	virtual bool ParseSpecificData(lua_State *l, int &j, const char *value, const CUnit &unit) override;

	virtual void Execute(CUnit &unit) override;
	virtual PixelPos Show(const CViewport &vp, const PixelPos &lastScreenPos) const override;
	virtual void UpdatePathFinderData(PathFinderInput &input) override;
	
	//Wyrmgus start
	void ConvertUnitType(const CUnit &unit, wyrmgus::unit_type &newType);
	//Wyrmgus end

	virtual void AiUnitKilled(CUnit &unit) override;

	const wyrmgus::unit_type &GetUnitType() const { return *Type; }
	virtual const Vec2i GetGoalPos() const override { return goalPos; }
	//Wyrmgus start
	virtual const int GetGoalMapLayer() const override { return MapLayer; }
	//Wyrmgus end

private:
	bool MoveToLocation(CUnit &unit);
	CUnit *CheckCanBuild(CUnit &unit) const;
	bool StartBuilding(CUnit &unit, CUnit &ontop);
	bool BuildFromOutside(CUnit &unit) const;
	void HelpBuild(CUnit &unit, CUnit &building);

	CUnit *get_building_unit() const;

private:
	const wyrmgus::unit_type *Type = nullptr;        /// build a unit of this unit-type
	std::shared_ptr<wyrmgus::unit_ref> BuildingUnit;  /// unit builded.
	int State = 0;
	int Range = 0;
	Vec2i goalPos;
	//Wyrmgus start
	int MapLayer = 0;
	const wyrmgus::site *settlement = nullptr;
	//Wyrmgus end
};
