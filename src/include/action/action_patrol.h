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
//      (c) Copyright 1998-2022 by Lutz Sammer, Jimmy Salmon and Andrettin
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

class COrder_Patrol final : public COrder
{
	//Wyrmgus start
//	friend std::unique_ptr<COrder> COrder::NewActionPatrol(const Vec2i &currentPos, const Vec2i &dest);
	friend std::unique_ptr<COrder> COrder::NewActionPatrol(const Vec2i &currentPos, const Vec2i &dest, int current_z, int dest_z);
	//Wyrmgus end
public:
	//Wyrmgus start
//	COrder_Patrol() : COrder(UnitAction::Patrol), WaitingCycle(0), Range(0)
	COrder_Patrol() : COrder(UnitAction::Patrol), WaitingCycle(0), Range(0), MapLayer(0), WayPointMapLayer(0)
	//Wyrmgus end
	{
		goalPos.x = -1;
		goalPos.y = -1;
	}

	virtual std::unique_ptr<COrder> Clone() const override
	{
		return std::make_unique<COrder_Patrol>(*this);
	}

	virtual bool IsValid() const override;

	virtual void Save(CFile &file, const CUnit &unit) const override;
	virtual bool ParseSpecificData(lua_State *l, int &j, const char *value, const CUnit &unit) override;

	virtual void Execute(CUnit &unit) override;

	virtual PixelPos Show(const CViewport &vp, const PixelPos &lastScreenPos, std::vector<std::function<void(renderer *)>> &render_commands) const override;

	virtual void UpdatePathFinderData(PathFinderInput &input) override;

	const Vec2i &GetWayPoint() const { return WayPoint; }

private:
	Vec2i WayPoint; /// position for patroling.
	unsigned int WaitingCycle; /// number of cycle pathfinder wait.
	int Range;
	Vec2i goalPos;
	//Wyrmgus start
	int MapLayer;
	int WayPointMapLayer;
	//Wyrmgus end
};
