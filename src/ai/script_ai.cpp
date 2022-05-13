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
/**@name script_ai.cpp - The AI ccl functions. */
//
//      (c) Copyright 2000-2022 by Lutz Sammer, Ludovic Pollet,
//      Jimmy Salmon and Andrettin
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

#include "ai.h"
#include "ai_local.h"

#include "database/defines.h"
#include "economy/resource.h"
#include "map/map.h"
#include "map/site.h"
#include "pathfinder/pathfinder.h"
#include "player/civilization.h"
#include "player/player.h"
#include "script.h"
#include "ui/button.h"
#include "ui/button_cmd.h"
#include "ui/interface.h"
#include "unit/unit.h"
#include "unit/unit_class.h"
#include "unit/unit_domain.h"
#include "unit/unit_manager.h"
#include "unit/unit_type.h"
#include "upgrade/upgrade.h"
#include "upgrade/upgrade_class.h"
#include "util/assert_util.h"
#include "util/log_util.h"
#include "util/vector_util.h"

/**
**  Insert new unit-type element.
**
**  @param table  Table with elements.
**  @param n      Index to insert new into table
**  @param base   Base type to insert into table.
*/
static void AiHelperInsert(std::vector<std::vector<const wyrmgus::unit_type *> > &table,
						   unsigned int n, const wyrmgus::unit_type *base)
{
	if (n >= table.size()) {
		table.resize(n + 1);
	}
	// Look if already known
	std::vector<const wyrmgus::unit_type *>::const_iterator it = std::find(table[n].begin(), table[n].end(), base);
	if (it != table[n].end()) {
		return;
	}
	table[n].push_back(base);
}

static void AiHelperInsert(std::vector<const wyrmgus::unit_type *> &table, const wyrmgus::unit_type *base)
{
	// Look if already known
	if (wyrmgus::vector::contains(table, base)) {
		return;
	}
	table.push_back(base);
}

static void AiHelperInsert(std::map<const CUpgrade *, std::vector<const wyrmgus::unit_type *>> &table,
	const CUpgrade *key, const wyrmgus::unit_type *target)
{
	if (table.contains(key) && wyrmgus::vector::contains(table[key], target)) {
		return;
	}

	table[key].push_back(target);
}

static void AiHelperInsert(std::map<const wyrmgus::upgrade_class *, std::vector<const wyrmgus::unit_class *>> &table,
	const wyrmgus::upgrade_class *key, const wyrmgus::unit_class *target)
{
	if (table.contains(key) && wyrmgus::vector::contains(table[key], target)) {
		return;
	}

	table[key].push_back(target);
}

template <typename target_type>
static void AiHelperInsert(wyrmgus::unit_type_map<std::vector<const target_type *>> &table,
	const wyrmgus::unit_type *key, const target_type *target)
{
	if (table.contains(key) && wyrmgus::vector::contains(table[key], target)) {
		return;
	}

	table[key].push_back(target);
}

template <typename target_type>
static void AiHelperInsert(wyrmgus::unit_class_map<std::vector<const target_type *>> &table,
	const wyrmgus::unit_class *key, const target_type *target)
{
	if (table.contains(key) && wyrmgus::vector::contains(table[key], target)) {
		return;
	}

	table[key].push_back(target);
}

//Wyrmgus start
/**
**  Insert new upgrade element.
**
**  @param table  Table with elements.
**  @param n      Index to insert new into table
**  @param base   Base type to insert into table.
*/
static void AiHelperInsert(std::vector<std::vector<const CUpgrade *> > &table,
						   unsigned int n, const CUpgrade *base)
{
	if (n >= table.size()) {
		table.resize(n + 1);
	}
	// Look if already known
	std::vector<const CUpgrade *>::const_iterator it = std::find(table[n].begin(), table[n].end(), base);
	if (it != table[n].end()) {
		return;
	}
	table[n].push_back(base);
}
//Wyrmgus end

/**
**  Transform list of unit separed with coma to a true list.
*/
static std::vector<const wyrmgus::unit_type *> get_unit_type_from_string(const std::string &list)
{
	std::vector<const wyrmgus::unit_type *> res;

	if (list == "*") {
		for (const wyrmgus::unit_type *unit_type : wyrmgus::unit_type::get_all()) {
			if (unit_type->is_template()) {
				continue;
			}

			res.push_back(unit_type);
		}
	} else {
		size_t begin = 1;
		size_t end = list.find(",", begin);
		while (end != std::string::npos) {
			std::string unitName = list.substr(begin, end - begin);
			begin = end + 1;
			end = list.find(",", begin);
			if (!unitName.empty()) {
				assert_throw(unitName[0] != ',');
				const wyrmgus::unit_type *unit_type = wyrmgus::unit_type::try_get(unitName);
				if (unit_type != nullptr) {
					res.push_back(unit_type);
				}
			}
		}
	}

	return res;
}

/**
**  Get list of unit types which can be repared.
*/
static std::vector<const wyrmgus::unit_type *> get_reparable_units()
{
	std::vector<const wyrmgus::unit_type *> res;

	for (const wyrmgus::unit_type *unit_type : wyrmgus::unit_type::get_all()) {
		if (unit_type->is_template()) {
			continue;
		}

		if (unit_type->get_repair_hp() > 0) {
			res.push_back(unit_type);
		}
	}

	return res;
}

/**
**  Get sorted list of unittype with Supply not null.
**
**  @note Better (supply / cost) first.
*/
static std::vector<const wyrmgus::unit_type *> get_supply_units()
{
	std::vector<const wyrmgus::unit_type *> res;

	for (const wyrmgus::unit_type *unit_type : wyrmgus::unit_type::get_all()) {
		if (unit_type->is_template()) {
			continue;
		}

		if (unit_type->BoolFlag[TOWNHALL_INDEX].value) {
			continue;
		}

		if (unit_type->DefaultStat.Variables[SUPPLY_INDEX].Value > 0) { //supply units are identified as being those with a default stat supply of 1 or more; so if a unit has a supply default stat of 0, but through an upgrade ends up having 1 or more supply, it won't be included here
			res.push_back(unit_type);
		}
	}

	//now, sort them, best first.
	std::sort(res.begin(), res.end(), [](const wyrmgus::unit_type *type, const wyrmgus::unit_type *other_type) {
		unsigned int cost = 0;
		unsigned int other_cost = 0;

		for (const resource *resource : resource::get_all()) {
			cost += type->DefaultStat.get_cost(resource);
			other_cost += other_type->DefaultStat.get_cost(resource);
		}

		const unsigned int score = type->DefaultStat.Variables[SUPPLY_INDEX].Value * 10000 / cost;
		const unsigned int other_score = other_type->DefaultStat.Variables[SUPPLY_INDEX].Value * 10000 / cost;

		return score > other_score;
	});

	return res;
}

