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
//      (c) Copyright 1999-2022 by Vladi Belperchinov-Shabanski,
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
#include "unit/unit_stats.h"
#include "unit/unit_type_container.h"

class CPlayer;
class CUnit;
class CUpgrade;
struct lua_State;

extern int CclDefineModifier(lua_State *l);

namespace wyrmgus {

class civilization;
class faction;
class gsml_data;
class gsml_property;
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

	explicit upgrade_modifier(const CUpgrade *upgrade);

	std::unique_ptr<upgrade_modifier> duplicate(const CUpgrade *new_upgrade) const;
	
	void process_gsml_property(const gsml_property &property);
	void process_gsml_scope(const gsml_data &scope);

	const CUpgrade *get_upgrade() const
	{
		return this->upgrade;
	}

	int get_infantry_cost_modifier() const
	{
		return this->infantry_cost_modifier;
	}

	int get_cavalry_cost_modifier() const
	{
		return this->cavalry_cost_modifier;
	}

	const std::vector<const CUpgrade *> &get_removed_upgrades() const
	{
		return this->removed_upgrades;
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
	
	bool affects_variable(const int var_index) const;

	void apply_to_player(CPlayer *player, const int multiplier) const;
	void apply_to_unit(CUnit *unit, const int multiplier) const;

	std::string get_string() const;

private:
	const CUpgrade *upgrade = nullptr; //used to filter required modifier

public:
	unit_stats Modifier;					/// modifier of unit stats.
	std::unique_ptr<int[]> ModifyPercent;	/// use for percent modifiers
	int SpeedResearch = 0;					/// speed factor for researching
private:
	int infantry_cost_modifier = 0;
	int cavalry_cost_modifier = 0;
public:
	int ImproveIncomes[MaxCosts];			/// improve incomes
	// allow/forbid bitmaps -- used as chars for example:
	// `?' -- leave as is, `F' -- forbid, `A' -- allow
	// TODO: see below allow more semantics?
	// TODO: pointers or ids would be faster and less memory use
	int  ChangeUnits[UnitTypeMax];			/// add/remove allowed units

	const unit_type *convert_to = nullptr; //convert to this unit type

	const civilization *change_civilization_to = nullptr; //changes the player's civilization to this one
	const faction *change_faction_to = nullptr; //changes the player's faction to this one

private:
	std::vector<const CUpgrade *> free_upgrades; //upgrades granted for free when this upgrade modifier is applied
	std::vector<const CUpgrade *> removed_upgrades; //upgrades to be removed when this upgrade modifier is applied

	std::vector<unit_type *> unit_types; //which unit types are affected
	std::vector<unit_class *> unit_classes; //which unit classes are affected

	friend int ::CclDefineModifier(lua_State *l);
};

}
