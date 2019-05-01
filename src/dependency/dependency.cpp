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
/**@name dependency.cpp - The dependency source file */
//
//      (c) Copyright 2000-2019 by Vladi Belperchinov-Shabanski, Lutz Sammer,
//                                 Jimmy Salmon, Pali Rohár and Andrettin
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

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "dependency/dependency.h"

#include "config.h"
#include "config_operator.h"
#include "dependency/and_dependency.h"
#include "dependency/not_dependency.h"
#include "dependency/or_dependency.h"
#include "dependency/unit_type_dependency.h"
#include "dependency/upgrade_dependency.h"
#include "faction.h"
#include "game/trigger.h"
#include "player.h"
#include "religion/deity.h"
#include "script.h"
#include "translate.h"
#include "ui/button_action.h"
#include "ui/interface.h"
#include "unit/unit.h"
#include "unit/unit_type.h"
#include "upgrade/upgrade.h"
#include "upgrade/upgrade_modifier.h"
#include "upgrade/upgrade_structs.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Process data provided by a configuration file
**
**	@param	config_data	The configuration data
*/
void CDependency::ProcessConfigData(const CConfigData *config_data)
{
	for (const CConfigProperty &property : config_data->Properties) {
		if (property.Operator != CConfigOperator::Assignment) {
			fprintf(stderr, "Wrong operator enumeration index for property \"%s\": %i.\n", property.Key.c_str(), property.Operator);
			continue;
		}
		
		this->ProcessConfigDataProperty(std::pair<std::string, std::string>(property.Key, property.Value));
	}
	
	for (const CConfigData *section : config_data->Sections) {
		this->ProcessConfigDataSection(section);
	}
}

void CDependency::ProcessConfigDataProperty(const std::pair<std::string, std::string> &property)
{
	fprintf(stderr, "Invalid dependency property: \"%s\".\n", property.first.c_str());
}

void CDependency::ProcessConfigDataSection(const CConfigData *section)
{
	fprintf(stderr, "Invalid dependency property: \"%s\".\n", section->Tag.c_str());
}

bool CDependency::Check(const CUnit *unit, bool ignore_units) const
{
	//dependencies check the unit's player by default, but can be overriden in the case of e.g. upgrades (where we want to check individual upgrades for the unit)
	return this->Check(unit->Player, ignore_units);
}

/**
**  Check if this upgrade or unit is available.
**
**  @param player  For this player available.
**  @param target  Unit or Upgrade.
**
**  @return        True if available, false otherwise.
*/
std::string PrintDependencies(const CPlayer &player, const ButtonAction &button)
{
	std::string rules;

	//
	//  first have to check, if target is allowed itself
	//
	if (!strncmp(button.ValueStr.c_str(), "unit-", 5)) {
		// target string refers to unit-XXX
		const CUnitType *unit_type = CUnitType::Get(button.ValueStr);
		if (unit_type->Dependency) {
			rules = unit_type->Dependency->GetString();
		}
	} else if (!strncmp(button.ValueStr.c_str(), "upgrade-", 8)) {
		// target string refers to upgrade-XXX
		const CUpgrade *upgrade = CUpgrade::Get(button.ValueStr);
		if (upgrade->Dependency) {
			rules = upgrade->Dependency->GetString();
		}
	} else {
		DebugPrint("target '%s' should be unit-type or upgrade\n" _C_ button.ValueStr.c_str());
		return rules;
	}

	if (rules.empty()) {  //no dependencies found
		return rules;
	}

	rules.insert(0, _("Requirements:\n"));
	
	return rules;
}

bool CheckDependencies(const CUnitType *target, const CPlayer *player, const bool ignore_units, const bool is_predependency, const bool is_neutral_use)
{
	if (!is_predependency && !CheckDependencies(target, player, ignore_units, true, is_neutral_use)) {
		return false;
	}
	
	if (UnitIdAllowed(*player, target->GetIndex()) == 0) {
		return false;
	}
	
	if (is_predependency) {
		return !target->Predependency || target->Predependency->Check(player, ignore_units);
	} else {
		return !target->Dependency || target->Dependency->Check(player, ignore_units);
	}
}