/**
**  Get sorted list of unittype with CanHarvest not null.
**
**  @note Better (MaxOnBoard / cost) first.
*/
static std::vector<const wyrmgus::unit_type *> get_mine_units()
{
	std::vector<const wyrmgus::unit_type *> res;

	for (const wyrmgus::unit_type *unit_type : wyrmgus::unit_type::get_all()) {
		if (unit_type->is_template()) {
			continue;
		}

		if (unit_type->get_given_resource() != nullptr && unit_type->BoolFlag[CANHARVEST_INDEX].value) {
			res.push_back(unit_type);
		}
	}

#if 0
	std::vector<wyrmgus::unit_type *> sorted_res;
	// Now, sort them, best first.
	while (!res.empty()) {
		float bestscore;
		wyrmgus::unit_type *besttype;

		bestscore = 0;
		for (std::vector<wyrmgus::unit_type *>::const_iterator i = res.begin(); i != res.end(); ++i) {
			wyrmgus::unit_type *type = *i;
			float score;
			unsigned int cost = 0;

			for (size_t j = 0; j < wyrmgus::resource::get_all().size(); ++j) {
				cost += type->_Costs[j];
			}
			score = ((float) type->MaxOnBoard) / cost;
			if (score > bestscore) {
				bestscore = score;
				besttype = type;
			}
		}
		sorted_res.push_back(besttype);
		for (std::vector<wyrmgus::unit_type *>::iterator i = res.begin(); i != res.end(); ++i) {
			if (*i == besttype) {
				i = res.erase(i);
				break;
			}
		}
	}
	return sorted_res;
#else
	return res;
#endif

}

//Wyrmgus start
/**
**  Get list of unit types which are markets.
*/
static std::vector<const wyrmgus::unit_type *> get_market_units()
{
	std::vector<const wyrmgus::unit_type *> res;

	for (const wyrmgus::unit_type *unit_type : wyrmgus::unit_type::get_all()) {
		if (unit_type->is_template()) {
			continue;
		}

		if (unit_type->BoolFlag[MARKET_INDEX].value) {
			res.push_back(unit_type);
		}
	}
	return res;
}

/**
**  Get list of unit types which are transporters.
*/
static std::vector<const wyrmgus::unit_type *> get_naval_transporter_units()
{
	std::vector<const wyrmgus::unit_type *> res;

	for (const wyrmgus::unit_type *unit_type : wyrmgus::unit_type::get_all()) {
		if (unit_type->is_template()) {
			continue;
		}

		if (unit_type->CanTransport() && (unit_type->get_domain() == unit_domain::water || unit_type->get_domain() == unit_domain::air || unit_type->get_domain() == unit_domain::air_low || unit_type->get_domain() == unit_domain::space)) { //if the unit is a transporter that can travel through water (not necessarily a ship, can also fly)
			res.push_back(unit_type);
		}
	}

	return res;
}
//Wyrmgus end

/**
**  Init AiHelper.
**
**  @param aiHelper  variable to initialise.
**
**  @todo missing Equiv initialisation.
*/
static void InitAiHelper(AiHelper &aiHelper)
{
	//Wyrmgus start
	//free AI helper (except for equivs) before initializing, in case this is a re-definition
	AiHelpers.clear();
	//Wyrmgus end
	
	const std::vector<const unit_type *> reparable_units = get_reparable_units();
	const std::vector<const unit_type *> supply_units = get_supply_units();
	const std::vector<const unit_type *> mine_units = get_mine_units();
	//Wyrmgus start
	const std::vector<const unit_type *> market_units = get_market_units();
	const std::vector<const unit_type *> naval_transporter_units = get_naval_transporter_units();
	//Wyrmgus end

	for (const unit_type *supply_unit_type : supply_units) {
		AiHelperInsert(aiHelper.UnitLimit, supply_unit_type);
	}
	//Wyrmgus start
	for (const unit_type *sell_market : market_units) {
		AiHelperInsert(aiHelper.SellMarkets, 0, sell_market);
	}
	for (const unit_type *naval_transporter : naval_transporter_units) {
		AiHelperInsert(aiHelper.NavalTransporters, naval_transporter);
	}
	//Wyrmgus end

	for (const resource *resource : resource::get_all()) {
		if (resource == defines::get()->get_time_resource()) {
			continue;
		}

		for (const unit_type *mine_unit_type : mine_units) {
			if (mine_unit_type->get_given_resource() == resource) {
				AiHelperInsert(aiHelper.Mines, resource->get_index(), mine_unit_type);
			}
		}
		for (const unit_type *unit_type : unit_type::get_all()) {
			if (unit_type->is_template()) {
				continue;
			}

			if (unit_type->can_store(resource)) {
				AiHelperInsert(aiHelper.Depots, resource->get_index(), unit_type);
			}
		}
	}

	for (const button *button : button::get_all()) {
		std::vector<const unit_type *> unitmask = get_unit_type_from_string(button->UnitMask);
		for (const unit_class *unit_class : button->get_unit_classes()) {
			vector::merge(unitmask, unit_class->get_unit_types());
		}

		switch (button->Action) {
			case ButtonCmd::Repair:
				for (const unit_type *repairer : unitmask) {
					for (const unit_type *reparable_unit_type : reparable_units) {
						AiHelperInsert(aiHelper.Repair, reparable_unit_type->get_index(), repairer);
					}
				}
				break;
			case ButtonCmd::Build: {
				const unit_type *buildingType = unit_type::get(button->ValueStr);

				for (const unit_type *builder : unitmask) {
					AiHelperInsert(aiHelper.builders, buildingType, builder);
				}
				break;
			}
			case ButtonCmd::BuildClass: {
				const unit_class *built_class = unit_class::get(button->ValueStr);

				for (const unit_class *builder : button->get_unit_classes()) {
					AiHelperInsert(aiHelper.builder_classes, built_class, builder);
				}
				break;
			}
			case ButtonCmd::Train : {
				const unit_type *trained_type = unit_type::get(button->ValueStr);

				for (const unit_type *trainer : unitmask) {
					AiHelperInsert(aiHelper.trainers, trained_type, trainer);
				}
				break;
			}
			case ButtonCmd::TrainClass : {
				const unit_class *trained_class = unit_class::get(button->ValueStr);

				for (const unit_class *trainer : button->get_unit_classes()) {
					AiHelperInsert(aiHelper.trainer_classes, trained_class, trainer);
				}
				break;
			}
			case ButtonCmd::UpgradeTo : {
				const unit_type *upgrade_to_type = unit_type::get(button->ValueStr);

				for (const unit_type *unit_type : unitmask) {
					AiHelperInsert(aiHelper.unit_type_upgrades, unit_type, upgrade_to_type);
					AiHelperInsert(aiHelper.unit_type_upgradees, upgrade_to_type, unit_type);
				}
				break;
			}
			case ButtonCmd::UpgradeToClass: {
				const unit_class *upgrade_to_class = unit_class::get(button->ValueStr);

				for (const unit_class *unit_class : button->get_unit_classes()) {
					AiHelperInsert(aiHelper.unit_class_upgrades, unit_class, upgrade_to_class);
					AiHelperInsert(aiHelper.unit_class_upgradees, upgrade_to_class, unit_class);
				}
				break;
			}
			case ButtonCmd::Research : {
				const CUpgrade *upgrade = CUpgrade::get(button->ValueStr);

				for (const unit_type *researcher : unitmask) {
					AiHelperInsert(aiHelper.researchers, upgrade, researcher);
					AiHelperInsert(aiHelper.researched_upgrades, researcher, upgrade);
				}
				break;
			}
			case ButtonCmd::ResearchClass: {
				const upgrade_class *upgrade_class = upgrade_class::get(button->ValueStr);

				for (const unit_class *researcher : button->get_unit_classes()) {
					AiHelperInsert(aiHelper.researcher_classes, upgrade_class, researcher);
					AiHelperInsert(aiHelper.researched_upgrade_classes, researcher, upgrade_class);
				}
				break;
			}
			case ButtonCmd::SellResource : {
				const int resource = GetResourceIdByName(button->ValueStr.c_str());

				for (const unit_type *sell_market : unitmask) {
					AiHelperInsert(aiHelper.SellMarkets, resource - 1, sell_market);
				}
				break;
			}
			case ButtonCmd::BuyResource : {
				int resource = GetResourceIdByName(button->ValueStr.c_str());

				for (const unit_type *buy_market : unitmask) {
					AiHelperInsert(aiHelper.BuyMarkets, resource - 1, buy_market);
				}
				break;
			}
			case ButtonCmd::ProduceResource : {
				const resource *resource = resource::get(button->ValueStr);

				for (const unit_type *producer : unitmask) {
					AiHelperInsert(aiHelper.produced_resources, producer, resource);
				}
				break;
			}
			case ButtonCmd::ExperienceUpgradeTo : {
				const unit_type *upgrade_to_type = unit_type::get(button->ValueStr);

				for (const unit_type *experience_upgradee : unitmask) {
					AiHelperInsert(aiHelper.ExperienceUpgrades, experience_upgradee->get_index(), upgrade_to_type);
				}
				break;
			}
			case ButtonCmd::LearnAbility : {
				const CUpgrade *ability = CUpgrade::get(button->ValueStr);

				if (ability->is_ability()) {
					for (const unit_type *learner : unitmask) {
						AiHelperInsert(aiHelper.LearnableAbilities, learner->get_index(), ability);
					}
				}
				break;
			}
			default:
				break;
		}
	}
}

