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
//      (c) Copyright 1999-2021 by Vladi Belperchinov-Shabanski,
//		Jimmy Salmon and Andrettin
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

#include "economy/resource.h"
#include "unit/unit_type_container.h"
#include "upgrade/upgrade_structs.h" //for CUnitStats

class CConfigData;
class CUnit;
class CUpgrade;
struct lua_State;

static int CclDefineModifier(lua_State *l);

namespace wyrmgus {

class faction;
class sml_data;
class sml_property;
class unit_class;
class unit_type;

/**
**  This is the modifier of an upgrade.
**  This does the real action of an upgrade, and an upgrade can have multiple
**  modifiers.
*/
class upgrade_modifier final
{
public:
	static std::vector<upgrade_modifier *> UpgradeModifiers;

	upgrade_modifier();

	std::unique_ptr<upgrade_modifier> duplicate() const;
	
	void process_sml_property(const sml_property &property);
	void process_sml_scope(const sml_data &scope);

	int get_infantry_cost_modifier() const
	{
		return this->infantry_cost_modifier;
	}

	int get_cavalry_cost_modifier() const
	{
		return this->cavalry_cost_modifier;
	}

	const std::vector<unit_type *> &get_unit_types() const
	{
		return this->unit_types;
	}
	
	const std::vector<unit_class *> &get_unit_classes() const
	{
		return this->unit_classes;
	}

	bool applies_to(const unit_type *unit_type) const;
	
	int GetUnitStock(unit_type *unit_type) const;
	void SetUnitStock(unit_type *unit_type, int quantity);
	void ChangeUnitStock(unit_type *unit_type, int quantity);

	bool affects_variable(const int var_index) const;
	void apply_to_unit(CUnit *unit, const int multiplier) const;

	int UpgradeId = 0;						/// used to filter required modifier

	CUnitStats Modifier;					/// modifier of unit stats.
	std::unique_ptr<int[]> ModifyPercent;	/// use for percent modifiers
	int SpeedResearch = 0;					/// speed factor for researching
private:
	int infantry_cost_modifier = 0;
	int cavalry_cost_modifier = 0;
public:
	int ImproveIncomes[MaxCosts];			/// improve incomes
	unit_type_map<int> UnitStock;	/// unit stock
	// allow/forbid bitmaps -- used as chars for example:
	// `?' -- leave as is, `F' -- forbid, `A' -- allow
	// TODO: see below allow more semantics?
	// TODO: pointers or ids would be faster and less memory use
	int  ChangeUnits[UnitTypeMax];			/// add/remove allowed units
	char ChangeUpgrades[UpgradeMax];		/// allow/forbid upgrades
private:
	std::vector<unit_type *> unit_types; //which unit types are affected
	std::vector<unit_class *> unit_classes; //which unit classes are affected

public:
	unit_type *ConvertTo = nullptr;			/// convert to this unit-type.

	//Wyrmgus start
	const civilization *change_civilization_to = nullptr;	/// changes the player's civilization to this one
	const faction *change_faction_to = nullptr;	/// changes the player's faction to this one
	
	std::vector<CUpgrade *> RemoveUpgrades;	/// Upgrades to be removed when this upgrade modifier is implented
	//Wyrmgus end

	friend int ::CclDefineModifier(lua_State *l);
};

}
