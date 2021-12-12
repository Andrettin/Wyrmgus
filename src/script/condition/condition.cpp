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
//      (c) Copyright 2000-2021 by Vladi Belperchinov-Shabanski, Lutz Sammer,
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

#include "stratagus.h"

#include "script/condition/condition.h"

#include "config.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "player/faction_type.h"
#include "player/player.h"
#include "religion/deity.h"
#include "script.h"
#include "script/condition/age_condition.h"
#include "script/condition/and_condition.h"
#include "script/condition/any_other_player_condition.h"
#include "script/condition/any_player_condition.h"
#include "script/condition/any_unit_condition.h"
#include "script/condition/any_unit_of_class_condition.h"
#include "script/condition/any_unit_of_type_condition.h"
#include "script/condition/can_accept_quest_condition.h"
#include "script/condition/character_condition.h"
#include "script/condition/character_exists_condition.h"
#include "script/condition/character_unit_condition.h"
#include "script/condition/civilization_condition.h"
#include "script/condition/civilization_group_condition.h"
#include "script/condition/coastal_condition.h"
#include "script/condition/completed_quest_condition.h"
#include "script/condition/dynasty_condition.h"
#include "script/condition/equipment_condition.h"
#include "script/condition/faction_condition.h"
#include "script/condition/location_condition.h"
#include "script/condition/near_site_condition.h"
#include "script/condition/neutral_player_condition.h"
#include "script/condition/not_condition.h"
#include "script/condition/or_condition.h"
#include "script/condition/quest_condition.h"
#include "script/condition/random_condition.h"
#include "script/condition/real_day_condition.h"
#include "script/condition/real_day_of_the_week_condition.h"
#include "script/condition/real_month_condition.h"
#include "script/condition/scripted_condition_condition.h"
#include "script/condition/season_condition.h"
#include "script/condition/settlement_condition.h"
#include "script/condition/site_exists_condition.h"
#include "script/condition/snowy_terrain_condition.h"
#include "script/condition/source_unit_condition.h"
#include "script/condition/terrain_condition.h"
#include "script/condition/time_of_day_condition.h"
#include "script/condition/trigger_condition.h"
#include "script/condition/unique_can_drop_condition.h"
#include "script/condition/unit_class_condition.h"
#include "script/condition/unit_type_condition.h"
#include "script/condition/upgrade_condition.h"
#include "script/condition/upgrade_class_condition.h"
#include "script/condition/war_condition.h"
#include "translate.h"
#include "ui/button.h"
#include "ui/interface.h"
#include "upgrade/upgrade_modifier.h"
#include "util/string_util.h"
#include "util/vector_util.h"

namespace wyrmgus {

std::unique_ptr<const condition> condition::from_sml_property(const sml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "age") {
		return std::make_unique<age_condition>(value);
	} else if (key == "can_accept_quest") {
		return std::make_unique<can_accept_quest_condition>(value);
	} else if (key == "character") {
		return std::make_unique<character_condition>(value);
	} else if (key == "character_exists") {
		return std::make_unique<character_exists_condition>(value);
	} else if (key == "civilization") {
		return std::make_unique<civilization_condition>(value);
	} else if (key == "civilization_group") {
		return std::make_unique<civilization_group_condition>(value);
	} else if (key == "coastal") {
		return std::make_unique<coastal_condition>(value);
	} else if (key == "completed_quest") {
		return std::make_unique<completed_quest_condition>(value);
	} else if (key == "dynasty") {
		return std::make_unique<dynasty_condition>(value);
	} else if (key == "equipment") {
		return std::make_unique<equipment_condition>(value);
	} else if (key == "faction") {
		return std::make_unique<faction_condition>(value);
	} else if (key == "near_site") {
		return std::make_unique<near_site_condition>(value);
	} else if (key == "quest") {
		return std::make_unique<quest_condition>(value);
	} else if (key == "random") {
		return std::make_unique<random_condition>(value);
	} else if (key == "real_day") {
		return std::make_unique<real_day_condition>(value);
	} else if (key == "real_day_of_the_week") {
		return std::make_unique<real_day_of_the_week_condition>(value);
	} else if (key == "real_month") {
		return std::make_unique<real_month_condition>(value);
	} else if (key == "scripted_condition") {
		return std::make_unique<scripted_condition_condition>(value);
	} else if (key == "season") {
		return std::make_unique<season_condition>(value);
	} else if (key == "settlement") {
		return std::make_unique<settlement_condition>(value);
	} else if (key == "site_exists") {
		return std::make_unique<site_exists_condition>(value);
	} else if (key == "snowy_terrain") {
		return std::make_unique<snowy_terrain_condition>(value);
	} else if (key == "terrain") {
		return std::make_unique<terrain_condition>(value);
	} else if (key == "time_of_day") {
		return std::make_unique<time_of_day_condition>(value);
	} else if (key == "trigger") {
		return std::make_unique<trigger_condition>(value);
	} else if (key == "unique_can_drop") {
		return std::make_unique<unique_can_drop_condition>(value);
	} else if (key == "unit_class") {
		return std::make_unique<unit_class_condition>(value);
	} else if (key == "unit_type") {
		return std::make_unique<unit_type_condition>(value);
	} else if (key == "upgrade") {
		return std::make_unique<upgrade_condition>(value);
	} else if (key == "upgrade_class") {
		return std::make_unique<upgrade_class_condition>(value);
	} else if (key == "war") {
		return std::make_unique<war_condition>(value);
	} else {
		throw std::runtime_error("Invalid condition property: \"" + key + "\".");
	}
}

