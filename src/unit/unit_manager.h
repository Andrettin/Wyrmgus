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
//      (c) Copyright 2007-2022 by Jimmy Salmon and Andrettin
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

#include "util/singleton.h"

class CUnit;
class CFile;
struct lua_State;

namespace wyrmgus {

class unit_ref;

class unit_manager final : public singleton<unit_manager>
{
public:
	unit_manager();
	~unit_manager();

	void init();

	void clean_units();

	CUnit *AllocUnit();
	void ReleaseUnit(CUnit *unit);
	void Save(CFile &file) const;
	void Load(lua_State *Lua);

	// Following is for already allocated Unit (no specific order)
	void Add(CUnit *unit);
	bool empty() const;

	const std::vector<CUnit *> &get_units() const
	{
		return this->units;
	}

	CUnit *lastCreatedUnit();

	// Following is mainly for scripting
	CUnit &GetSlotUnit(int index) const;
	unsigned int GetUsedSlotCount() const;

	void add_unit_seen_under_fog(CUnit *unit);
	void remove_unit_seen_under_fog(CUnit *unit);

private:
	//units currently in use
	std::vector<CUnit *> units;

	//all units, including released ones; the unit's index here is its slot
	std::vector<std::unique_ptr<CUnit>> unit_slots;

	std::list<CUnit *> released_units;
	CUnit *lastCreated = nullptr;

	//units seen under fog, which we need to keep references to in order to prevent them from being released
	std::map<const CUnit *, std::shared_ptr<unit_ref>> units_seen_under_fog;
};

}
