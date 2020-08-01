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
//      (c) Copyright 2000-2020 by Vladi Belperchinov-Shabanski, Lutz Sammer,
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

#include "stratagus.h"

#include "script/condition/condition.h"

#include "age.h"
#include "character.h"
#include "config.h"
#include "faction.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "map/site.h"
#include "player.h"
#include "religion/deity.h"
#include "script.h"
#include "script/condition/age_condition.h"
#include "script/condition/and_condition.h"
#include "script/condition/character_condition.h"
#include "script/condition/not_condition.h"
#include "script/condition/or_condition.h"
#include "script/condition/season_condition.h"
#include "script/condition/settlement_condition.h"
#include "script/condition/trigger_condition.h"
#include "script/condition/unit_type_condition.h"
#include "script/condition/upgrade_condition.h"
#include "script/trigger.h"
#include "time/season.h"
#include "translate.h"
#include "ui/button.h"
#include "ui/interface.h"
#include "unit/unit.h"
#include "unit/unit_find.h"
#include "unit/unit_type.h"
#include "upgrade/upgrade.h"
#include "upgrade/upgrade_modifier.h"
#include "upgrade/upgrade_structs.h"
#include "util/string_util.h"
#include "util/vector_util.h"

namespace stratagus {

std::unique_ptr<const condition> condition::from_sml_property(const sml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "upgrade") {
		return std::make_unique<upgrade_condition>(CUpgrade::get(value));
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
	} else if (tag == "or") {
		condition = std::make_unique<or_condition>();
	} else if (tag == "not") {
		condition = std::make_unique<not_condition>();
	} else if (tag == "unit_type") {
		condition = std::make_unique<unit_type_condition>();
	} else if (tag == "upgrade") {
		condition = std::make_unique<upgrade_condition>();
	} else if (tag == "age") {
		condition = std::make_unique<age_condition>();
	} else if (tag == "character") {
		condition = std::make_unique<character_condition>();
	} else if (tag == "season") {
		condition = std::make_unique<season_condition>();
	} else if (tag == "settlement") {
		condition = std::make_unique<settlement_condition>();
	} else if (tag == "trigger") {
		condition = std::make_unique<trigger_condition>();
	} else {
		throw std::runtime_error("Invalid condition scope: \"" + tag + "\".");
	}

	database::process_sml_data(condition, scope);

	return condition;
}

/**
**	@brief	Process data provided by a configuration file
**
**	@param	config_data	The configuration data
*/
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

bool condition::check(const CUnit *unit, bool ignore_units) const
{
	//conditions check the unit's player by default, but can be overriden in the case of e.g. upgrades (where we want to check individual upgrades for the unit)
	return this->check(unit->Player, ignore_units);
}