/**
**  Define helper for AI.
**
**  @param l  Lua state.
**
**  @todo  FIXME: the first unit could be a list see ../doc/ccl/ai.html
*/
static int CclDefineAiHelper(lua_State *l)
{
	InitAiHelper(AiHelpers);

	const int args = lua_gettop(l);
	for (int j = 0; j < args; ++j) {
		if (!lua_istable(l, j + 1)) {
			LuaError(l, "incorrect argument");
		}
		const int subargs = lua_rawlen(l, j + 1);
		const char *value = LuaToString(l, j + 1, 1);
		if (!strcmp(value, "build")
			|| !strcmp(value, "train")
			|| !strcmp(value, "upgrade")
			|| !strcmp(value, "research")
			|| !strcmp(value, "unit-limit")
			//Wyrmgus start
//			|| !strcmp(value, "repair")) {
			|| !strcmp(value, "repair")
			|| !strcmp(value, "experience-upgrade")
			|| !strcmp(value, "learnable-ability")
		) {
			//Wyrmgus end
#ifdef DEBUG
			log::log_error("DefineAiHelper: Relation is computed from buttons, you may remove safely the block beginning with '\"" + std::string(value) + "\"'");
#endif
			continue;
		} else if (!strcmp(value, "unit-equiv")) {
		} else {
			LuaError(l, "unknown tag: %s" _C_ value);
		}
		// Get the base unit type, which could handle the action.
		const char *baseTypeName = LuaToString(l, j + 1, 2);
		const wyrmgus::unit_type *base = wyrmgus::unit_type::get(baseTypeName);

		// Get the unit types, which could be produced
		for (int k = 2; k < subargs; ++k) {
			const char *equivTypeName = LuaToString(l, j + 1, k + 1);
			const wyrmgus::unit_type *type = wyrmgus::unit_type::get(equivTypeName);
			AiHelperInsert(AiHelpers.Equiv, base->Slot, type);
			AiNewUnitTypeEquiv(*base, *type);
		}
	}
	return 0;
}

static CAiType *GetAiTypesByName(const char *name)
{
	for (const std::unique_ptr<CAiType> &ait : AiTypes) {
		if (ait->Name == name) {
			return ait.get();
		}
	}
	return nullptr;
}

/**
**  Define an AI engine.
**
**  @param l  Lua state.
**
**  @return   FIXME: docu
*/
static int CclDefineAi(lua_State *l)
{
	LuaCheckArgs(l, 4);
	if (!lua_isfunction(l, 4)) {
		LuaError(l, "incorrect argument");
	}

	auto aitype = std::make_unique<CAiType>();

	// AI Name
	const char *aiName = LuaToString(l, 1);
	aitype->Name = aiName;

#ifdef DEBUG
	if (GetAiTypesByName(aiName)) {
		DebugPrint("Warning two or more AI's with the same name '%s'\n" _C_ aiName);
	}
#endif

	// AI Race
	const char *value = LuaToString(l, 2);
	if (*value != '*') {
		aitype->Race = value;
	} else {
		aitype->Race.clear();
	}

	// AI Class
	aitype->Class = LuaToString(l, 3);

	// AI Script
	lua_getglobal(l, "_ai_scripts_");
	if (lua_isnil(l, -1)) {
		lua_pop(l, 1);
		lua_newtable(l);
		lua_setglobal(l, "_ai_scripts_");
		lua_getglobal(l, "_ai_scripts_");
	}
	aitype->Script = aitype->Name + aitype->Race + aitype->Class;
	lua_pushstring(l, aitype->Script.c_str());
	lua_pushvalue(l, 4);
	lua_rawset(l, 5);
	lua_pop(l, 1);

	AiTypes.insert(AiTypes.begin(), std::move(aitype));

	return 0;
}

/*----------------------------------------------------------------------------
--  AI script functions
----------------------------------------------------------------------------*/

/**
**  Append unit-type to request table.
**
**  @param type   Unit-type to be appended.
**  @param count  How many unit-types to build.
*/
static void InsertUnitTypeRequests(wyrmgus::unit_type *type, int count)
{
	AiRequestType ait;

	ait.Type = type;
	ait.Count = count;

	AiPlayer->UnitTypeRequests.push_back(ait);
}

/**
**  Find unit-type in request table.
**
**  @param type  Unit-type to be found.
*/
static AiRequestType *FindInUnitTypeRequests(const wyrmgus::unit_type *type)
{
	const size_t n = AiPlayer->UnitTypeRequests.size();

	for (size_t i = 0; i < n; ++i) {
		if (AiPlayer->UnitTypeRequests[i].Type == type) {
			return &AiPlayer->UnitTypeRequests[i];
		}
	}
	return nullptr;
}

//Wyrmgus start
/**
**  Remove unit-type in request table.
**
**  @param type  Unit-type to be removed.
*/
static void RemoveFromUnitTypeRequests(const wyrmgus::unit_type *type)
{
	std::vector<AiRequestType>::iterator i;

	for (i = AiPlayer->UnitTypeRequests.begin(); i != AiPlayer->UnitTypeRequests.end(); ++i) {
		if (type == (*i).Type) {
			AiPlayer->UnitTypeRequests.erase(i);
			break;
		}
	}
}
//Wyrmgus end