std::unique_ptr<const condition> condition::from_sml_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();
	std::unique_ptr<condition> condition;

	if (tag == "and") {
		condition = std::make_unique<and_condition>();
	} else if (tag == "any_other_player") {
		condition = std::make_unique<any_other_player_condition>();
	} else if (tag == "any_player") {
		condition = std::make_unique<any_player_condition>();
	} else if (tag == "any_unit") {
		condition = std::make_unique<any_unit_condition>();
	} else if (tag == "any_unit_of_class") {
		condition = std::make_unique<any_unit_of_class_condition>();
	} else if (tag == "any_unit_of_type") {
		condition = std::make_unique<any_unit_of_type_condition>();
	} else if (tag == "character_unit") {
		condition = std::make_unique<character_unit_condition>();
	} else if (tag == "location") {
		condition = std::make_unique<location_condition>();
	} else if (tag == "or") {
		condition = std::make_unique<or_condition>();
	} else if (tag == "neutral_player") {
		condition = std::make_unique<neutral_player_condition>();
	} else if (tag == "not") {
		condition = std::make_unique<not_condition>();
	} else if (tag == "source_unit") {
		condition = std::make_unique<source_unit_condition>();
	} else if (tag == "unit_class") {
		condition = std::make_unique<unit_class_condition>();
	} else if (tag == "unit_type") {
		condition = std::make_unique<unit_type_condition>();
	} else if (tag == "settlement") {
		condition = std::make_unique<settlement_condition>();
	} else {
		throw std::runtime_error("Invalid condition scope: \"" + tag + "\".");
	}

	database::process_sml_data(condition, scope);

	return condition;
}

std::string condition::get_object_highlighted_name(const named_data_entry *object, const std::string &name_string)
{
	if (!name_string.empty()) {
		return string::highlight(name_string);
	} else {
		return string::highlight(object->get_name());
	}
}

void condition::ProcessConfigData(const CConfigData *config_data)
{
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		this->ProcessConfigDataProperty(config_data->Properties[i]);
	}
	
	for (const CConfigData *child_config_data : config_data->Children) {
		this->ProcessConfigDataSection(child_config_data);
	}
}

void condition::ProcessConfigDataProperty(const std::pair<std::string, std::string> &property)
{
	fprintf(stderr, "Invalid condition property: \"%s\".\n", property.first.c_str());
}

void condition::ProcessConfigDataSection(const CConfigData *section)
{
	fprintf(stderr, "Invalid condition property: \"%s\".\n", section->Tag.c_str());
}

void condition::process_sml_property(const sml_property &property)
{
	throw std::runtime_error("Invalid condition property: \"" + property.get_key() + "\".");
}

void condition::process_sml_scope(const sml_data &scope)
{
	throw std::runtime_error("Invalid condition scope: \"" + scope.get_tag() + "\".");
}

bool condition::check(const CUnit *unit, const read_only_context &ctx, const bool ignore_units) const
{
	//conditions check the unit's player by default, but can be overriden in the case of e.g. upgrades (where we want to check individual upgrades for the unit)
	return this->check(unit->Player, ctx, ignore_units);
}