bool CheckDependencies(const CUpgrade *target, const CPlayer *player, const bool ignore_units, const bool is_predependency, const bool is_neutral_use)
{
	if (!is_predependency && !CheckDependencies(target, player, ignore_units, true, is_neutral_use)) {
		return false;
	}
	
	if (UpgradeIdAllowed(*player, target->ID) != 'A' && !((is_predependency || is_neutral_use) && UpgradeIdAllowed(*player, target->ID) == 'R')) {
		return false;
	}

	if (player->GetFaction() != nullptr && player->GetFaction()->Type == FactionTypeHolyOrder) { // if the player is a holy order, and the upgrade is incompatible with its deity, don't allow it
		if (player->GetFaction()->HolyOrderDeity) {
			CUpgrade *deity_upgrade = player->GetFaction()->HolyOrderDeity->DeityUpgrade;
			if (deity_upgrade) {
				for (CUpgradeModifier *upgrade_modifier : target->UpgradeModifiers) {
					if (std::find(upgrade_modifier->RemoveUpgrades.begin(), upgrade_modifier->RemoveUpgrades.end(), deity_upgrade) != upgrade_modifier->RemoveUpgrades.end()) {
						return false;
					}
				}
				for (CUpgradeModifier *upgrade_modifier : deity_upgrade->UpgradeModifiers) {
					if (std::find(upgrade_modifier->RemoveUpgrades.begin(), upgrade_modifier->RemoveUpgrades.end(), target) != upgrade_modifier->RemoveUpgrades.end()) {
						return false;
					}
				}
			}
		}
	}
	
	if (is_predependency) {
		return !target->Predependency || target->Predependency->Check(player, ignore_units);
	} else {
		return !target->Dependency || target->Dependency->Check(player, ignore_units);
	}
}

bool CheckDependencies(const CUnitType *target, const CUnit *unit, const bool ignore_units, const bool is_predependency)
{
	if (!is_predependency && !CheckDependencies(target, unit, ignore_units, true)) {
		return false;
	}
	
	if (UnitIdAllowed(*unit->Player, target->GetIndex()) == 0) {
		return false;
	}
	
	if (is_predependency) {
		return !target->Predependency || target->Predependency->Check(unit, ignore_units);
	} else {
		return !target->Dependency || target->Dependency->Check(unit, ignore_units);
	}
}

bool CheckDependencies(const CUpgrade *target, const CUnit *unit, const bool ignore_units, const bool is_predependency)
{
	if (!is_predependency && !CheckDependencies(target, unit, ignore_units, true)) {
		return false;
	}
	
	if (UpgradeIdAllowed(*unit->Player, target->ID) == 'F') {
		return false;
	}

	if (is_predependency) {
		return !target->Predependency || target->Predependency->Check(unit, ignore_units);
	} else {
		return !target->Dependency || target->Dependency->Check(unit, ignore_units);
	}
}

/*----------------------------------------------------------------------------
--  Ccl part of dependencies
----------------------------------------------------------------------------*/

/**
**  Define a new dependency.
**
**  @param l  Lua state.
*/
static int CclDefineDependency(lua_State *l)
{
	const int args = lua_gettop(l);
	const char *target = LuaToString(l, 1);

	std::vector<CDependency *> and_dependencies;
	
	//  All or rules.
	bool or_flag = false;
	for (int j = 1; j < args; ++j) {
		if (!lua_istable(l, j + 1)) {
			LuaError(l, "incorrect argument");
		}
		const int subargs = lua_rawlen(l, j + 1);

		std::vector<CDependency *> dependencies;
	
		for (int k = 0; k < subargs; ++k) {
			const char *required = LuaToString(l, j + 1, k + 1);
			int count = 1;
			if (k + 1 < subargs) {
				lua_rawgeti(l, j + 1, k + 2);
				if (lua_isnumber(l, -1)) {
					count = LuaToNumber(l, -1);
					++k;
				}
				lua_pop(l, 1);
			}
			CDependency *dependency = nullptr;
			
			if (!strncmp(required, "unit-", 5)) {
				const CUnitType *unit_type = CUnitType::Get(required);
				if (unit_type != nullptr) {
					dependency = new CUnitTypeDependency(unit_type, count > 0 ? count : 1);
				}
			} else if (!strncmp(required, "upgrade-", 8)) {
				const CUpgrade *upgrade = CUpgrade::Get(required);
				if (upgrade != nullptr) {
					dependency = new CUpgradeDependency(upgrade);
				} else {
					fprintf(stderr, "Invalid upgrade: \"%s\".\n", required);
				}
			} else {
				fprintf(stderr, "Invalid required type for dependency: \"%s\".\n", required);
			}
			
			if (dependency == nullptr) {
				continue;
			}
			
			if (count == 0) {
				dependency = new CNotDependency(dependency);
			}
			
			dependencies.push_back(dependency);
		}
		if (j + 1 < args) {
			++j;
			const char *value = LuaToString(l, j + 1);
			if (strcmp(value, "or")) {
				LuaError(l, "not or symbol: %s" _C_ value);
				return 0;
			}
			or_flag = true;
		}
		
		and_dependencies.push_back(new CAndDependency(dependencies));
		dependencies.clear();
	}
	
	CDependency *dependency = nullptr;
	if (or_flag) {
		dependency = new COrDependency(and_dependencies);
	} else {
		dependency = new CAndDependency(and_dependencies);
	}
	
	if (!strncmp(target, "unit-", 5)) {
		CUnitType *unit_type = CUnitType::Get(target);
		if (unit_type != nullptr) {
			unit_type->Dependency = dependency;
		}
	} else if (!strncmp(target, "upgrade-", 8)) {
		CUpgrade *upgrade = CUpgrade::Get(target);
		if (upgrade != nullptr) {
			upgrade->Dependency = dependency;
		} else {
			fprintf(stderr, "Invalid upgrade: \"%s\".\n", target);
		}
	} else {
		fprintf(stderr, "Invalid dependency target: \"%s\".\n", target);
	}
	
	return 0;
}

