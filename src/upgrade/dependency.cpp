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
/**@name depend.cpp - The dependencies source file */
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

#include "upgrade/dependency.h"

#include "age.h"
#include "character.h"
#include "config.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "player.h"
#include "religion/deity.h"
#include "script.h"
#include "time/season.h"
#include "translate.h"
#include "trigger.h"
#include "ui/button_action.h"
#include "ui/interface.h"
#include "unit/unit.h"
#include "unit/unittype.h"
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
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		this->ProcessConfigDataProperty(config_data->Properties[i]);
	}
	
	for (const CConfigData *child_config_data : config_data->Children) {
		this->ProcessConfigDataSection(child_config_data);
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

void CAndDependency::ProcessConfigDataSection(const CConfigData *section)
{
	CDependency *dependency = nullptr;
	if (section->Tag == "and") {
		dependency = new CAndDependency;
	} else if (section->Tag == "or") {
		dependency = new COrDependency;
	} else if (section->Tag == "not") {
		dependency = new CNotDependency;
	} else if (section->Tag == "unit_type") {
		dependency = new CUnitTypeDependency;
	} else if (section->Tag == "upgrade") {
		dependency = new CUpgradeDependency;
	} else if (section->Tag == "age") {
		dependency = new CAgeDependency;
	} else if (section->Tag == "character") {
		dependency = new CCharacterDependency;
	} else if (section->Tag == "season") {
		dependency = new CSeasonDependency;
	} else if (section->Tag == "trigger") {
		dependency = new CTriggerDependency;
	} else {
		fprintf(stderr, "Invalid and dependency property: \"%s\".\n", section->Tag.c_str());
		return;
	}
	dependency->ProcessConfigData(section);
	this->Dependencies.push_back(dependency);
}

bool CAndDependency::Check(const CPlayer *player, bool ignore_units) const
{
	for (const CDependency *dependency : this->Dependencies) {
		if (!dependency->Check(player, ignore_units)) {
			return false;
		}
	}
	
	return true;
}

bool CAndDependency::Check(const CUnit *unit, bool ignore_units) const
{
	for (const CDependency *dependency : this->Dependencies) {
		if (!dependency->Check(unit, ignore_units)) {
			return false;
		}
	}
	
	return true;
}

void COrDependency::ProcessConfigDataSection(const CConfigData *section)
{
	CDependency *dependency = nullptr;
	if (section->Tag == "and") {
		dependency = new CAndDependency;
	} else if (section->Tag == "or") {
		dependency = new COrDependency;
	} else if (section->Tag == "not") {
		dependency = new CNotDependency;
	} else if (section->Tag == "unit_type") {
		dependency = new CUnitTypeDependency;
	} else if (section->Tag == "upgrade") {
		dependency = new CUpgradeDependency;
	} else if (section->Tag == "age") {
		dependency = new CAgeDependency;
	} else if (section->Tag == "character") {
		dependency = new CCharacterDependency;
	} else if (section->Tag == "season") {
		dependency = new CSeasonDependency;
	} else if (section->Tag == "trigger") {
		dependency = new CTriggerDependency;
	} else {
		fprintf(stderr, "Invalid or dependency property: \"%s\".\n", section->Tag.c_str());
		return;
	}
	dependency->ProcessConfigData(section);
	this->Dependencies.push_back(dependency);
}

bool COrDependency::Check(const CPlayer *player, bool ignore_units) const
{
	for (const CDependency *dependency : this->Dependencies) {
		if (dependency->Check(player, ignore_units)) {
			return true;
		}
	}
	
	return false;
}

bool COrDependency::Check(const CUnit *unit, bool ignore_units) const
{
	for (const CDependency *dependency : this->Dependencies) {
		if (dependency->Check(unit, ignore_units)) {
			return true;
		}
	}
	
	return false;
}

void CNotDependency::ProcessConfigDataSection(const CConfigData *section)
{
	CDependency *dependency = nullptr;
	if (section->Tag == "and") {
		dependency = new CAndDependency;
	} else if (section->Tag == "or") {
		dependency = new COrDependency;
	} else if (section->Tag == "not") {
		dependency = new CNotDependency;
	} else if (section->Tag == "unit_type") {
		dependency = new CUnitTypeDependency;
	} else if (section->Tag == "upgrade") {
		dependency = new CUpgradeDependency;
	} else if (section->Tag == "age") {
		dependency = new CAgeDependency;
	} else if (section->Tag == "character") {
		dependency = new CCharacterDependency;
	} else if (section->Tag == "season") {
		dependency = new CSeasonDependency;
	} else if (section->Tag == "trigger") {
		dependency = new CTriggerDependency;
	} else {
		fprintf(stderr, "Invalid not dependency property: \"%s\".\n", section->Tag.c_str());
		return;
	}
	dependency->ProcessConfigData(section);
	this->Dependencies.push_back(dependency);
}

bool CNotDependency::Check(const CPlayer *player, bool ignore_units) const
{
	for (const CDependency *dependency : this->Dependencies) {
		if (dependency->Check(player, ignore_units)) {
			return false;
		}
	}
	
	return true;
}

bool CNotDependency::Check(const CUnit *unit, bool ignore_units) const
{
	for (const CDependency *dependency : this->Dependencies) {
		if (dependency->Check(unit, ignore_units)) {
			return false;
		}
	}
	
	return true;
}

void CUnitTypeDependency::ProcessConfigDataProperty(const std::pair<std::string, std::string> &property)
{
	const std::string &key = property.first;
	std::string value = property.second;
	if (key == "unit_type") {
		value = FindAndReplaceString(value, "_", "-");
		this->UnitType = UnitTypeByIdent(value);
		if (!this->UnitType) {
			fprintf(stderr, "Invalid unit type: \"%s\".\n", value.c_str());
		}
	} else if (key == "count") {
		this->Count = std::stoi(value);
	} else {
		fprintf(stderr, "Invalid unit type dependency property: \"%s\".\n", key.c_str());
	}
}

bool CUnitTypeDependency::Check(const CPlayer *player, bool ignore_units) const
{
	if (ignore_units) {
		return true;
	}
	
	return player->GetUnitTypeCount(this->UnitType) >= this->Count;
}

std::string CUnitTypeDependency::GetString(const std::string &prefix) const
{
	std::string str = prefix + this->UnitType->Name;
	
	if (this->Count > 1) {
		str += '(' + std::to_string(this->Count) + ')';
	}
	
	str += '\n';
	
	return str;
}

void CUpgradeDependency::ProcessConfigDataProperty(const std::pair<std::string, std::string> &property)
{
	const std::string &key = property.first;
	std::string value = property.second;
	if (key == "upgrade") {
		value = FindAndReplaceString(value, "_", "-");
		this->Upgrade = CUpgrade::Get(value);
		if (!this->Upgrade) {
			fprintf(stderr, "Invalid upgrade: \"%s\".\n", value.c_str());
		}
	} else {
		fprintf(stderr, "Invalid upgrade dependency property: \"%s\".\n", key.c_str());
	}
}

bool CUpgradeDependency::Check(const CPlayer *player, bool ignore_units) const
{
	return UpgradeIdAllowed(*player, this->Upgrade->ID) == 'R';
}

bool CUpgradeDependency::Check(const CUnit *unit, bool ignore_units) const
{
	return this->Check(unit->Player, ignore_units) || unit->GetIndividualUpgrade(this->Upgrade);
}

std::string CUpgradeDependency::GetString(const std::string &prefix) const
{
	std::string str = prefix + this->Upgrade->Name + '\n';
	return str;
}

void CAgeDependency::ProcessConfigDataProperty(const std::pair<std::string, std::string> &property)
{
	const std::string &key = property.first;
	std::string value = property.second;
	if (key == "age") {
		value = FindAndReplaceString(value, "_", "-");
		this->Age = CAge::GetAge(value);
	} else {
		fprintf(stderr, "Invalid age dependency property: \"%s\".\n", key.c_str());
	}
}

bool CAgeDependency::Check(const CPlayer *player, bool ignore_units) const
{
	return player->Age == this->Age;
}

std::string CAgeDependency::GetString(const std::string &prefix) const
{
	std::string str = prefix + this->Age->Name + '\n';
	return str;
}

void CCharacterDependency::ProcessConfigDataProperty(const std::pair<std::string, std::string> &property)
{
	const std::string &key = property.first;
	std::string value = property.second;
	if (key == "character") {
		value = FindAndReplaceString(value, "_", "-");
		this->Character = CCharacter::GetCharacter(value);
	} else {
		fprintf(stderr, "Invalid character dependency property: \"%s\".\n", key.c_str());
	}
}

bool CCharacterDependency::Check(const CPlayer *player, bool ignore_units) const
{
	return player->HasHero(this->Character);
}

bool CCharacterDependency::Check(const CUnit *unit, bool ignore_units) const
{
	return unit->Character == this->Character;
}

std::string CCharacterDependency::GetString(const std::string &prefix) const
{
	std::string str = prefix + this->Character->GetFullName() + '\n';
	return str;
}

void CSeasonDependency::ProcessConfigDataProperty(const std::pair<std::string, std::string> &property)
{
	const std::string &key = property.first;
	std::string value = property.second;
	if (key == "season") {
		value = FindAndReplaceString(value, "_", "-");
		this->Season = CSeason::GetSeason(value);
	} else {
		fprintf(stderr, "Invalid season dependency property: \"%s\".\n", key.c_str());
	}
}

bool CSeasonDependency::Check(const CPlayer *player, bool ignore_units) const
{
	return Map.MapLayers[player->StartMapLayer]->GetSeason() == this->Season;
}

bool CSeasonDependency::Check(const CUnit *unit, bool ignore_units) const
{
	return unit->MapLayer->GetSeason() == this->Season;
}

std::string CSeasonDependency::GetString(const std::string &prefix) const
{
	std::string str = prefix + this->Season->Name + '\n';
	return str;
}

void CTriggerDependency::ProcessConfigDataProperty(const std::pair<std::string, std::string> &property)
{
	const std::string &key = property.first;
	std::string value = property.second;
	if (key == "trigger") {
		value = FindAndReplaceString(value, "_", "-");
		this->Trigger = CTrigger::GetTrigger(value);
	} else {
		fprintf(stderr, "Invalid trigger dependency property: \"%s\".\n", key.c_str());
	}
}

bool CTriggerDependency::Check(const CPlayer *player, bool ignore_units) const
{
	//checks whether a trigger has already fired
	
	return std::find(CTrigger::DeactivatedTriggers.begin(), CTrigger::DeactivatedTriggers.end(), this->Trigger->Ident) != CTrigger::DeactivatedTriggers.end(); //this works fine for global triggers, but for player triggers perhaps it should check only the player?
}

std::string CTriggerDependency::GetString(const std::string &prefix) const
{
	return std::string();
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
		const CUnitType *unit_type = UnitTypeByIdent(button.ValueStr);
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

bool CheckDependencies(const CUnitType *target, const CPlayer *player, bool ignore_units, bool is_predependency, bool is_neutral_use)
{
	if (!is_predependency && !CheckDependencies(target, player, ignore_units, true, is_neutral_use)) {
		return false;
	}
	
	if (UnitIdAllowed(*player, target->Slot) == 0) {
		return false;
	}
	
	if (is_predependency) {
		return !target->Predependency || target->Predependency->Check(player, ignore_units);
	} else {
		return !target->Dependency || target->Dependency->Check(player, ignore_units);
	}
}

bool CheckDependencies(const CUpgrade *target, const CPlayer *player, bool ignore_units, bool is_predependency, bool is_neutral_use)
{
	if (!is_predependency && !CheckDependencies(target, player, ignore_units, true, is_neutral_use)) {
		return false;
	}
	
	if (UpgradeIdAllowed(*player, target->ID) != 'A' && !((is_predependency || is_neutral_use) && UpgradeIdAllowed(*player, target->ID) == 'R')) {
		return false;
	}

	if (player->Faction != -1 && PlayerRaces.Factions[player->Faction]->Type == FactionTypeHolyOrder) { // if the player is a holy order, and the upgrade is incompatible with its deity, don't allow it
		if (PlayerRaces.Factions[player->Faction]->HolyOrderDeity) {
			CUpgrade *deity_upgrade = PlayerRaces.Factions[player->Faction]->HolyOrderDeity->DeityUpgrade;
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

bool CheckDependencies(const CUnitType *target, const CUnit *unit, bool ignore_units, bool is_predependency)
{
	if (!is_predependency && !CheckDependencies(target, unit, ignore_units, true)) {
		return false;
	}
	
	if (UnitIdAllowed(*unit->Player, target->Slot) == 0) {
		return false;
	}
	
	if (is_predependency) {
		return !target->Predependency || target->Predependency->Check(unit, ignore_units);
	} else {
		return !target->Dependency || target->Dependency->Check(unit, ignore_units);
	}
}

bool CheckDependencies(const CUpgrade *target, const CUnit *unit, bool ignore_units, bool is_predependency)
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

	std::vector<const CDependency *> and_dependencies;
	
	//  All or rules.
	bool or_flag = false;
	for (int j = 1; j < args; ++j) {
		if (!lua_istable(l, j + 1)) {
			LuaError(l, "incorrect argument");
		}
		const int subargs = lua_rawlen(l, j + 1);

		std::vector<const CDependency *> dependencies;
	
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
				const CUnitType *unit_type = UnitTypeByIdent(required);
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
		CUnitType *unit_type = UnitTypeByIdent(target);
		if (!unit_type) {
			LuaError(l, "Invalid unit type: \"%s\"" _C_ target);
		}
		unit_type->Dependency = dependency;
	} else if (!strncmp(target, "upgrade-", 8)) {
		CUpgrade *upgrade = CUpgrade::Get(target);
		if (!upgrade) {
			LuaError(l, "Invalid upgrade: \"%s\"" _C_ target);
		}
		upgrade->Dependency = dependency;
	} else {
		LuaError(l, "Invalid dependency target: \"%s\"" _C_ target);
	}
	
	return 0;
}

static int CclDefinePredependency(lua_State *l)
{
	const int args = lua_gettop(l);
	const char *target = LuaToString(l, 1);

	std::vector<const CDependency *> and_dependencies;
	
	//  All or rules.
	bool or_flag = false;
	for (int j = 1; j < args; ++j) {
		if (!lua_istable(l, j + 1)) {
			LuaError(l, "incorrect argument");
		}
		const int subargs = lua_rawlen(l, j + 1);

		std::vector<const CDependency *> dependencies;
	
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
				const CUnitType *unit_type = UnitTypeByIdent(required);
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
		CUnitType *unit_type = UnitTypeByIdent(target);
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
	const CPlayer *player = &Players[plynr];
	
	if (!strncmp(object, "unit-", 5)) {
		const CUnitType *unit_type = UnitTypeByIdent(object);
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