void and_condition::ProcessConfigDataSection(const CConfigData *section)
{
	std::unique_ptr<condition> condition;

	if (section->Tag == "and") {
		condition = std::make_unique<and_condition>();
	} else if (section->Tag == "or") {
		condition = std::make_unique<or_condition>();
	} else if (section->Tag == "not") {
		condition = std::make_unique<not_condition>();
	} else if (section->Tag == "unit_type") {
		condition = std::make_unique<unit_type_condition>();
	} else if (section->Tag == "upgrade") {
		condition = std::make_unique<upgrade_condition>();
	} else if (section->Tag == "age") {
		condition = std::make_unique<age_condition>();
	} else if (section->Tag == "character") {
		condition = std::make_unique<character_condition>();
	} else if (section->Tag == "season") {
		condition = std::make_unique<season_condition>();
	} else if (section->Tag == "trigger") {
		condition = std::make_unique<trigger_condition>();
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

bool and_condition::check(const CPlayer *player, bool ignore_units) const
{
	for (const auto &condition : this->conditions) {
		if (!condition->check(player, ignore_units)) {
			return false;
		}
	}
	
	return true;
}

bool and_condition::check(const CUnit *unit, bool ignore_units) const
{
	for (const auto &condition : this->conditions) {
		if (!condition->check(unit, ignore_units)) {
			return false;
		}
	}
	
	return true;
}

void or_condition::ProcessConfigDataSection(const CConfigData *section)
{
	condition *condition = nullptr;
	if (section->Tag == "and") {
		condition = new and_condition;
	} else if (section->Tag == "or") {
		condition = new or_condition;
	} else if (section->Tag == "not") {
		condition = new not_condition;
	} else if (section->Tag == "unit_type") {
		condition = new unit_type_condition;
	} else if (section->Tag == "upgrade") {
		condition = new upgrade_condition;
	} else if (section->Tag == "age") {
		condition = new age_condition;
	} else if (section->Tag == "character") {
		condition = new character_condition;
	} else if (section->Tag == "season") {
		condition = new season_condition;
	} else if (section->Tag == "trigger") {
		condition = new trigger_condition;
	} else {
		fprintf(stderr, "Invalid or condition property: \"%s\".\n", section->Tag.c_str());
		return;
	}
	condition->ProcessConfigData(section);
	this->conditions.push_back(std::unique_ptr<stratagus::condition>(condition));
}

void or_condition::process_sml_property(const sml_property &property)
{
	this->conditions.push_back(condition::from_sml_property(property));
}

void or_condition::process_sml_scope(const sml_data &scope)
{
	this->conditions.push_back(condition::from_sml_scope(scope));
}

bool or_condition::check(const CPlayer *player, bool ignore_units) const
{
	for (const auto &condition : this->conditions) {
		if (condition->check(player, ignore_units)) {
			return true;
		}
	}
	
	return false;
}

bool or_condition::check(const CUnit *unit, bool ignore_units) const
{
	for (const auto &condition : this->conditions) {
		if (condition->check(unit, ignore_units)) {
			return true;
		}
	}
	
	return false;
}

void not_condition::process_sml_property(const sml_property &property)
{
	this->conditions.push_back(condition::from_sml_property(property));
}

void not_condition::process_sml_scope(const sml_data &scope)
{
	this->conditions.push_back(condition::from_sml_scope(scope));
}

void not_condition::ProcessConfigDataSection(const CConfigData *section)
{
	condition *condition = nullptr;
	if (section->Tag == "and") {
		condition = new and_condition;
	} else if (section->Tag == "or") {
		condition = new or_condition;
	} else if (section->Tag == "not") {
		condition = new not_condition;
	} else if (section->Tag == "unit_type") {
		condition = new unit_type_condition;
	} else if (section->Tag == "upgrade") {
		condition = new upgrade_condition;
	} else if (section->Tag == "age") {
		condition = new age_condition;
	} else if (section->Tag == "character") {
		condition = new character_condition;
	} else if (section->Tag == "season") {
		condition = new season_condition;
	} else if (section->Tag == "trigger") {
		condition = new trigger_condition;
	} else {
		fprintf(stderr, "Invalid not condition property: \"%s\".\n", section->Tag.c_str());
		return;
	}
	condition->ProcessConfigData(section);
	this->conditions.push_back(std::unique_ptr<stratagus::condition>(condition));
}

bool not_condition::check(const CPlayer *player, bool ignore_units) const
{
	for (const auto &condition : this->conditions) {
		if (condition->check(player, ignore_units)) {
			return false;
		}
	}
	
	return true;
}

bool not_condition::check(const CUnit *unit, bool ignore_units) const
{
	for (const auto &condition : this->conditions) {
		if (condition->check(unit, ignore_units)) {
			return false;
		}
	}
	
	return true;
}

void unit_type_condition::process_sml_property(const sml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "unit_type") {
		this->unit_type = unit_type::get(value);
	} else if (key == "count") {
		this->count = std::stoi(value);
	} else if (key == "settlement") {
		this->settlement = site::get(value);
	} else {
		throw std::runtime_error("Invalid unit type condition property: \"" + property.get_key() + "\".");
	}
}

void unit_type_condition::ProcessConfigDataProperty(const std::pair<std::string, std::string> &property)
{
	const std::string &key = property.first;
	std::string value = property.second;
	if (key == "unit_type") {
		this->unit_type = unit_type::get(value);
	} else if (key == "count") {
		this->count = std::stoi(value);
	} else {
		fprintf(stderr, "Invalid unit type condition property: \"%s\".\n", key.c_str());
	}
}

bool unit_type_condition::check(const CPlayer *player, bool ignore_units) const
{
	if (ignore_units) {
		return true;
	}
	
	if (this->settlement != nullptr) {
		if (!player->HasSettlement(this->settlement)) {
			return false;
		}

		std::vector<CUnit *> units;
		FindPlayerUnitsByType(*player, *this->unit_type, units);

		int counter = 0;
		for (const CUnit *unit : units) {
			if (unit->settlement == this->settlement) {
				counter++;

				if (counter >= this->count) {
					return true;
				}
			}
		}

		return false;
	} else {
		return player->GetUnitTypeCount(this->unit_type) >= this->count;
	}
}

std::string unit_type_condition::get_string(const std::string &prefix) const
{
	std::string str = prefix + this->unit_type->get_name();
	
	if (this->count > 1) {
		str += '(' + std::to_string(this->count) + ')';
	}

	if (this->settlement != nullptr) {
		str += " in " + settlement->get_name();
	}
	
	str += '\n';
	
	return str;
}

void upgrade_condition::process_sml_property(const sml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "upgrade") {
		this->Upgrade = CUpgrade::get(value);
	} else {
		throw std::runtime_error("Invalid upgrade condition property: \"" + property.get_key() + "\".");
	}
}