void and_condition::ProcessConfigDataSection(const CConfigData *section)
{
	std::unique_ptr<condition> condition;

	if (section->Tag == "and") {
		condition = std::make_unique<and_condition>();
	} else if (section->Tag == "or") {
		condition = std::make_unique<or_condition>();
	} else if (section->Tag == "upgrade") {
		condition = std::make_unique<upgrade_condition>();
	} else {
		throw std::runtime_error("Invalid and condition property: \"" + section->Tag + "\".");
	}

	condition->ProcessConfigData(section);

	this->conditions.push_back(std::move(condition));
}

void and_condition::process_sml_property(const sml_property &property)
{
	this->conditions.push_back(condition::from_sml_property(property));
}

void and_condition::process_sml_scope(const sml_data &scope)
{
	this->conditions.push_back(condition::from_sml_scope(scope));
}

void and_condition::check_validity() const
{
	for (const auto &condition : this->conditions) {
		condition->check_validity();
	}
}

bool and_condition::check(const civilization *civilization) const
{
	for (const auto &condition : this->conditions) {
		if (!condition->check(civilization)) {
			return false;
		}
	}
	
	return true;
}

bool and_condition::check(const CPlayer *player, const read_only_context &ctx, const bool ignore_units) const
{
	for (const auto &condition : this->conditions) {
		if (!condition->check(player, ctx, ignore_units)) {
			return false;
		}
	}
	
	return true;
}

bool and_condition::check(const CUnit *unit, const read_only_context &ctx, const bool ignore_units) const
{
	for (const auto &condition : this->conditions) {
		if (!condition->check(unit, ctx, ignore_units)) {
			return false;
		}
	}
	
	return true;
}

void or_condition::ProcessConfigDataSection(const CConfigData *section)
{
	std::unique_ptr<condition> condition = nullptr;

	if (section->Tag == "and") {
		condition = std::make_unique<and_condition>();
	} else if (section->Tag == "or") {
		condition = std::make_unique<or_condition>();
	} else if (section->Tag == "upgrade") {
		condition = std::make_unique<upgrade_condition>();
	} else {
		fprintf(stderr, "Invalid or condition property: \"%s\".\n", section->Tag.c_str());
		return;
	}
	condition->ProcessConfigData(section);
	this->conditions.push_back(std::move(condition));
}

void upgrade_condition::ProcessConfigDataProperty(const std::pair<std::string, std::string> &property)
{
	const std::string &key = property.first;
	std::string value = property.second;
	if (key == "upgrade") {
		this->upgrade = CUpgrade::get(value);
	} else {
		fprintf(stderr, "Invalid upgrade condition property: \"%s\".\n", key.c_str());
	}
}

template <bool precondition>
bool check_special_conditions(const unit_type *target, const CPlayer *player, const bool ignore_units, const bool is_neutral_use)
{
	if (UnitIdAllowed(*player, target->Slot) == 0) {
		return false;
	}

	if (target->get_unit_class() != nullptr) {
		return check_conditions<precondition>(target->get_unit_class(), player, ignore_units, is_neutral_use);
	}

	return true;
}

template bool check_special_conditions<false>(const unit_type *target, const CPlayer *player, const bool ignore_units, const bool is_neutral_use);
template bool check_special_conditions<true>(const unit_type *target, const CPlayer *player, const bool ignore_units, const bool is_neutral_use);

template <bool precondition>
bool check_special_conditions(const CUpgrade *target, const CPlayer *player, const bool ignore_units, const bool is_neutral_use)
{
	if (UpgradeIdAllowed(*player, target->ID) != 'A' && !((precondition || is_neutral_use) && UpgradeIdAllowed(*player, target->ID) == 'R')) {
		return false;
	}

	const faction *player_faction = player->get_faction();
	if (player_faction != nullptr && player_faction->get_type() == faction_type::holy_order) {
		//if the player is a holy order, and the upgrade is incompatible with its deity, don't allow it
		if (player_faction->get_holy_order_deity() != nullptr) {
			const CUpgrade *deity_upgrade = player_faction->get_holy_order_deity()->get_upgrade();
			if (deity_upgrade != nullptr) {
				for (const auto &modifier : target->get_modifiers()) {
					if (vector::contains(modifier->get_removed_upgrades(), deity_upgrade)) {
						return false;
					}
				}
				for (const auto &modifier : deity_upgrade->get_modifiers()) {
					if (vector::contains(modifier->get_removed_upgrades(), target)) {
						return false;
					}
				}
			}
		}
	}

	if (target->get_upgrade_class() != nullptr) {
		return check_conditions<precondition>(target->get_upgrade_class(), player, ignore_units, is_neutral_use);
	}

	return true;
}

