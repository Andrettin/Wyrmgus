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
//      (c) Copyright 2007-2020 by Jimmy Salmon and Andrettin
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

#include "stratagus.h"

//Wyrmgus start
#include "character.h"
//Wyrmgus end
#include "unit/unit_manager.h"
#include "unit/unit.h"
#include "iolib.h"
#include "script.h"

namespace wyrmgus {

/**
**  Initial memory allocation for units.
*/
void unit_manager::Init()
{
	this->lastCreated = nullptr;
	//Assert(units.empty());
	this->units.clear();
	// Release memory of units in release list.
	while (!this->released_units.empty()) {
		CUnit *unit = this->released_units.front();
		this->released_units.pop_front();
		delete unit;
	}

	// Initialize the free unit slots
	this->unitSlots.clear();
}

/**
**  Allocate a new unit
**
**  @return  New unit
*/
CUnit *unit_manager::AllocUnit()
{
	// Can use released unit?
	if (!this->released_units.empty() && this->released_units.front()->ReleaseCycle < GameCycle) {
		CUnit *unit = this->released_units.front();
		this->released_units.pop_front();
		const int slot = unit->UnitManagerData.slot;
		unit->Init();
		unit->UnitManagerData.slot = slot;
		unit->UnitManagerData.unitSlot = -1;
		return unit;
	} else {
		auto unit = std::make_unique<CUnit>();

		unit->UnitManagerData.slot = unitSlots.size();
		unitSlots.push_back(unit.get());
		return unit.release();
	}
}

/**
**  Release a unit
**
**  @param unit  Unit to release
*/
void unit_manager::ReleaseUnit(CUnit *unit)
{
	Assert(unit);

	if (lastCreated == unit) {
		lastCreated = nullptr;
	}
	if (unit->UnitManagerData.unitSlot != -1) { // == -1 when loading.
		Assert(this->units[unit->UnitManagerData.unitSlot] == unit);

		CUnit *temp = this->units.back();
		temp->UnitManagerData.unitSlot = unit->UnitManagerData.unitSlot;
		this->units[unit->UnitManagerData.unitSlot] = temp;
		unit->UnitManagerData.unitSlot = -1;
		this->units.pop_back();
	}
	this->released_units.push_back(unit);
	unit->ReleaseCycle = GameCycle + 500; // can be reused after this time
	//Refs = GameCycle + (NetworkMaxLag << 1); // could be reuse after this time
}

CUnit &unit_manager::GetSlotUnit(int index) const
{
	return *unitSlots[index];
}

unsigned int unit_manager::GetUsedSlotCount() const
{
	return static_cast<unsigned int>(unitSlots.size());
}

bool unit_manager::empty() const
{
	return units.empty();
}

CUnit *unit_manager::lastCreatedUnit()
{
	return this->lastCreated;
}

void unit_manager::Add(CUnit *unit)
{
	lastCreated = unit;
	unit->UnitManagerData.unitSlot = static_cast<int>(units.size());
	units.push_back(unit);
}

/**
**  Save state of unit manager to file.
**
**  @param file  Output file.
*/
void unit_manager::Save(CFile &file) const
{
	file.printf("SlotUsage(%lu, {", (long unsigned int)unitSlots.size());

	for (const CUnit *unit : this->released_units) {
		file.printf("{Slot = %d, FreeCycle = %u}, ", UnitNumber(*unit), unit->ReleaseCycle);
	}
	//Wyrmgus start
	//add items owned by persistent heroes here, as if they were released
	for (std::vector<CUnit *>::const_iterator it = units.begin(); it != units.end(); ++it) {
		const CUnit &unit = **it;
		if (unit.Container && unit.Container->Character && unit.Container->HasInventory() && unit.Type->BoolFlag[ITEM_INDEX].value) { // don't save items for persistent heroes
			file.printf("{Slot = %d, FreeCycle = %u}, ", UnitNumber(unit), (unsigned int) GameCycle);
		}
	}
	//Wyrmgus end
	file.printf("})\n");

	for (std::vector<CUnit *>::const_iterator it = units.begin(); it != units.end(); ++it) {
		const CUnit &unit = **it;
		//Wyrmgus start
		if (unit.Container && unit.Container->Character && unit.Container->HasInventory() && unit.Type->BoolFlag[ITEM_INDEX].value) { // don't save items for persistent heroes
			continue;
		}
		//Wyrmgus end
		SaveUnit(unit, file);
	}
}

void unit_manager::Load(lua_State *l)
{
	Init();
	if (lua_gettop(l) != 2) {
		return;
	}
	unsigned int unitCount = LuaToNumber(l, 1);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument");
	}
	for (unsigned int i = 0; i < unitCount; i++) {
		CUnit *unit = new CUnit;
		unitSlots.push_back(unit);
		unit->UnitManagerData.slot = i;
	}
	const unsigned int args = lua_rawlen(l, 2);
	for (unsigned int i = 0; i < args; i++) {
		lua_rawgeti(l, 2, i + 1);
		int unit_index = -1;
		unsigned long cycle = static_cast<unsigned long>(-1);

		for (lua_pushnil(l); lua_next(l, -2); lua_pop(l, 1)) {
			const char *key = LuaToString(l, -2);

			if (!strcmp(key, "Slot")) {
				unit_index = LuaToNumber(l, -1);
			} else if (!strcmp(key, "FreeCycle")) {
				cycle = LuaToNumber(l, -1);
			} else {
				LuaError(l, "Wrong key %s" _C_ key);
			}
		}
		Assert(unit_index != -1 && cycle != static_cast<unsigned long>(-1));
		ReleaseUnit(unitSlots[unit_index]);
		unitSlots[unit_index]->ReleaseCycle = cycle;
		lua_pop(l, 1);
	}
}

}