/**
**  Find unit-type in upgrade-to table.
**
**  @param type  Unit-type to be found.
*/
static int FindInUpgradeToRequests(const wyrmgus::unit_type *type)
{
	const size_t n = AiPlayer->UpgradeToRequests.size();
	for (size_t i = 0; i < n; ++i) {
		if (AiPlayer->UpgradeToRequests[i] == type) {
			return 1;
		}
	}
	return 0;
}

/**
**  Append unit-type to request table.
**
**  @param type  Unit-type to be appended.
*/
static void InsertUpgradeToRequests(wyrmgus::unit_type *type)
{
	AiPlayer->UpgradeToRequests.push_back(type);
}

/**
**  Append unit-type to request table.
**
**  @param upgrade  Upgrade to be appended.
*/
static void InsertResearchRequests(const CUpgrade *upgrade)
{
	AiPlayer->add_research_request(upgrade);
}

//----------------------------------------------------------------------------

/**
**  Get the race of the current AI player.
**
**  @param l  Lua state.
*/
static int CclAiGetRace(lua_State *l)
{
	LuaCheckArgs(l, 0);
	lua_pushstring(l, AiPlayer->Player->get_civilization()->get_identifier().c_str());
	return 1;
}

/**
**  Get the number of cycles to sleep.
**
**  @param l  Lua state
**
**  @return   Number of return values
*/
static int CclAiGetSleepCycles(lua_State *l)
{
	LuaCheckArgs(l, 0);
	lua_pushnumber(l, AiSleepCycles);
	return 1;
}

//----------------------------------------------------------------------------

/**
**  Set debugging flag of AI script
**
**  @param l  Lua state
**
**  @return   Number of return values
*/
static int CclAiDebug(lua_State *l)
{
	LuaCheckArgs(l, 1);
	AiPlayer->ScriptDebug = LuaToBoolean(l, 1);
	return 0;
}

/**
**  Activate AI debugging for the given player(s)
**  Player can be a number for a specific player
**    "self" for current human player (ai me)
**    "none" to disable
**
**  @param l  Lua State
**
**  @return   Number of return values
*/
static int CclAiDebugPlayer(lua_State *l)
{
	const int args = lua_gettop(l);
	for (int j = 0; j < args; ++j) {
		const char *item;

		if (lua_isstring(l, j + 1)) {
			item = LuaToString(l, j + 1);
		} else {
			item = nullptr;
		}
		if (item && !strcmp(item, "none")) {
			for (int i = 0; i != NumPlayers; ++i) {
				if (!CPlayer::Players[i]->AiEnabled || !CPlayer::Players[i]->Ai) {
					continue;
				}
				CPlayer::Players[i]->Ai->ScriptDebug = 0;
			}
		} else {
			int playerid;
			if (item && !strcmp(item, "self")) {
				if (!CPlayer::GetThisPlayer()) {
					continue;
				}
				playerid = CPlayer::GetThisPlayer()->get_index();
			} else {
				playerid = LuaToNumber(l, j + 1);
			}

			if (!CPlayer::Players[playerid]->AiEnabled || !CPlayer::Players[playerid]->Ai) {
				continue;
			}
			CPlayer::Players[playerid]->Ai->ScriptDebug = 1;
		}
	}
	return 0;
}

/**
**  Need a unit.
**
**  @param l  Lua state.
**
**  @return   Number of return values
*/
static int CclAiNeed(lua_State *l)
{
	LuaCheckArgs(l, 1);
	//Wyrmgus start
	if (lua_isnil(l, 1)) {
		lua_pushboolean(l, 0);
		return 1;
	}
	//Wyrmgus end
	
	InsertUnitTypeRequests(CclGetUnitType(l), 1);

	lua_pushboolean(l, 0);
	return 1;
}

/**
**  Set the number of units.
**
**  @param l  Lua state
**
**  @return   Number of return values
*/
static int CclAiSet(lua_State *l)
{
	LuaCheckArgs(l, 2);
	//Wyrmgus start
	if (lua_isnil(l, 1)) {
		lua_pushboolean(l, 0);
		return 1;
	}
	//Wyrmgus end
	lua_pushvalue(l, 1);
	wyrmgus::unit_type *type = CclGetUnitType(l);
	lua_pop(l, 1);

	AiRequestType *autt = FindInUnitTypeRequests(type);
	//Wyrmgus start
	int set_count = LuaToNumber(l, 2);
//	if (autt) {
	if (autt && set_count <= 0) {
		RemoveFromUnitTypeRequests(type);
	} else if (autt) {
	//Wyrmgus end
		autt->Count = LuaToNumber(l, 2);
		// FIXME: 0 should remove it.
	//Wyrmgus start
//	} else {
	} else if (set_count > 0) {
	//Wyrmgus end
		InsertUnitTypeRequests(type, LuaToNumber(l, 2));
	}
	lua_pushboolean(l, 0);
	return 1;
}

/**
**  Wait for a unit.
**
**  @param l  Lua State.
**
**  @return   Number of return values
*/
static int CclAiWait(lua_State *l)
{
	LuaCheckArgs(l, 1);

	if (lua_isnil(l, 1)) {
		lua_pushboolean(l, 0);
		return 1;
	}

	const char *ident = LuaToString(l, 1);
	if (!strncmp(ident, "unit", 4)) {
		const wyrmgus::unit_type *type = wyrmgus::unit_type::get(ident);
		const AiRequestType *autt = FindInUnitTypeRequests(type);
		if (!autt) {
			// Look if we have this unit-type.
			if (AiPlayer->Player->GetUnitTypeAiActiveCount(type)) {
				lua_pushboolean(l, 0);
				return 1;
			}

			// Look if we have equivalent unit-types.
			if (type->Slot < (int)AiHelpers.Equiv.size()) {
				for (size_t j = 0; j < AiHelpers.Equiv[type->Slot].size(); ++j) {
					if (AiPlayer->Player->GetUnitTypeAiActiveCount(AiHelpers.Equiv[type->Slot][j])) {
						lua_pushboolean(l, 0);
						return 1;
					}
				}
			}
			// Look if we have an upgrade-to request.
			if (FindInUpgradeToRequests(type)) {
				lua_pushboolean(l, 1);
				return 1;
			}
			lua_pushboolean(l, 1);
			return 1;
		}
		//
		// Add equivalent units
		//
		unsigned int n = AiPlayer->Player->GetUnitTypeAiActiveCount(type);
		if (type->Slot < (int)AiHelpers.Equiv.size()) {
			for (size_t j = 0; j < AiHelpers.Equiv[type->Slot].size(); ++j) {
				n += AiPlayer->Player->GetUnitTypeAiActiveCount(AiHelpers.Equiv[type->Slot][j]);
			}
		}
		// units available?
		if (n >= autt->Count) {
			lua_pushboolean(l, 0);
			return 1;
		}
	} else if (!strncmp(ident, "upgrade", 7)) {
		if (UpgradeIdentAllowed(*AiPlayer->Player, ident) == 'R') {
			lua_pushboolean(l, 0);
			return 1;
		}
	}
	//Wyrmgus end
	lua_pushboolean(l, 1);
	return 1;
}

