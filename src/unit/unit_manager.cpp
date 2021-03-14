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
//      (c) Copyright 2007-2021 by Jimmy Salmon and Andrettin
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

#include "stratagus.h"

//Wyrmgus start
#include "character.h"
//Wyrmgus end
#include "iolib.h"
#include "script.h"
#include "unit/unit_manager.h"
#include "unit/unit.h"
#include "util/exception_util.h"
#include "util/list_util.h"

namespace wyrmgus {

unit_manager::unit_manager()
{
}

unit_manager::~unit_manager()
{
}

/**
**  Initial memory allocation for units.
*/
void unit_manager::init()
{
	this->lastCreated = nullptr;
	this->units.clear();
	this->released_units.clear();
	this->unit_slots.clear();
}

void unit_manager::clean_units()
{
	//copy the vector, as clearing the orders of units may cause one of them to be released, thus changing the vector
	std::vector<CUnit *> units = this->get_units();

	for (CUnit *unit : units) {
		if (unit == nullptr) {
			throw std::runtime_error("Error cleaning unit: unit is null.");
		}

		if (unit->Type == nullptr) {
			throw std::runtime_error("Unit \"" + std::to_string(UnitNumber(*unit)) + "\"'s type is null.");
		}

		if (!unit->Destroyed) {
			if (!unit->Removed) {
				unit->Remove(nullptr);
			}

			//clear orders of all units, removing remaining references to existing units
			unit->clear_all_orders();

			unit->Resource.Workers.clear();
		}
	}

	this->units_seen_under_fog.clear();

	//copy the vector, because releasing units can remove them from the list
	units = this->get_units();

	for (CUnit *unit : units) {
		try {
			unit->Release(true);
		} catch (const std::exception &exception) {
			exception::report(exception);
		}
	}

	this->init();
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
		CUnit *unit = list::take_front(this->released_units);

		if (!unit->Destroyed) {
			throw std::runtime_error("Fetched a non-destroyed unit from the released units list.");
		}

		if (unit->ReleaseCycle == 0) {
			throw std::runtime_error("Fetched a non-released unit from the released units list.");
		}

		const int slot = unit->UnitManagerData.slot;
		unit->Init();
		unit->UnitManagerData.slot = slot;
		unit->UnitManagerData.unitSlot = -1;
		return unit;
	} else {
		auto unit = std::make_unique<CUnit>();
		unit->moveToThread(QApplication::instance()->thread());

		unit->UnitManagerData.slot = this->unit_slots.size();
		CUnit *unit_ptr = unit.get();
		this->unit_slots.push_back(std::move(unit));

		return unit_ptr;
	}
}

/**
**  Release a unit
**
**  @param unit  Unit to release
*/
void unit_manager::ReleaseUnit(CUnit *unit)
{
	if (unit == nullptr) {
		throw std::runtime_error("Tried to call the unit manager's release unit function for a null unit.");
	}

	if (this->lastCreated == unit) {
		this->lastCreated = nullptr;
	}

	if (unit->UnitManagerData.unitSlot != -1) { // == -1 when loading.
		if (this->units[unit->UnitManagerData.unitSlot] != unit) {
			throw std::runtime_error("Unit has index \"" + std::to_string(unit->UnitManagerData.unitSlot) + "\" in the unit manager's units vector, but another unit is present there at that index.");
		}

		CUnit *temp = this->units.back();
		temp->UnitManagerData.unitSlot = unit->UnitManagerData.unitSlot;
		this->units[unit->UnitManagerData.unitSlot] = temp;
		unit->UnitManagerData.unitSlot = -1;
		this->units.pop_back();
	}

	if (!unit->Destroyed) {
		throw std::runtime_error("Adding a non-destroyed unit to the released units list.");
	}

	this->released_units.push_back(unit);
	unit->ReleaseCycle = GameCycle + 500; // can be reused after this time
	//Refs = GameCycle + (NetworkMaxLag << 1); // could be reuse after this time
}

CUnit &unit_manager::GetSlotUnit(const int index) const
{
	return *this->unit_slots[index];
}

unsigned int unit_manager::GetUsedSlotCount() const
{
	return static_cast<unsigned int>(this->unit_slots.size());
}

bool unit_manager::empty() const
{
	return this->units.empty();
}

CUnit *unit_manager::lastCreatedUnit()
{
	return this->lastCreated;
}

void unit_manager::Add(CUnit *unit)
{
	this->lastCreated = unit;
	unit->UnitManagerData.unitSlot = static_cast<int>(this->units.size());
	this->units.push_back(unit);
}

/**
**  Save state of unit manager to file.
**
**  @param file  Output file.
*/
void unit_manager::Save(CFile &file) const
{
	file.printf("SlotUsage(%lu, {", (long unsigned int)this->unit_slots.size());

	for (const CUnit *unit : this->released_units) {
		file.printf("{Slot = %d, FreeCycle = %u}, ", UnitNumber(*unit), unit->ReleaseCycle);
	}
	//Wyrmgus start
	//add items owned by persistent heroes here, as if they were released
	for (std::vector<CUnit *>::const_iterator it = units.begin(); it != units.end(); ++it) {
		const CUnit &unit = **it;
		if (unit.Container && unit.Container->get_character() != nullptr && unit.Container->HasInventory() && unit.Type->BoolFlag[ITEM_INDEX].value) { // don't save items for persistent heroes
			file.printf("{Slot = %d, FreeCycle = %u}, ", UnitNumber(unit), (unsigned int) GameCycle);
		}
	}
	//Wyrmgus end
	file.printf("})\n");

	for (std::vector<CUnit *>::const_iterator it = units.begin(); it != units.end(); ++it) {
		const CUnit &unit = **it;
		//Wyrmgus start
		if (unit.Container && unit.Container->get_character() != nullptr && unit.Container->HasInventory() && unit.Type->BoolFlag[ITEM_INDEX].value) { // don't save items for persistent heroes
			continue;
		}
		//Wyrmgus end
		SaveUnit(unit, file);
	}
}

void unit_manager::Load(lua_State *l)
{
	this->init();
	if (lua_gettop(l) != 2) {
		return;
	}
	unsigned int unitCount = LuaToNumber(l, 1);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument");
	}
	for (unsigned int i = 0; i < unitCount; i++) {
		auto unit = std::make_unique<CUnit>();
		unit->moveToThread(QApplication::instance()->thread());
		unit->UnitManagerData.slot = i;
		this->unit_slots.push_back(std::move(unit));
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
		this->unit_slots[unit_index]->Destroyed = 1;
		this->ReleaseUnit(this->unit_slots[unit_index].get());
		this->unit_slots[unit_index]->ReleaseCycle = cycle;
		lua_pop(l, 1);
	}

	//initialize the base reference for all non-destroyed units
	for (const std::unique_ptr<CUnit> &unit : this->unit_slots) {
		if (unit->Destroyed) {
			continue;
		}

		unit->initialize_base_reference();
	}
}

void unit_manager::add_unit_seen_under_fog(CUnit *unit)
{
	this->units_seen_under_fog[unit] = unit->acquire_ref();
}

void unit_manager::remove_unit_seen_under_fog(CUnit *unit)
{
	this->units_seen_under_fog.erase(unit);
}

}