template bool check_special_conditions<false>(const CUpgrade *target, const CPlayer *player, const bool ignore_units, const bool is_neutral_use);
template bool check_special_conditions<true>(const CUpgrade *target, const CPlayer *player, const bool ignore_units, const bool is_neutral_use);

template <bool precondition>
bool check_special_conditions(const unit_type *target, const CUnit *unit, const bool ignore_units)
{
	if (UnitIdAllowed(*unit->Player, target->Slot) == 0) {
		return false;
	}

	if (target->get_unit_class() != nullptr) {
		return check_conditions<precondition>(target->get_unit_class(), unit, ignore_units);
	}

	return true;
}

template bool check_special_conditions<false>(const unit_type *target, const CUnit *unit, const bool ignore_units);
template bool check_special_conditions<true>(const unit_type *target, const CUnit *unit, const bool ignore_units);

template <bool precondition>
bool check_special_conditions(const CUpgrade *target, const CUnit *unit, const bool ignore_units)
{
	if (UpgradeIdAllowed(*unit->Player, target->ID) == 'F') {
		return false;
	}

	if (target->get_upgrade_class() != nullptr) {
		return check_conditions<precondition>(target->get_upgrade_class(), unit, ignore_units);
	}

	return true;
}

template bool check_special_conditions<false>(const CUpgrade *target, const CUnit *unit, const bool ignore_units);
template bool check_special_conditions<true>(const CUpgrade *target, const CUnit *unit, const bool ignore_units);

}

/**
**  Check if this upgrade or unit is available.
**
**  @param player  For this player available.
**  @param target  Unit or Upgrade.
**
**  @return        True if available, false otherwise.
*/
std::string PrintConditions(const wyrmgus::button &button)
{
	std::string rules;

	//
	//  first have to check, if target is allowed itself
	//
	if (!strncmp(button.ValueStr.c_str(), "unit", 4)) {
		// target string refers to unit-XXX
		const wyrmgus::unit_type *unit_type = wyrmgus::unit_type::get(button.ValueStr);
		rules = unit_type->get_conditions()->get_string(0, false);
	} else if (!strncmp(button.ValueStr.c_str(), "upgrade", 7)) {
		// target string refers to upgrade-XXX
		const CUpgrade *upgrade = CUpgrade::get(button.ValueStr);
		if (upgrade->get_conditions()) {
			rules = upgrade->get_conditions()->get_string(0, false);
		}
	} else {
		DebugPrint("target '%s' should be unit-type or upgrade\n" _C_ button.ValueStr.c_str());
		return rules;
	}

	if (rules.empty()) {  //no conditions found
		return rules;
	}

	rules.insert(0, _("Requirements:\n"));
	
	return rules;
}

/*----------------------------------------------------------------------------
--  Ccl part of conditions
----------------------------------------------------------------------------*/

static int CclDefineDependency(lua_State *l)
{
	const int args = lua_gettop(l);
	const char *target = LuaToString(l, 1);

	std::vector<std::unique_ptr<const wyrmgus::condition>> and_conditions;
	
	//  All or rules.
	bool or_flag = false;
	for (int j = 1; j < args; ++j) {
		if (!lua_istable(l, j + 1)) {
			LuaError(l, "incorrect argument");
		}
		const int subargs = lua_rawlen(l, j + 1);

		std::vector<std::unique_ptr<const wyrmgus::condition>> conditions;
	
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
			wyrmgus::condition *condition = nullptr;
			
			if (!strncmp(required, "unit", 4)) {
				const wyrmgus::unit_type *unit_type = wyrmgus::unit_type::get(required);
				condition = new unit_type_condition(unit_type, count > 0 ? count : 1);
			} else if (!strncmp(required, "upgrade", 7)) {
				condition = new upgrade_condition(required);
			} else {
				LuaError(l, "Invalid required type for condition: \"%s\"" _C_ required);
			}
			
			if (count == 0) {
				condition = new not_condition(std::unique_ptr<wyrmgus::condition>(condition));
			}
			
			conditions.push_back(std::unique_ptr<wyrmgus::condition>(condition));
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
		
		if (conditions.size() == 1) {
			and_conditions.push_back(std::move(conditions.front()));
		} else {
			and_conditions.push_back(std::make_unique<wyrmgus::and_condition>(std::move(conditions)));
		}
		conditions.clear();
	}
	
	std::unique_ptr<and_condition> condition;
	if (or_flag) {
		auto or_condition = std::make_unique<wyrmgus::or_condition>(std::move(and_conditions));
		std::vector<std::unique_ptr<const wyrmgus::condition>> or_condition_vector;
		or_condition_vector.push_back(std::move(or_condition));
		condition = std::make_unique<and_condition>(std::move(or_condition_vector));
	} else {
		condition = std::make_unique<and_condition>(std::move(and_conditions));
	}
	
	if (!strncmp(target, "unit", 4)) {
		wyrmgus::unit_type *unit_type = wyrmgus::unit_type::get(target);
		unit_type->conditions = std::move(condition);
	} else if (!strncmp(target, "upgrade", 7)) {
		CUpgrade *upgrade = CUpgrade::get(target);
		upgrade->conditions = std::move(condition);
	} else {
		LuaError(l, "Invalid condition target: \"%s\"" _C_ target);
	}
	
	return 0;
}