/**
**  Define a force, a groups of units.
**
**  @param l  Lua state.
*/
static int CclAiForce(lua_State *l)
{
	bool resetForce = false;
	const int arg = lua_gettop(l);
	assert_throw(0 < arg && arg <= 3);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument");
	}
	if (arg == 3) {
		resetForce = LuaToBoolean(l, 3);
	}
	int force = LuaToNumber(l, 1);
	if (force < 0 || force >= AI_MAX_FORCE_INTERNAL) {
		LuaError(l, "Force out of range: %d" _C_ force);
	}
	AiForce &aiforce = AiPlayer->Force[AiPlayer->Force.getScriptForce(force)];
	if (resetForce) {
		aiforce.Reset(true);
	}
	AiForceRole role = aiforce.Role;
	aiforce.State = AiForceAttackingState::Waiting;
	aiforce.Role = role;

	int args = lua_rawlen(l, 2);
	for (int j = 0; j < args; ++j) {
		lua_rawgeti(l, 2, j + 1);
		const unit_type *type = CclGetUnitType(l);
		lua_pop(l, 1);
		++j;
		int count = LuaToNumber(l, 2, j + 1);

		if (!type) { // bulletproof
			continue;
		}

		// Use the equivalent unittype.
		type = wyrmgus::unit_type::get_all()[UnitTypeEquivs[type->Slot]];

		if (resetForce) {
			// Append it.
			AiUnitType newaiut;
			newaiut.Want = count;
			newaiut.Type = type;
			aiforce.UnitTypes.push_back(newaiut);
		} else {
			// Look if already in force.
			size_t i;
			for (i = 0; i < aiforce.UnitTypes.size(); ++i) {
				AiUnitType *aiut = &aiforce.UnitTypes[i];
				if (aiut->Type->Slot == type->Slot) { // found
					if (count) {
						aiut->Want = count;
					} else {
						aiforce.UnitTypes.erase(aiforce.UnitTypes.begin() + i);
					}
					break;
				}
			}
			// New type append it.
			if (i == aiforce.UnitTypes.size()) {
				AiUnitType newaiut;
				newaiut.Want = count;
				newaiut.Type = type;
				aiforce.UnitTypes.push_back(newaiut);
			}
		}

	}
	AiAssignFreeUnitsToForce(AiPlayer->Force.getScriptForce(force));
	lua_pushboolean(l, 0);
	return 1;
}

/**
**  Define the role of a force.
**
**  @param l  Lua state.
*/
static int CclAiForceRole(lua_State *l)
{
	LuaCheckArgs(l, 2);
	int force = LuaToNumber(l, 1);
	if (force < 0 || force >= AI_MAX_FORCE_INTERNAL) {
		LuaError(l, "Force %i out of range" _C_ force);
	}

	AiForce &aiforce = AiPlayer->Force[AiPlayer->Force.getScriptForce(force)];

	const char *flag = LuaToString(l, 2);
	if (!strcmp(flag, "attack")) {
		aiforce.Role = AiForceRole::Attack;
	} else if (!strcmp(flag, "defend")) {
		aiforce.Role = AiForceRole::Defend;
	} else {
		LuaError(l, "Unknown force role '%s'" _C_ flag);
	}
	lua_pushboolean(l, 0);
	return 1;
}

/**
**  Release force.
**
**  @param l  Lua state.
*/
static int CclAiReleaseForce(lua_State *l)
{
	LuaCheckArgs(l, 1);
	int force = LuaToNumber(l, 1);
	if (force < 0 || force >= AI_MAX_FORCE_INTERNAL) {
		LuaError(l, "Force out of range: %d" _C_ force);
	}
	for (int i = AI_MAX_FORCE_INTERNAL; i < AI_MAX_FORCES; ++i) {
		if (AiPlayer->Force[i].FormerForce != -1 && AiPlayer->Force[i].FormerForce == force) {
			AiPlayer->Force[i].Reset(false);
		}
	}

	lua_pushboolean(l, 0);
	return 1;
}

/**
**  Check if a force ready.
**
**  @param l  Lua state.
*/
static int CclAiCheckForce(lua_State *l)
{
	LuaCheckArgs(l, 1);
	int force = LuaToNumber(l, 1);
	if (force < 0 || force >= AI_MAX_FORCE_INTERNAL) {
		lua_pushfstring(l, "Force out of range: %d", force);
	}
	if (AiPlayer->Force[AiPlayer->Force.getScriptForce(force)].Completed) {
		lua_pushboolean(l, 1);
		return 1;
	}
	lua_pushboolean(l, 0);
	return 1;
}

/**
**  Wait for a force ready.
**
**  @param l  Lua state.
*/
static int CclAiWaitForce(lua_State *l)
{
	LuaCheckArgs(l, 1);
	int force = LuaToNumber(l, 1);
	if (force < 0 || force >= AI_MAX_FORCE_INTERNAL) {
		lua_pushfstring(l, "Force out of range: %d", force);
	}
	if (AiPlayer->Force[AiPlayer->Force.getScriptForce(force)].Completed) {
		lua_pushboolean(l, 0);
		return 1;
	}
#if 0
	// Debuging
	AiRemoveDeadUnitInForces();
	assert_throw(!AiPlayer->Force.getScriptForce(f).Completed);
#endif
	lua_pushboolean(l, 1);
	return 1;
}

/**
**  Attack with force.
**
**  @param l  Lua state.
*/
static int CclAiAttackWithForce(lua_State *l)
{
	LuaCheckArgs(l, 1);
	int force = LuaToNumber(l, 1);
	if (force < 0 || force >= AI_MAX_FORCE_INTERNAL) {
		LuaError(l, "Force out of range: %d" _C_ force);
	}
	AiAttackWithForce(force);
	lua_pushboolean(l, 0);
	return 1;
}

/**
**  Wait for a forces ready.
**
**  @param l  Lua state.
*/
static int CclAiWaitForces(lua_State *l)
{
	LuaCheckArgs(l, 1);
	if (!lua_istable(l, 1)) {
		LuaError(l, "incorrect argument");
	}
	const int args = lua_rawlen(l, 1);
	for (int i = 0; i < args; ++i) {
		const int force = LuaToNumber(l, 1, i + 1);

		if (force < 0 || force >= AI_MAX_FORCE_INTERNAL) {
			lua_pushfstring(l, "Force out of range: %d", force);
		}
		if (!AiPlayer->Force[AiPlayer->Force.getScriptForce(force)].Completed) {
			lua_pushboolean(l, 1);
			return 1;
		}
	}
	lua_pushboolean(l, 0);
	return 1;
}

/**
**  Attack with forces.
**
**  @param l  Lua state.
*/
static int CclAiAttackWithForces(lua_State *l)
{
	std::array<int, AI_MAX_FORCE_INTERNAL + 1> Forces{};

	LuaCheckArgs(l, 1);
	if (!lua_istable(l, 1)) {
		LuaError(l, "incorrect argument");
	}
	int args = lua_rawlen(l, 1);
	for (int i = 0; i < args; ++i) {
		const int force = LuaToNumber(l, 1, i + 1);

		if (force < 0 || force >= AI_MAX_FORCE_INTERNAL) {
			lua_pushfstring(l, "Force out of range: %d", force);
		}
		Forces[i] = AiPlayer->Force.getScriptForce(force);
	}
	Forces[args] = -1;
	AiAttackWithForces(Forces);
	lua_pushboolean(l, 0);
	return 1;
}