static int CclDefinePredependency(lua_State *l)
{
	const int args = lua_gettop(l);
	const char *target = LuaToString(l, 1);

	std::vector<CDependency *> and_dependencies;
	
	//  All or rules.
	bool or_flag = false;
	for (int j = 1; j < args; ++j) {
		if (!lua_istable(l, j + 1)) {
			LuaError(l, "incorrect argument");
		}
		const int subargs = lua_rawlen(l, j + 1);

		std::vector<CDependency *> dependencies;
	
		for (int k = 0; k < subargs; ++k) {
			const char *required = LuaToString(l, j + 1, k + 1);
			int count = 1;
			if (k + 1 < subargs) {
				lua_rawgeti(l, j + 1, k + 2);
				if (lua_isnumber(l, -1)) {
					count = LuaToNumber(l, -1);
					++k;
				}
				lua_pop(l, 1);
			}
			CDependency *dependency = nullptr;
			
			if (!strncmp(required, "unit-", 5)) {
				const CUnitType *unit_type = CUnitType::Get(required);
				if (!unit_type) {
					LuaError(l, "Invalid unit type: \"%s\"" _C_ required);
				}
				dependency = new CUnitTypeDependency(unit_type, count > 0 ? count : 1);
			} else if (!strncmp(required, "upgrade-", 8)) {
				const CUpgrade *upgrade = CUpgrade::Get(required);
				if (!upgrade) {
					LuaError(l, "Invalid upgrade: \"%s\"" _C_ required);
				}
				dependency = new CUpgradeDependency(upgrade);
			} else {
				LuaError(l, "Invalid required type for dependency: \"%s\"" _C_ required);
			}
			
			if (count == 0) {
				dependency = new CNotDependency(dependency);
			}
			
			dependencies.push_back(dependency);
		}
		if (j + 1 < args) {
			++j;
			const char *value = LuaToString(l, j + 1);
			if (strcmp(value, "or")) {
				LuaError(l, "not or symbol: %s" _C_ value);
				return 0;
			}
			or_flag = true;
		}
		
		and_dependencies.push_back(new CAndDependency(dependencies));
		dependencies.clear();
	}
	
	CDependency *dependency = nullptr;
	if (or_flag) {
		dependency = new COrDependency(and_dependencies);
	} else {
		dependency = new CAndDependency(and_dependencies);
	}
	
	if (!strncmp(target, "unit-", 5)) {
		CUnitType *unit_type = CUnitType::Get(target);
		if (!unit_type) {
			LuaError(l, "Invalid unit type: \"%s\"" _C_ target);
		}
		unit_type->Predependency = dependency;
	} else if (!strncmp(target, "upgrade-", 8)) {
		CUpgrade *upgrade = CUpgrade::Get(target);
		if (!upgrade) {
			LuaError(l, "Invalid upgrade: \"%s\"" _C_ target);
		}
		upgrade->Predependency = dependency;
	} else {
		LuaError(l, "Invalid dependency target: \"%s\"" _C_ target);
	}
	
	return 0;
}

/**
**  Checks if dependencies are met.
**
**  @return true if the dependencies are met.
**
**  @param l  Lua state.
**  Argument 1: player
**  Argument 2: object which we want to check the dependencies of
*/
static int CclCheckDependency(lua_State *l)
{
	LuaCheckArgs(l, 2);
	const char *object = LuaToString(l, 2);
	lua_pop(l, 1);
	const int plynr = TriggerGetPlayer(l);
	if (plynr == -1) {
		LuaError(l, "bad player: %i" _C_ plynr);
	}
	const CPlayer *player = CPlayer::Players[plynr];
	
	if (!strncmp(object, "unit-", 5)) {
		const CUnitType *unit_type = CUnitType::Get(object);
		if (!unit_type) {
			LuaError(l, "Invalid unit type: \"%s\"" _C_ object);
		}
		lua_pushboolean(l, CheckDependencies(unit_type, player));
	} else if (!strncmp(object, "upgrade-", 8)) {
		const CUpgrade *upgrade = CUpgrade::Get(object);
		if (!upgrade) {
			LuaError(l, "Invalid upgrade: \"%s\"" _C_ object);
		}
		lua_pushboolean(l, CheckDependencies(upgrade, player));
	} else {
		LuaError(l, "Invalid target of dependency check: \"%s\"" _C_ object);
	}

	return 1;
}

/**
**  Register CCL features for dependencies.
*/
void DependenciesCclRegister()
{
	lua_register(Lua, "DefinePredependency", CclDefinePredependency);
	lua_register(Lua, "DefineDependency", CclDefineDependency);
	lua_register(Lua, "CheckDependency", CclCheckDependency);
}