void upgrade_condition::ProcessConfigDataProperty(const std::pair<std::string, std::string> &property)
{
	const std::string &key = property.first;
	std::string value = property.second;
	if (key == "upgrade") {
		value = FindAndReplaceString(value, "_", "-");
		this->Upgrade = CUpgrade::get(value);
	} else {
		fprintf(stderr, "Invalid upgrade condition property: \"%s\".\n", key.c_str());
	}
}

bool upgrade_condition::check(const CPlayer *player, bool ignore_units) const
{
	return UpgradeIdAllowed(*player, this->Upgrade->ID) == 'R';
}

bool upgrade_condition::check(const CUnit *unit, bool ignore_units) const
{
	return this->check(unit->Player, ignore_units) || unit->GetIndividualUpgrade(this->Upgrade);
}

std::string upgrade_condition::get_string(const std::string &prefix) const
{
	std::string str = prefix + this->Upgrade->get_name() + '\n';
	return str;
}

void age_condition::ProcessConfigDataProperty(const std::pair<std::string, std::string> &property)
{
	const std::string &key = property.first;
	std::string value = property.second;
	if (key == "age") {
		this->age = age::get(value);
	} else {
		fprintf(stderr, "Invalid age condition property: \"%s\".\n", key.c_str());
	}
}

void character_condition::process_sml_property(const sml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "character") {
		this->character = character::get(value);
	} else {
		throw std::runtime_error("Invalid character condition property: \"" + property.get_key() + "\".");
	}
}

void character_condition::ProcessConfigDataProperty(const std::pair<std::string, std::string> &property)
{
	const std::string &key = property.first;
	std::string value = property.second;
	if (key == "character") {
		this->character = character::get(value);
	} else {
		fprintf(stderr, "Invalid character condition property: \"%s\".\n", key.c_str());
	}
}

bool character_condition::check(const CPlayer *player, bool ignore_units) const
{
	return player->HasHero(this->character);
}

bool character_condition::check(const CUnit *unit, bool ignore_units) const
{
	return unit->Character == this->character;
}

std::string character_condition::get_string(const std::string &prefix) const
{
	std::string str = prefix + this->character->GetFullName() + '\n';
	return str;
}

void season_condition::ProcessConfigDataProperty(const std::pair<std::string, std::string> &property)
{
	const std::string &key = property.first;
	const std::string &value = property.second;

	if (key == "season") {
		this->Season = season::get(value);
	} else {
		fprintf(stderr, "Invalid season condition property: \"%s\".\n", key.c_str());
	}
}

bool season_condition::check(const CPlayer *player, bool ignore_units) const
{
	return CMap::Map.MapLayers[player->StartMapLayer]->GetSeason() == this->Season;
}

bool season_condition::check(const CUnit *unit, bool ignore_units) const
{
	return unit->MapLayer->GetSeason() == this->Season;
}

std::string season_condition::get_string(const std::string &prefix) const
{
	std::string str = prefix + this->Season->get_name() + '\n';
	return str;
}

void settlement_condition::process_sml_property(const sml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "settlement") {
		this->settlement = site::get(value);
	} else if (key == "faction") {
		this->faction = faction::get(value);
	} else if (key == "enemy") {
		this->enemy = string::to_bool(value);
	} else {
		throw std::runtime_error("Invalid settlement condition property: \"" + property.get_key() + "\".");
	}
}

bool settlement_condition::check(const CPlayer *player, bool ignore_units) const
{
	Q_UNUSED(ignore_units)

	if (this->faction != nullptr) {
		const CPlayer *faction_player = GetFactionPlayer(this->faction);
		if (faction_player == nullptr) {
			return false;
		}

		if (this->enemy && !faction_player->IsEnemy(*player)) {
			return false;
		}

		return faction_player->HasSettlement(this->settlement);
	}

	return player->HasSettlement(this->settlement);
}

std::string settlement_condition::get_string(const std::string &prefix) const
{
	std::string str = prefix + this->settlement->get_name() + '\n';
	return str;
}