/**
**  Sleep n cycles.
**
**  @param l  Lua state.
*/
static int CclAiSleep(lua_State *l)
{
	LuaCheckArgs(l, 1);
	int i = LuaToNumber(l, 1);
	if (AiPlayer->SleepCycles || i == 0) {
		if (AiPlayer->SleepCycles < GameCycle) {
			AiPlayer->SleepCycles = 0;
			lua_pushboolean(l, 0);
			return 1;
		}
	} else {
		AiPlayer->SleepCycles = GameCycle + i;
	}
	lua_pushboolean(l, 1);
	return 1;
}

/**
**  Research an upgrade.
**
**  @param l  Lua state.
*/
static int CclAiResearch(lua_State *l)
{
	LuaCheckArgs(l, 1);
	//Wyrmgus start
	if (lua_isnil(l, 1)) {
		lua_pushboolean(l, 0);
		return 1;
	}
	//Wyrmgus end
	const char *str = LuaToString(l, 1);
	CUpgrade *upgrade;

	if (str) {
		upgrade = CUpgrade::get(str);
	} else {
		LuaError(l, "Upgrade needed");
		upgrade = nullptr;
	}

	//Wyrmgus start
//	InsertResearchRequests(upgrade);
	if (UpgradeIdentAllowed(*AiPlayer->Player, str) != 'R') {
		InsertResearchRequests(upgrade);
	}
	//Wyrmgus end

	lua_pushboolean(l, 0);
	return 1;
}

/**
**  Upgrade an unit to an new unit-type.
**
**  @param l  Lua state.
*/
static int CclAiUpgradeTo(lua_State *l)
{
	LuaCheckArgs(l, 1);
	//Wyrmgus start
	if (lua_isnil(l, 1)) {
		lua_pushboolean(l, 0);
		return 1;
	}
	//Wyrmgus end
	wyrmgus::unit_type *type = CclGetUnitType(l);
	InsertUpgradeToRequests(type);

	lua_pushboolean(l, 0);
	return 1;
}

/**
**  Return the player of the running AI.
**
**  @param l  Lua state.
**
**  @return  Player number of the AI.
*/
static int CclAiPlayer(lua_State *l)
{
	LuaCheckArgs(l, 0);
	lua_pushnumber(l, AiPlayer->Player->get_index());
	return 1;
}

/**
**  Set AI player resource reserve.
**
**  @param l  Lua state.
**
**  @return     Old resource vector
*/
static int CclAiSetReserve(lua_State *l)
{
	LuaCheckArgs(l, 1);
	if (!lua_istable(l, 1)) {
		LuaError(l, "incorrect argument");
	}
	lua_newtable(l);
	for (size_t i = 0; i < resource::get_all().size(); ++i) {
		lua_pushnumber(l, AiPlayer->get_reserve(resource::get_all()[i]));
		lua_rawseti(l, -2, i + 1);
	}
	for (size_t i = 0; i < wyrmgus::resource::get_all().size(); ++i) {
		AiPlayer->set_reserve(resource::get_all()[i], LuaToNumber(l, 1, i + 1));
	}
	return 1;
}

/**
**  Set AI player resource collect percent.
**
**  @param l  Lua state.
*/
static int CclAiSetCollect(lua_State *l)
{
	//Wyrmgus start
	/*
	LuaCheckArgs(l, 1);
	if (!lua_istable(l, 1)) {
		LuaError(l, "incorrect argument");
	}
	for (int i = 0; i < MaxCosts; ++i) {
		AiPlayer->Collect[i] = LuaToNumber(l, 1, i + 1);
	}
	*/
	const unsigned int args = lua_gettop(l);

	if (args & 1) {
		LuaError(l, "incorrect argument");
	}
	for (unsigned int j = 0; j < args; ++j) {
		const std::string resource_str = LuaToString(l, j + 1);
		const resource *resource = resource::get(resource_str);

		++j;
		AiPlayer->set_collect(resource, LuaToNumber(l, j + 1));
	}
	//Wyrmgus end
	return 0;
}

/**
**  Set AI player build.
**
**  @param l  Lua state.
*/
static int CclAiSetBuildDepots(lua_State *l)
{
	LuaCheckArgs(l, 1);
	if (!lua_isboolean(l, 1)) {
		LuaError(l, "incorrect argument");
	}
	AiPlayer->BuildDepots = LuaToBoolean(l, 1);
	return 0;
}

/**
**  Dump some AI debug information.
**
**  @param l  Lua state.
*/
static int CclAiDump(lua_State *l)
{
	LuaCheckArgs(l, 0);
	for (int p = 0; p < PlayerMax - 1; ++p) {
		CPlayer &aip = *CPlayer::Players[p];
		if (aip.AiEnabled) {
			// Script

			printf("------\n");
			for (const resource *resource : resource::get_all()) {
				printf("%s(%4d, %4d/%4d) ", resource->get_identifier().c_str(),
					   aip.get_resource(resource), aip.get_stored_resource(resource), aip.get_max_resource(resource));
			}
			printf("\n");
			printf("Player %d:", aip.get_index());
#if 0
			gh_display(gh_car(AiPlayer->Script));
#endif
			//
			// Requests
			//
			size_t n = aip.Ai->UnitTypeRequests.size();
			printf("UnitTypeRequests(%u):\n", static_cast<unsigned int>(n));
			for (size_t i = 0; i < n; ++i) {
				printf("%s ", aip.Ai->UnitTypeRequests[i].Type->get_identifier().c_str());
			}
			printf("\n");
			n = aip.Ai->UpgradeToRequests.size();
			printf("UpgradeToRequests(%u):\n", static_cast<unsigned int>(n));
			for (size_t i = 0; i < n; ++i) {
				printf("%s ", aip.Ai->UpgradeToRequests[i]->get_identifier().c_str());
			}
			printf("\n");

			printf("ResearchRequests(%u):\n", static_cast<unsigned int>(aip.Ai->get_research_requests().size()));
			for (const CUpgrade *requested_upgrade : aip.Ai->get_research_requests()) {
				printf("%s ", requested_upgrade->get_identifier().c_str());
			}
			printf("\n");

			// Building queue
			printf("Building queue:\n");
			for (size_t i = 0; i < aip.Ai->UnitTypeBuilt.size(); ++i) {
				const AiBuildQueue &queue = aip.Ai->UnitTypeBuilt[i];
				printf("%s(%d/%d) ", queue.Type->get_identifier().c_str(), queue.Made, queue.Want);
			}
			printf("\n");

			// PrintForce
			for (size_t i = 0; i < aip.Ai->Force.Size(); ++i) {
				printf("Force(%u%s%s):\n", static_cast<unsigned int>(i),
					   aip.Ai->Force[i].Completed ? ",complete" : ",recruit",
					   aip.Ai->Force[i].Attacking ? ",attack" : "");
				for (size_t j = 0; j < aip.Ai->Force[i].UnitTypes.size(); ++j) {
					const AiUnitType &aut = aip.Ai->Force[i].UnitTypes[j];
					printf("%s(%d) ", aut.Type->get_identifier().c_str(), aut.Want);
				}
				printf("\n");
			}
			printf("\n");
		}
	}
	lua_pushboolean(l, 0);
	return 1;
}