static int CclDefinePredependency(lua_State *l)
{
	const int args = lua_gettop(l);
	const char *target = LuaToString(l, 1);

	std::vector<std::unique_ptr<const wyrmgus::condition>> and_conditions;
	
	//  All or rules.
	bool or_flag = false;
	for (int j = 1; j < args; ++j) {
		if (!lua_istable(l, j + 1)) {
			LuaError(l, "incorrect argument");
		}
		const int subargs = lua_rawlen(l, j + 1);

		std::vector<std::unique_ptr<const wyrmgus::condition>> conditions;
	
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
			wyrmgus::condition *condition = nullptr;
			
			if (!strncmp(required, "unit", 4)) {
				const wyrmgus::unit_type *unit_type = wyrmgus::unit_type::get(required);
				condition = new wyrmgus::unit_type_condition(unit_type, count > 0 ? count : 1);
			} else if (!strncmp(required, "upgrade", 7)) {
				condition = new wyrmgus::upgrade_condition(required);
			} else {
				LuaError(l, "Invalid required type for condition: \"%s\"" _C_ required);
			}
			
			if (count == 0) {
				condition = new wyrmgus::not_condition(std::unique_ptr<wyrmgus::condition>(condition));
			}
			
			conditions.push_back(std::unique_ptr<wyrmgus::condition>(condition));
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
		
		if (conditions.size() == 1) {
			and_conditions.push_back(std::move(conditions.front()));
		} else {
			and_conditions.push_back(std::make_unique<wyrmgus::and_condition>(std::move(conditions)));
		}
		conditions.clear();
	}
	
	std::unique_ptr<and_condition> condition;
	if (or_flag) {
		auto or_condition = std::make_unique<wyrmgus::or_condition>(std::move(and_conditions));
		std::vector<std::unique_ptr<const wyrmgus::condition>> or_condition_vector;
		or_condition_vector.push_back(std::move(or_condition));
		condition = std::make_unique<and_condition>(std::move(or_condition_vector));
	} else {
		condition = std::make_unique<and_condition>(std::move(and_conditions));
	}
	
	if (!strncmp(target, "unit", 4)) {
		wyrmgus::unit_type *unit_type = wyrmgus::unit_type::get(target);
		unit_type->preconditions = std::move(condition);
	} else if (!strncmp(target, "upgrade", 7)) {
		CUpgrade *upgrade = CUpgrade::get(target);
		upgrade->preconditions = std::move(condition);
	} else {
		LuaError(l, "Invalid condition target: \"%s\"" _C_ target);
	}
	
	return 0;
}

/**
**  Checks if conditions are met.
**
**  @return true if the conditions are met.
**
**  @param l  Lua state.
**  Argument 1: player
**  Argument 2: object which we want to check the conditions of
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
	const CPlayer *player = CPlayer::Players[plynr].get();
	
	if (!strncmp(object, "unit", 4)) {
		const wyrmgus::unit_type *unit_type = wyrmgus::unit_type::get(object);
		lua_pushboolean(l, check_conditions(unit_type, player));
	} else if (!strncmp(object, "upgrade", 7)) {
		const CUpgrade *upgrade = CUpgrade::get(object);
		lua_pushboolean(l, check_conditions(upgrade, player));
	} else {
		LuaError(l, "Invalid target of condition check: \"%s\"" _C_ object);
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
