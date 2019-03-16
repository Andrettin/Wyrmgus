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
/**@name action_patrol.h - The actions headerfile. */
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
//

#ifndef __ACTION_PATROL_H__
#define __ACTION_PATROL_H__

#include "action/actions.h"

class COrder_Patrol : public COrder
{
	//Wyrmgus start
//	friend COrder *COrder::NewActionPatrol(const Vec2i &currentPos, const Vec2i &dest);
	friend COrder *COrder::NewActionPatrol(const Vec2i &currentPos, const Vec2i &dest, int current_z, int dest_z);
	//Wyrmgus end
public:
	COrder_Patrol() : COrder(UnitActionPatrol)
	{
	}

	virtual COrder_Patrol *Clone() const { return new COrder_Patrol(*this); }

	virtual bool IsValid() const;

	virtual void Save(CFile &file, const CUnit &unit) const;
	virtual bool ParseSpecificData(lua_State *l, int &j, const char *value, const CUnit &unit);

	virtual void Execute(CUnit &unit);
	virtual PixelPos Show(const CViewport &vp, const PixelPos &lastScreenPos) const;
	virtual void UpdatePathFinderData(PathFinderInput &input);

	const Vec2i &GetWayPoint() const { return WayPoint; }
private:
	Vec2i WayPoint = Vec2i(-1, -1);	/// position for patroling.
	unsigned int WaitingCycle = 0;	/// number of cycle pathfinder wait.
	int Range = 0;
	Vec2i goalPos = Vec2i(-1, -1);
	//Wyrmgus start
	int MapLayer = 0;
	int WayPointMapLayer = 0;
	//Wyrmgus end
};

#endif