/**
**  Parse AiBuildQueue builing list
**
**  @param l     Lua state.
**  @param ai  PlayerAi pointer which should be filled with the data.
*/
static void CclParseBuildQueue(lua_State *l, PlayerAi *ai, int offset)
{
	if (!lua_istable(l, offset)) {
		LuaError(l, "incorrect argument");
	}

	Vec2i pos(-1, -1);
	//Wyrmgus start
	int z = -1;
	const landmass *landmass = nullptr;
	const site *settlement = nullptr;
	//Wyrmgus end

	const int args = lua_rawlen(l, offset);
	for (int k = 0; k < args; ++k) {
		const char *value = LuaToString(l, offset, k + 1);
		++k;

		if (!strcmp(value, "onpos")) {
			pos.x = LuaToNumber(l, offset, k + 1);
			++k;
			pos.y = LuaToNumber(l, offset, k + 1);
		//Wyrmgus start
		} else if (!strcmp(value, "map-layer")) {
			z = LuaToNumber(l, offset, k + 1);
		} else if (!strcmp(value, "landmass")) {
			const int landmass_index = LuaToNumber(l, offset, k + 1);
			landmass = CMap::get()->get_landmasses()[landmass_index].get();
		} else if (!strcmp(value, "settlement")) {
			settlement = site::get(LuaToString(l, offset, k + 1));
		//Wyrmgus end
		} else {
			//ident = LuaToString(l, j + 1, k + 1);
			//++k;
			const int made = LuaToNumber(l, offset, k + 1);
			++k;
			const int want = LuaToNumber(l, offset, k + 1);

			AiBuildQueue queue;
			queue.Type = wyrmgus::unit_type::get(value);
			queue.Want = want;
			queue.Made = made;
			queue.Pos = pos;
			//Wyrmgus start
			queue.MapLayer = z;
			queue.landmass = landmass;
			queue.settlement = settlement;
			//Wyrmgus end

			ai->UnitTypeBuilt.push_back(queue);
			pos.x = -1;
			pos.y = -1;
			//Wyrmgus start
			z = -1;
			landmass = nullptr;
			settlement = nullptr;
			//Wyrmgus end
		}
	}
}