void trigger_condition::ProcessConfigDataProperty(const std::pair<std::string, std::string> &property)
{
	const std::string &key = property.first;
	std::string value = property.second;
	if (key == "trigger") {
		this->trigger = trigger::get(value);
	} else {
		fprintf(stderr, "Invalid trigger condition property: \"%s\".\n", key.c_str());
	}
}

bool trigger_condition::check(const CPlayer *player, bool ignore_units) const
{
	//checks whether a trigger has already fired
	
	return vector::contains(trigger::DeactivatedTriggers, this->trigger->get_identifier()); //this works fine for global triggers, but for player triggers perhaps it should check only the player?
}

std::string trigger_condition::get_string(const std::string &prefix) const
{
	return std::string();
}

}

/**
**  Check if this upgrade or unit is available.
**
**  @param player  For this player available.
**  @param target  Unit or Upgrade.
**
**  @return        True if available, false otherwise.
*/
std::string PrintConditions(const CPlayer &player, const stratagus::button &button)
{
	std::string rules;

	//
	//  first have to check, if target is allowed itself
	//
	if (!strncmp(button.ValueStr.c_str(), "unit-", 5)) {
		// target string refers to unit-XXX
		const stratagus::unit_type *unit_type = stratagus::unit_type::get(button.ValueStr);
		rules = unit_type->get_conditions()->get_string();
	} else if (!strncmp(button.ValueStr.c_str(), "upgrade", 7)) {
		// target string refers to upgrade-XXX
		const CUpgrade *upgrade = CUpgrade::get(button.ValueStr);
		if (upgrade->get_conditions()) {
			rules = upgrade->get_conditions()->get_string();
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

bool CheckConditions(const stratagus::unit_type *target, const CPlayer *player, bool ignore_units, bool is_precondition, bool is_neutral_use)
{
	if (!is_precondition && !CheckConditions(target, player, ignore_units, true, is_neutral_use)) {
		return false;
	}
	
	if (UnitIdAllowed(*player, target->Slot) == 0) {
		return false;
	}
	
	if (is_precondition) {
		return target->get_preconditions() == nullptr || target->get_preconditions()->check(player, ignore_units);
	} else {
		return target->get_conditions() == nullptr || target->get_conditions()->check(player, ignore_units);
	}
}

bool CheckConditions(const CUpgrade *target, const CPlayer *player, bool ignore_units, bool is_precondition, bool is_neutral_use)
{
	if (!is_precondition && !CheckConditions(target, player, ignore_units, true, is_neutral_use)) {
		return false;
	}
	
	if (UpgradeIdAllowed(*player, target->ID) != 'A' && !((is_precondition || is_neutral_use) && UpgradeIdAllowed(*player, target->ID) == 'R')) {
		return false;
	}

	if (player->Faction != -1 && stratagus::faction::get_all()[player->Faction]->Type == FactionTypeHolyOrder) { // if the player is a holy order, and the upgrade is incompatible with its deity, don't allow it
		if (stratagus::faction::get_all()[player->Faction]->HolyOrderDeity) {
			CUpgrade *deity_upgrade = stratagus::faction::get_all()[player->Faction]->HolyOrderDeity->DeityUpgrade;
			if (deity_upgrade) {
				for (const auto &modifier : target->get_modifiers()) {
					if (stratagus::vector::contains(modifier->RemoveUpgrades, deity_upgrade)) {
						return false;
					}
				}
				for (const auto &modifier : deity_upgrade->get_modifiers()) {
					if (stratagus::vector::contains(modifier->RemoveUpgrades, target)) {
						return false;
					}
				}
			}
		}
	}
	
	if (is_precondition) {
		return !target->get_preconditions() || target->get_preconditions()->check(player, ignore_units);
	} else {
		return !target->get_conditions() || target->get_conditions()->check(player, ignore_units);
	}
}

bool CheckConditions(const stratagus::unit_type *target, const CUnit *unit, bool ignore_units, bool is_precondition)
{
	if (!is_precondition && !CheckConditions(target, unit, ignore_units, true)) {
		return false;
	}
	
	if (UnitIdAllowed(*unit->Player, target->Slot) == 0) {
		return false;
	}
	
	if (is_precondition) {
		return target->get_preconditions() == nullptr || target->get_preconditions()->check(unit, ignore_units);
	} else {
		return target->get_conditions() == nullptr || target->get_conditions()->check(unit, ignore_units);
	}
}

bool CheckConditions(const CUpgrade *target, const CUnit *unit, bool ignore_units, bool is_precondition)
{
	if (!is_precondition && !CheckConditions(target, unit, ignore_units, true)) {
		return false;
	}
	
	if (UpgradeIdAllowed(*unit->Player, target->ID) == 'F') {
		return false;
	}

	if (is_precondition) {
		return target->get_preconditions() == nullptr || target->get_preconditions()->check(unit, ignore_units);
	} else {
		return target->get_conditions() || target->get_conditions()->check(unit, ignore_units);
	}
}

/*----------------------------------------------------------------------------
--  Ccl part of conditions
----------------------------------------------------------------------------*/

static int CclDefineDependency(lua_State *l)
{
	const int args = lua_gettop(l);
	const char *target = LuaToString(l, 1);

	std::vector<std::unique_ptr<const stratagus::condition>> and_conditions;
	
	//  All or rules.
	bool or_flag = false;
	for (int j = 1; j < args; ++j) {
		if (!lua_istable(l, j + 1)) {
			LuaError(l, "incorrect argument");
		}
		const int subargs = lua_rawlen(l, j + 1);

		std::vector<std::unique_ptr<const stratagus::condition>> conditions;
	
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
			stratagus::condition *condition = nullptr;
			
			if (!strncmp(required, "unit-", 5)) {
				const stratagus::unit_type *unit_type = stratagus::unit_type::get(required);
				condition = new stratagus::unit_type_condition(unit_type, count > 0 ? count : 1);
			} else if (!strncmp(required, "upgrade", 7)) {
				const CUpgrade *upgrade = CUpgrade::get(required);
				condition = new stratagus::upgrade_condition(upgrade);
			} else {
				LuaError(l, "Invalid required type for condition: \"%s\"" _C_ required);
			}
			
			if (count == 0) {
				condition = new stratagus::not_condition(std::unique_ptr<stratagus::condition>(condition));
			}
			
			conditions.push_back(std::unique_ptr<stratagus::condition>(condition));
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
		
		and_conditions.push_back(std::make_unique<stratagus::and_condition>(std::move(conditions)));
		conditions.clear();
	}
	
	std::unique_ptr<stratagus::condition> condition;
	if (or_flag) {
		condition = std::make_unique<stratagus::or_condition>(std::move(and_conditions));
	} else {
		condition = std::make_unique<stratagus::and_condition>(std::move(and_conditions));
	}
	
	if (!strncmp(target, "unit-", 5)) {
		stratagus::unit_type *unit_type = stratagus::unit_type::get(target);
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

	std::vector<std::unique_ptr<const stratagus::condition>> and_conditions;
	
	//  All or rules.
	bool or_flag = false;
	for (int j = 1; j < args; ++j) {
		if (!lua_istable(l, j + 1)) {
			LuaError(l, "incorrect argument");
		}
		const int subargs = lua_rawlen(l, j + 1);

		std::vector<std::unique_ptr<const stratagus::condition>> conditions;
	
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
			stratagus::condition *condition = nullptr;
			
			if (!strncmp(required, "unit-", 5)) {
				const stratagus::unit_type *unit_type = stratagus::unit_type::get(required);
				condition = new stratagus::unit_type_condition(unit_type, count > 0 ? count : 1);
			} else if (!strncmp(required, "upgrade", 7)) {
				const CUpgrade *upgrade = CUpgrade::get(required);
				condition = new stratagus::upgrade_condition(upgrade);
			} else {
				LuaError(l, "Invalid required type for condition: \"%s\"" _C_ required);
			}
			
			if (count == 0) {
				condition = new stratagus::not_condition(std::unique_ptr<stratagus::condition>(condition));
			}
			
			conditions.push_back(std::unique_ptr<stratagus::condition>(condition));
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
		
		and_conditions.push_back(std::make_unique<stratagus::and_condition>(std::move(conditions)));
		conditions.clear();
	}
	
	std::unique_ptr<stratagus::condition> condition;
	if (or_flag) {
		condition = std::make_unique<stratagus::or_condition>(std::move(and_conditions));
	} else {
		condition = std::make_unique<stratagus::and_condition>(std::move(and_conditions));
	}
	
	if (!strncmp(target, "unit-", 5)) {
		stratagus::unit_type *unit_type = stratagus::unit_type::get(target);
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
	const CPlayer *player = CPlayer::Players[plynr];
	
	if (!strncmp(object, "unit-", 5)) {
		const stratagus::unit_type *unit_type = stratagus::unit_type::get(object);
		lua_pushboolean(l, CheckConditions(unit_type, player));
	} else if (!strncmp(object, "upgrade", 7)) {
		const CUpgrade *upgrade = CUpgrade::get(object);
		lua_pushboolean(l, CheckConditions(upgrade, player));
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