/**
** Define an AI player.
**
**  @param l  Lua state.
*/
static int CclDefineAiPlayer(lua_State *l)
{
	const unsigned int playerIdx = LuaToNumber(l, 0 + 1);

	assert_throw(playerIdx <= PlayerMax);
	DebugPrint("%p %d\n" _C_(void *)CPlayer::Players[playerIdx]->Ai.get() _C_ CPlayer::Players[playerIdx]->AiEnabled);
	// FIXME: lose this:
	// assert_throw(!Players[playerIdx].Ai && Players[playerIdx].AiEnabled);

	CPlayer::Players[playerIdx]->Ai = std::make_unique<PlayerAi>();
	PlayerAi *ai = CPlayer::Players[playerIdx]->Ai.get();
	ai->Player = CPlayer::Players[playerIdx].get();

	// Parse the list: (still everything could be changed!)
	const int args = lua_gettop(l);
	for (int j = 1; j < args; ++j) {
		const char *value = LuaToString(l, j + 1);
		++j;

		if (!strcmp(value, "ai-type")) {
			const char *aiName = LuaToString(l, j + 1);
			CAiType *ait = GetAiTypesByName(aiName);
			if (ait == nullptr) {
				LuaError(l, "ai-type not found: %s" _C_ aiName);
			}
			ai->AiType = ait;
			ai->Script = ait->Script;
		} else if (!strcmp(value, "script")) {
			ai->Script = LuaToString(l, j + 1);
		} else if (!strcmp(value, "script-debug")) {
			ai->ScriptDebug = LuaToBoolean(l, j + 1);
		} else if (!strcmp(value, "sleep-cycles")) {
			ai->SleepCycles = LuaToNumber(l, j + 1);
		//Wyrmgus start
		} else if (!strcmp(value, "scouting")) {
			ai->Scouting = true;
			--j;
		//Wyrmgus end
		} else if (!strcmp(value, "force")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			const int cclforceIdx = LuaToNumber(l, j + 1, 1);
			UNUSED(cclforceIdx);
			const int forceIdx = ai->Force.FindFreeForce(AiForceRole::Default);

			for (int k = 1; k < subargs; ++k) {
				const char *secondary_value = LuaToString(l, j + 1, k + 1);
				++k;
				if (!strcmp(secondary_value, "complete")) {
					ai->Force[forceIdx].Completed = true;
					--k;
				} else if (!strcmp(secondary_value, "recruit")) {
					ai->Force[forceIdx].Completed = false;
					--k;
				} else if (!strcmp(secondary_value, "attack")) {
					ai->Force[forceIdx].Attacking = true;
					--k;
				} else if (!strcmp(secondary_value, "defend")) {
					ai->Force[forceIdx].Defending = true;
					--k;
				} else if (!strcmp(secondary_value, "role")) {
					secondary_value = LuaToString(l, j + 1, k + 1);
					if (!strcmp(secondary_value, "attack")) {
						ai->Force[forceIdx].Role = AiForceRole::Attack;
					} else if (!strcmp(secondary_value, "defend")) {
						ai->Force[forceIdx].Role = AiForceRole::Defend;
					} else {
						LuaError(l, "Unsupported force tag: %s" _C_ secondary_value);
					}
				} else if (!strcmp(secondary_value, "types")) {
					lua_rawgeti(l, j + 1, k + 1);
					if (!lua_istable(l, -1)) {
						LuaError(l, "incorrect argument");
					}
					const int subsubargs = lua_rawlen(l, -1);
					for (int subk = 0; subk < subsubargs; ++subk) {
						const int num = LuaToNumber(l, -1, subk + 1);
						++subk;
						const char *ident = LuaToString(l, -1, subk + 1);
						AiUnitType queue;

						queue.Want = num;
						queue.Type = wyrmgus::unit_type::get(ident);
						ai->Force[forceIdx].UnitTypes.push_back(queue);
					}
					lua_pop(l, 1);
				} else if (!strcmp(secondary_value, "units")) {
					lua_rawgeti(l, j + 1, k + 1);
					if (!lua_istable(l, -1)) {
						LuaError(l, "incorrect argument");
					}
					const int subsubargs = lua_rawlen(l, -1);
					for (int subk = 0; subk < subsubargs; ++subk) {
						const int num = LuaToNumber(l, -1, subk + 1);
						++subk;
#if 0
						const char *ident = LuaToString(l, -1, subk + 1);
						UNUSED(ident);
#endif
						ai->Force[forceIdx].Insert(&wyrmgus::unit_manager::get()->GetSlotUnit(num));
					}
					lua_pop(l, 1);
				} else if (!strcmp(secondary_value, "state")) {
					ai->Force[forceIdx].State = AiForceAttackingState(LuaToNumber(l, j + 1, k + 1));
				} else if (!strcmp(secondary_value, "goalx")) {
					ai->Force[forceIdx].GoalPos.x = LuaToNumber(l, j + 1, k + 1);
				} else if (!strcmp(secondary_value, "goaly")) {
					ai->Force[forceIdx].GoalPos.y = LuaToNumber(l, j + 1, k + 1);
				} else if (!strcmp(secondary_value, "must-transport")) {
					// Keep for backward compatibility
				} else {
					LuaError(l, "Unsupported tag: %s" _C_ secondary_value);
				}
			}
		} else if (!strcmp(value, "reserve")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				const char *type = LuaToString(l, j + 1, k + 1);
				++k;
				int num = LuaToNumber(l, j + 1, k + 1);
				const resource *res = resource::get(type);
				ai->set_reserve(res, num);
			}
		} else if (!strcmp(value, "used")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				const char *type = LuaToString(l, j + 1, k + 1);
				++k;
				const int num = LuaToNumber(l, j + 1, k + 1);
				const resource *res = resource::get(type);
				ai->Used[res] = num;
			}
		} else if (!strcmp(value, "needed")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				const char *type = LuaToString(l, j + 1, k + 1);
				++k;
				const int num = LuaToNumber(l, j + 1, k + 1);
				const resource *res = resource::get(type);
				ai->Needed[res] = num;
			}
		} else if (!strcmp(value, "collect")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				const char *type = LuaToString(l, j + 1, k + 1);
				++k;
				const int num = LuaToNumber(l, j + 1, k + 1);
				const resource *res = resource::get(type);
				ai->set_collect(res, num);
			}
		} else if (!strcmp(value, "need-mask")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				const char *type = LuaToString(l, j + 1, k + 1);
				const int resId = GetResourceIdByName(l, type);
				ai->NeededMask |= ((long long int) 1 << resId);
			}
		} else if (!strcmp(value, "need-supply")) {
			ai->NeedSupply = true;
			--j;
		} else if (!strcmp(value, "exploration")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				Vec2i pos;

				lua_rawgeti(l, j + 1, k + 1);
				if (!lua_istable(l, -1) || lua_rawlen(l, -1) != 3) {
					LuaError(l, "incorrect argument");
				}
				pos.x = LuaToNumber(l, -1, 1);
				pos.y = LuaToNumber(l, -1, 2);
				const tile_flag mask = static_cast<tile_flag>(LuaToUnsignedNumber(l, -1, 3));
				lua_pop(l, 1);
				AiExplorationRequest queue(pos, mask);
				ai->FirstExplorationRequest.push_back(queue);
			}
		} else if (!strcmp(value, "last-exploration-cycle")) {
			ai->LastExplorationGameCycle = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "last-can-not-move-cycle")) {
			ai->LastCanNotMoveGameCycle = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "unit-type")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			int i = 0;
			if (subargs) {
				//Wyrmgus start
//				ai->UnitTypeRequests.resize(subargs / 2);
				ai->UnitTypeRequests.resize(subargs / 3);
				//Wyrmgus end
			}
			for (int k = 0; k < subargs; ++k) {
				const char *ident = LuaToString(l, j + 1, k + 1);
				++k;
				const int count = LuaToNumber(l, j + 1, k + 1);
				++k;
				const int landmass_index = LuaToNumber(l, j + 1, k + 1);
				ai->UnitTypeRequests[i].Type = wyrmgus::unit_type::get(ident);
				ai->UnitTypeRequests[i].Count = count;
				if (landmass_index != -1) {
					ai->UnitTypeRequests[i].landmass = CMap::get()->get_landmasses()[landmass_index].get();
				}
				++i;
			}
		} else if (!strcmp(value, "upgrade")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				const char *ident = LuaToString(l, j + 1, k + 1);
				ai->UpgradeToRequests.push_back(wyrmgus::unit_type::get(ident));
			}
		} else if (!strcmp(value, "research")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				const char *ident = LuaToString(l, j + 1, k + 1);
				ai->add_research_request(CUpgrade::get(ident));
			}
		} else if (!strcmp(value, "building")) {
			CclParseBuildQueue(l, ai, j + 1);
		} else if (!strcmp(value, "repair-building")) {
			ai->LastRepairBuilding = LuaToNumber(l, j + 1);
		//Wyrmgus start
		} else if (!strcmp(value, "pathway-construction-building")) {
			ai->LastPathwayConstructionBuilding = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "scouts")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				const int num = LuaToNumber(l, j + 1, k + 1);
				++k;
				ai->Scouts.push_back(&unit_manager::get()->GetSlotUnit(num));
			}
			//Wyrmgus end
		} else if (!strcmp(value, "transporters")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				const int landmass_index = LuaToNumber(l, j + 1, k + 1);
				const landmass *landmass = nullptr;
				if (landmass_index != -1) {
					landmass = CMap::get()->get_landmasses()[landmass_index].get();
				}
				++k;
				const int num = LuaToNumber(l, j + 1, k + 1);
				ai->add_transporter(&unit_manager::get()->GetSlotUnit(num), landmass);
			}
		} else if (!strcmp(value, "site-transport-units")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				const std::string site_identifier = LuaToString(l, j + 1, k + 1);
				const site *site = site::get(site_identifier);
				++k;
				const int num = LuaToNumber(l, j + 1, k + 1);
				ai->add_site_transport_unit(&unit_manager::get()->GetSlotUnit(num), site);
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	return 0;
}

/**
**  Register CCL features for unit-type.
*/
void AiCclRegister()
{
	// FIXME: Need to save memory here.
	// Loading all into memory isn't necessary.

	lua_register(Lua, "DefineAiHelper", CclDefineAiHelper);
	lua_register(Lua, "DefineAi", CclDefineAi);

	lua_register(Lua, "AiGetRace", CclAiGetRace);
	lua_register(Lua, "AiGetSleepCycles", CclAiGetSleepCycles);

	lua_register(Lua, "AiDebug", CclAiDebug);
	lua_register(Lua, "AiDebugPlayer", CclAiDebugPlayer);
	lua_register(Lua, "AiNeed", CclAiNeed);
	lua_register(Lua, "AiSet", CclAiSet);
	lua_register(Lua, "AiWait", CclAiWait);

	lua_register(Lua, "AiForce", CclAiForce);

	lua_register(Lua, "AiReleaseForce", CclAiReleaseForce);
	lua_register(Lua, "AiForceRole", CclAiForceRole);
	lua_register(Lua, "AiCheckForce", CclAiCheckForce);
	lua_register(Lua, "AiWaitForce", CclAiWaitForce);

	lua_register(Lua, "AiAttackWithForce", CclAiAttackWithForce);
	lua_register(Lua, "AiSleep", CclAiSleep);
	lua_register(Lua, "AiResearch", CclAiResearch);
	lua_register(Lua, "AiUpgradeTo", CclAiUpgradeTo);

	lua_register(Lua, "AiPlayer", CclAiPlayer);
	lua_register(Lua, "AiSetReserve", CclAiSetReserve);
	lua_register(Lua, "AiSetCollect", CclAiSetCollect);

	lua_register(Lua, "AiSetBuildDepots", CclAiSetBuildDepots);
	
	lua_register(Lua, "AiDump", CclAiDump);

	lua_register(Lua, "DefineAiPlayer", CclDefineAiPlayer);
	lua_register(Lua, "AiAttackWithForces", CclAiAttackWithForces);
	lua_register(Lua, "AiWaitForces", CclAiWaitForces);
}
