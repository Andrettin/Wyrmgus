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
//      (c) Copyright 2002-2022 by Lutz Sammer, Jimmy Salmon and Andrettin
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

#include "script/trigger.h"

#include "config.h"
#include "engine_interface.h"
#include "game/game.h"
#include "iolib.h"
//Wyrmgus start
#include "luacallback.h"
//Wyrmgus end
#include "map/map.h"
#include "map/map_info.h"
#include "player/player.h"
#include "player/player_type.h"
#include "quest/campaign.h"
//Wyrmgus start
#include "quest/quest.h" // for saving quests
//Wyrmgus end
#include "results.h"
#include "script.h"
#include "script/condition/and_condition.h"
#include "script/context.h"
#include "script/effect/effect.h"
#include "script/effect/effect_list.h"
#include "script/trigger_target.h"
#include "script/trigger_type.h"
#include "ui/interface.h"
#include "unit/unit.h"
#include "unit/unit_find.h"
#include "unit/unit_type.h"
#include "util/random.h"
#include "util/vector_random_util.h"
#include "util/vector_util.h"

/// Some data accessible for script during the game.
TriggerDataType TriggerData;

namespace wyrmgus {

std::vector<std::string> trigger::DeactivatedTriggers;
unsigned int trigger::CurrentTriggerId = 0;

}

/**
**  Get player number.
**
**  @param l  Lua state.
**
**  @return   The player number, -1 matches any.
*/
int TriggerGetPlayer(lua_State *l)
{
	if (lua_isnumber(l, -1)) {
		const int ret = LuaToNumber(l, -1);
		if (ret < 0 || ret > PlayerMax) {
			LuaError(l, "bad player: %d" _C_ ret);
		}
		return ret;
	}
	const char *player = LuaToString(l, -1);
	if (!strcmp(player, "any")) {
		return -1;
	} else if (!strcmp(player, "this")) {
		return CPlayer::GetThisPlayer()->get_index();
	}
	LuaError(l, "bad player: %s" _C_ player);
	return 0;
}

/**
**  Get the unit-type.
**
**  @param l  Lua state.
**
**  @return   The unit-type pointer.
*/
const wyrmgus::unit_type *TriggerGetUnitType(lua_State *l)
{
	const char *unit = LuaToString(l, -1);

	if (!strcmp(unit, "any")) {
		return ANY_UNIT;
	} else if (!strcmp(unit, "units")) {
		return ALL_FOODUNITS;
	} else if (!strcmp(unit, "buildings")) {
		return ALL_BUILDINGS;
	}
	return CclGetUnitType(l);
}

/*--------------------------------------------------------------------------
--  Conditions
--------------------------------------------------------------------------*/
static int CompareEq(int a, int b) { return a == b; }
static int CompareNEq(int a, int b) { return a != b; }
static int CompareGrEq(int a, int b) { return a >= b; }
static int CompareGr(int a, int b) { return a > b; }
static int CompareLeEq(int a, int b) { return a <= b; }
static int CompareLe(int a, int b) { return a < b; }

typedef int (*CompareFunction)(int, int);

/**
**  Returns a function pointer to the comparison function
**
**  @param op  The operation
**
**  @return    Function pointer to the compare function
*/
static CompareFunction GetCompareFunction(const char *op)
{
	if (op[0] == '=') {
		if ((op[1] == '=' && op[2] == '\0') || (op[1] == '\0')) {
			return &CompareEq;
		}
	} else if (op[0] == '>') {
		if (op[1] == '=' && op[2] == '\0') {
			return &CompareGrEq;
		} else if (op[1] == '\0') {
			return &CompareGr;
		}
	} else if (op[0] == '<') {
		if (op[1] == '=' && op[2] == '\0') {
			return &CompareLeEq;
		} else if (op[1] == '\0') {
			return &CompareLe;
		}
	} else if (op[0] == '!' && op[1] == '=' && op[2] == '\0') {
		return &CompareNEq;
	}
	return nullptr;
}

/**
**  Return the number of units of a given unit-type and player at a location.
*/
static int CclGetNumUnitsAt(lua_State *l)
{
	const int nargs = lua_gettop(l);
	if (nargs < 4 || nargs > 5) {
		LuaError(l, "incorrect argument\n");
	}

	int plynr = LuaToNumber(l, 1);
	lua_pushvalue(l, 2);
	const wyrmgus::unit_type *unittype = TriggerGetUnitType(l);
	lua_pop(l, 1);

	Vec2i minPos;
	Vec2i maxPos;
	CclGetPos(l, &minPos.x, &minPos.y, 3);
	CclGetPos(l, &maxPos.x, &maxPos.y, 4);
	
	minPos.x = std::max<int>(minPos.x, 0);
	minPos.y = std::max<int>(minPos.y, 0);

	std::vector<CUnit *> units;

	int z = 0;
	if (nargs >= 5) {
		z = LuaToNumber(l, 5);
	}
	
	if (z == 0 && (CMap::get()->Info->MapWidths.size() == 0 || CMap::get()->Info->MapHeights.size() == 0)) {
		maxPos.x = std::min<int>(maxPos.x, CMap::get()->Info->get_map_width() - 1);
		maxPos.y = std::min<int>(maxPos.y, CMap::get()->Info->get_map_height() - 1);
	} else if (z != -1) {
		maxPos.x = std::min<int>(maxPos.x, CMap::get()->Info->MapWidths[z] - 1);
		maxPos.y = std::min<int>(maxPos.y, CMap::get()->Info->MapHeights[z] - 1);
	}
	
	if (z == -1 || !CMap::get()->Info->IsPointOnMap(minPos, z) || !CMap::get()->Info->IsPointOnMap(maxPos, z)) {
		lua_pushnumber(l, 0);
		return 1;
	}
	
	Select(minPos, maxPos, units, z);

	int s = 0;
	for (size_t i = 0; i != units.size(); ++i) {
		const CUnit &unit = *units[i];
		// Check unit type
		
		//Wyrmgus start
		if (unit.Type->BoolFlag[REVEALER_INDEX].value) {
			continue;
		}
		//Wyrmgus end

		if (unittype == ANY_UNIT
			|| (unittype == ALL_FOODUNITS && !unit.Type->BoolFlag[BUILDING_INDEX].value)
			|| (unittype == ALL_BUILDINGS && unit.Type->BoolFlag[BUILDING_INDEX].value)
			|| (unittype == unit.Type && !unit.UnderConstruction)) {

			// Check the player
			//Wyrmgus start
//			if (plynr == -1 || plynr == unit.Player->get_index()) {
			if (plynr == -1 || plynr == unit.Player->get_index() || (plynr == -2 && unit.Player->get_type() != player_type::neutral)) { // -2 can be used for the player field to mean any non-neutral player
			//Wyrmgus end
				if (unit.IsAlive()) {
					++s;
				}
			}
		}
	}
	lua_pushnumber(l, s);
	return 1;
}

/**
**  Player has the quantity of unit-type near to unit-type.
*/
static int CclIfNearUnit(lua_State *l)
{
	const int nargs = lua_gettop(l);
	if (nargs < 5 || nargs > 6) {
		LuaError(l, "incorrect argument\n");
	}
	
	lua_pushvalue(l, 1);
	const int plynr = TriggerGetPlayer(l);
	lua_pop(l, 1);
	const char *op = LuaToString(l, 2);
	const int q = LuaToNumber(l, 3);
	lua_pushvalue(l, 4);
	const wyrmgus::unit_type *unittype = TriggerGetUnitType(l);
	lua_pop(l, 1);
	lua_pushvalue(l, 5);
	const wyrmgus::unit_type *ut2 = CclGetUnitType(l);
	lua_pop(l, 1);
	if (!unittype || !ut2) {
		LuaError(l, "CclIfNearUnit: not a unit-type valid");
	}
	CompareFunction compare = GetCompareFunction(op);
	if (!compare) {
		LuaError(l, "Illegal comparison operation in if-near-unit: %s" _C_ op);
	}

	int other_plynr = plynr;
	if (nargs >= 6) {
		lua_pushvalue(l, 6);
		other_plynr = TriggerGetPlayer(l);
		lua_pop(l, 1);
	}
	
	// Get all unit types 'near'.
	
	std::vector<CUnit *> unitsOfType;

	FindUnitsByType(*ut2, unitsOfType);
	for (size_t i = 0; i != unitsOfType.size(); ++i) {
		const CUnit &centerUnit = *unitsOfType[i];

		if (other_plynr != -1 && other_plynr != centerUnit.Player->get_index()) {
			continue;
		}
				
		std::vector<CUnit *> around;
		SelectAroundUnit(centerUnit, 1, around);

		// Count the requested units
		int s = 0;
		for (size_t j = 0; j < around.size(); ++j) {
			const CUnit &unit = *around[j];

			// Check unit type
			if (unittype == ANY_UNIT
				|| (unittype == ALL_FOODUNITS && !unit.Type->BoolFlag[BUILDING_INDEX].value)
				|| (unittype == ALL_BUILDINGS && unit.Type->BoolFlag[BUILDING_INDEX].value)
				|| (unittype == unit.Type)) {

				// Check the player
				if (plynr == -1 || plynr == unit.Player->get_index()) {
					++s;
				}
			}
		}
		if (compare(s, q)) {
			lua_pushboolean(l, 1);
			return 1;
		}
	}
	lua_pushboolean(l, 0);
	return 1;
}

/**
**  Player has the quantity of rescued unit-type near to unit-type.
*/
static int CclIfRescuedNearUnit(lua_State *l)
{
	LuaCheckArgs(l, 5);

	lua_pushvalue(l, 1);
	const int plynr = TriggerGetPlayer(l);
	lua_pop(l, 1);
	const char *op = LuaToString(l, 2);
	const int q = LuaToNumber(l, 3);
	lua_pushvalue(l, 4);
	const wyrmgus::unit_type *unittype = TriggerGetUnitType(l);
	lua_pop(l, 1);
	const wyrmgus::unit_type *ut2 = CclGetUnitType(l);
	if (!unittype || !ut2) {
		LuaError(l, "CclIfRescuedNearUnit: not a unit-type valid");
	}

	CompareFunction compare = GetCompareFunction(op);
	if (!compare) {
		LuaError(l, "Illegal comparison operation in if-rescued-near-unit: %s" _C_ op);
	}

	// Get all unit types 'near'.
	std::vector<CUnit *> table;
	FindUnitsByType(*ut2, table);
	for (size_t i = 0; i != table.size(); ++i) {
		CUnit &centerUnit = *table[i];
		std::vector<CUnit *> around;

		SelectAroundUnit(centerUnit, 1, around);
		// Count the requested units
		int s = 0;
		for (size_t j = 0; j != around.size(); ++j) {
			CUnit &unit = *around[j];

			if (unit.is_rescued()) { //only rescued units
				// Check unit type

				if (unittype == ANY_UNIT
					|| (unittype == ALL_FOODUNITS && !unit.Type->BoolFlag[BUILDING_INDEX].value)
					|| (unittype == ALL_BUILDINGS && unit.Type->BoolFlag[BUILDING_INDEX].value)
					|| (unittype == unit.Type)) {

					// Check the player
					if (plynr == -1 || plynr == unit.Player->get_index()) {
						++s;
					}
				}
			}
		}
		if (compare(s, q)) {
			lua_pushboolean(l, 1);
			return 1;
		}
	}
	lua_pushboolean(l, 0);
	return 1;
}

/**
**  Returns the number of opponents of a given player.
*/
int GetNumOpponents(int player)
{
	int n = 0;

	// Check the player opponents
	for (int i = 0; i < PlayerMax; ++i) {
		const int unitCount = CPlayer::Players[i]->GetUnitCount();

		// This player is our enemy and has units left.
		if ((CPlayer::Players[player]->is_enemy_of(*CPlayer::Players[i])) || (CPlayer::Players[i]->is_enemy_of(*CPlayer::Players[player]))) {
			// Don't count walls
			for (int j = 0; j < unitCount; ++j) {
				if (CPlayer::Players[i]->GetUnit(j).Type->BoolFlag[WALL_INDEX].value == false) {
					++n;
					break;
				}
			}
		}
	}
	return n;
}

/*---------------------------------------------------------------------------
-- Actions
---------------------------------------------------------------------------*/

/**
**  Stop the running game with a given result
*/
void StopGame(GameResults result)
{
	if (result == GameRestart) {
		engine_interface::get()->set_loading_message("Restarting Game...");
	}

	GameResult = result;
	game::get()->set_paused(true);
	GameRunning = false;
	game::get()->store_results();

	//update the display before setting the game to stopped, to prevent the map still being shown while the UI updates
	UpdateDisplay();
	game::get()->set_running(false);
}

/**
**  Action condition player wins.
*/
void ActionVictory()
{
	StopGame(GameVictory);
}

/**
**  Action condition player lose.
*/
void ActionDefeat()
{
	StopGame(GameDefeat);
}

/**
**  Action condition player draw.
*/
void ActionDraw()
{
	StopGame(GameDraw);
}

/**
**  Add a trigger.
*/
int CclAddTrigger(lua_State *l)
{
	LuaCheckArgs(l, 3);
	
	if (!lua_isfunction(l, 2) || !lua_isfunction(l, 3)) {
		LuaError(l, "incorrect argument");
	}

	const std::string trigger_ident = LuaToString(l, 1);
	
	if (wyrmgus::vector::contains(wyrmgus::trigger::DeactivatedTriggers, trigger_ident)) {
		return 0;
	}
	
	//this function only adds temporary triggers, that is, ones that will only last for the current game
	
	auto trigger = std::make_unique<wyrmgus::trigger>(trigger_ident);
	trigger->Local = true;
	trigger::active_triggers[trigger->get_type()].push_back(trigger.get());
	
	trigger->Conditions = std::make_unique<LuaCallback>(l, 2);
	trigger->Effects = std::make_unique<LuaCallback>(l, 3);
	
	if (trigger->Conditions == nullptr || trigger->Effects == nullptr) {
		fprintf(stderr, "Trigger \"%s\" has no conditions or no effects.\n", trigger->get_identifier().c_str());
	}

	trigger->set_campaign_only(false);

	wyrmgus::game::get()->add_local_trigger(std::move(trigger));

	return 0;
}

/**
**  Set the trigger values
*/
void SetCurrentTriggerId(unsigned int trigger_id)
{
	wyrmgus::trigger::CurrentTriggerId = trigger_id;
}

/**
**  Set the deactivated triggers
*/
static int CclSetDeactivatedTriggers(lua_State *l)
{
	const int args = lua_gettop(l);

	for (int j = 0; j < args; ++j) {
		wyrmgus::trigger::DeactivatedTriggers.push_back(LuaToString(l, j + 1));
	}
	return 0;
}

/**
**  Check trigger each game cycle.
*/
void TriggersEachCycle()
{
	try {
		std::vector<trigger *> &active_triggers = trigger::active_triggers[trigger_type::default_trigger];

		if (trigger::CurrentTriggerId >= active_triggers.size()) {
			trigger::CurrentTriggerId = 0;
		}

		if (game::get()->is_paused()) {
			return;
		}

		game::get()->process_delayed_effects();

		// go to the next trigger
		if (trigger::CurrentTriggerId < active_triggers.size()) {
			trigger *current_trigger = active_triggers[trigger::CurrentTriggerId];

			bool removed_trigger = false;

			//old Lua conditions/effects for triggers
			if (current_trigger->Conditions != nullptr && current_trigger->Effects != nullptr) {
				try {
					current_trigger->Conditions->pushPreamble();
					current_trigger->Conditions->run(1);
					if (current_trigger->Conditions->popBoolean()) {
						current_trigger->Effects->pushPreamble();
						current_trigger->Effects->run(1);
						if (current_trigger->Effects->popBoolean() == false) {
							trigger::DeactivatedTriggers.push_back(current_trigger->get_identifier());
							active_triggers.erase(active_triggers.begin() + trigger::CurrentTriggerId);
							removed_trigger = true;
							if (current_trigger->Local) {
								game::get()->remove_local_trigger(current_trigger);
							}
						}
					}
				} catch (...) {
					std::throw_with_nested(std::runtime_error("Lua error for trigger \"" + current_trigger->get_identifier() + "\"."));
				}
			}

			if (!removed_trigger && current_trigger->get_effects() != nullptr) {
				std::vector<CPlayer *> potential_target_players;
				bool triggered = false;

				switch (current_trigger->get_target()) {
					case trigger_target::player:
						for (CPlayer *player : CPlayer::get_non_neutral_players()) {
							if (player->get_type() == player_type::nobody) {
								continue;
							}

							if (!player->is_alive()) {
								continue;
							}

							potential_target_players.push_back(player);
						}
						break;
					case trigger_target::neutral_player:
						potential_target_players.push_back(CPlayer::get_neutral_player());
						break;
					case trigger_target::player_or_neutral_player:
						for (const qunique_ptr<CPlayer> &player : CPlayer::Players) {
							if (player->get_type() == player_type::nobody) {
								continue;
							}

							if (!player->is_alive()) {
								continue;
							}

							potential_target_players.push_back(player.get());
						}
						break;
				}

				for (CPlayer *player : potential_target_players) {
					triggered = current_trigger->check_for_player(player);
					if (triggered && current_trigger->fires_only_once()) {
						break;
					}
				}

				if (triggered && current_trigger->fires_only_once()) {
					removed_trigger = true;
					if (current_trigger->Local) {
						game::get()->remove_local_trigger(current_trigger);
					}
				}
			}

			if (!removed_trigger) {
				trigger::CurrentTriggerId++;
			}
		} else {
			trigger::CurrentTriggerId = 0;
		}

		if (GameCycle % CYCLES_PER_HALF_MINUTE == 0) {
			trigger::half_minute_pulse_random_offset = random::get()->generate(CYCLES_PER_HALF_MINUTE);
		}
		if ((GameCycle + trigger::half_minute_pulse_random_offset) % CYCLES_PER_HALF_MINUTE == 0) {
			trigger::check_triggers(trigger_type::half_minute_pulse);
		}

		if (GameCycle % CYCLES_PER_MINUTE == 0) {
			trigger::minute_pulse_random_offset = random::get()->generate(CYCLES_PER_MINUTE);
		}
		if ((GameCycle + trigger::minute_pulse_random_offset) % CYCLES_PER_MINUTE == 0) {
			trigger::check_triggers(trigger_type::minute_pulse);
		}
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Error executing the per cycle actions for triggers."));
	}
}

namespace wyrmgus {

void trigger::clear()
{
	ClearActiveTriggers();
	data_type::clear();
}

/**
**	Initialize the trigger module.
*/
void trigger::InitActiveTriggers()
{
	// Setup default triggers

	// FIXME: choose the triggers for game type

	lua_getglobal(Lua, "_triggers_");
	if (lua_isnil(Lua, -1)) {
		lua_getglobal(Lua, "SinglePlayerTriggers");
		LuaCall(0, 1);
	}
	lua_pop(Lua, 1);
	
	for (trigger *trigger : trigger::get_all()) {
		if (vector::contains(trigger::DeactivatedTriggers, trigger->get_identifier())) {
			continue;
		}

		if (trigger->is_campaign_only() && game::get()->get_current_campaign() == nullptr) {
			continue;
		}

		if (game::get()->get_current_campaign() != nullptr && trigger->fires_only_once()) {
			if (trigger->get_historical_date().isValid() && trigger->get_historical_date() <= game::get()->get_current_campaign()->get_start_date()) {
				trigger::DeactivatedTriggers.push_back(trigger->get_identifier());
				continue;
			}
		}

		if (trigger->is_random()) {
			std::vector<const wyrmgus::trigger *> &active_random_triggers = trigger::active_random_triggers[trigger->get_type()];

			for (int i = 0; i < trigger->get_random_weight(); ++i) {
				active_random_triggers.push_back(trigger);
			}
		} else {
			trigger::active_triggers[trigger->get_type()].push_back(trigger);
		}
	}

	trigger::half_minute_pulse_random_offset = random::get()->generate(CYCLES_PER_HALF_MINUTE);
	trigger::minute_pulse_random_offset = random::get()->generate(CYCLES_PER_MINUTE);
}

void trigger::ClearActiveTriggers()
{
	lua_pushnil(Lua);
	lua_setglobal(Lua, "_triggers_");

	lua_pushnil(Lua);
	lua_setglobal(Lua, "Triggers");

	trigger::CurrentTriggerId = 0;

	game::get()->clear_local_triggers();
	trigger::active_triggers.clear();
	trigger::active_random_triggers.clear();
	trigger::DeactivatedTriggers.clear();
	
	//Wyrmgus start
	for (quest *quest : quest::get_all()) {
		quest->CurrentCompleted = false;
	}
	//Wyrmgus end
}

void trigger::check_triggers(const trigger_type type)
{
	for (const qunique_ptr<CPlayer> &player : CPlayer::Players) {
		if (player->get_type() == player_type::nobody) {
			continue;
		}

		if (!player->is_alive()) {
			continue;
		}

		trigger::check_triggers_for_player(player.get(), type);
	}
}

void trigger::check_triggers_for_player(CPlayer *player, const trigger_type type)
{
	std::vector<trigger *> triggers = trigger::active_triggers[type];
	for (const trigger *trigger : triggers) {
		trigger->check_for_player(player);
	}

	//trigger one out of the random triggers for this pulse, for each player
	std::vector<const trigger *> random_triggers = trigger::active_random_triggers[type];

	while (!random_triggers.empty()) {
		const trigger *trigger = vector::get_random(random_triggers);

		if (trigger->check_for_player(player)) {
			break;
		}

		std::erase(random_triggers, trigger);
	}
}

trigger::trigger(const std::string &identifier)
	: data_entry(identifier), type(trigger_type::default_trigger), target(trigger_target::neutral_player)
{
}

trigger::~trigger()
{
}

void trigger::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "effects") {
		this->effects = std::make_unique<effect_list<CPlayer>>();
		database::process_gsml_data(this->effects, scope);
	} else if (tag == "conditions") {
		this->conditions = std::make_unique<and_condition<CPlayer>>();
		database::process_gsml_data(this->conditions, scope);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void trigger::check() const
{
	if (this->get_preconditions() != nullptr) {
		this->get_preconditions()->check_validity();
	}

	if (this->get_conditions() != nullptr) {
		this->get_conditions()->check_validity();
	}

	if (this->get_effects() != nullptr) {
		this->get_effects()->check();
	}
}

void trigger::add_condition(std::unique_ptr<condition<CPlayer>> &&condition)
{
	if (this->conditions == nullptr) {
		this->conditions = std::make_unique<and_condition<CPlayer>>();
	}

	this->conditions->add_condition(std::move(condition));
}

void trigger::add_effect(std::unique_ptr<effect<CPlayer>> &&effect)
{
	if (this->effects == nullptr) {
		this->effects = std::make_unique<effect_list<CPlayer>>();
	}

	this->effects->add_effect(std::move(effect));
}

bool trigger::is_player_valid_target(const CPlayer *player) const
{
	switch (this->get_target()) {
		case trigger_target::player:
			return !player->is_neutral_player();
		case trigger_target::neutral_player:
			return player->is_neutral_player();
		case trigger_target::player_or_neutral_player:
			return true;
		default:
			assert_throw(false);
	}

	return false;
}

bool trigger::check_for_player(CPlayer *player) const
{
	if (!this->is_player_valid_target(player)) {
		return false;
	}

	if (!check_conditions(this, player)) {
		return false;
	}

	context ctx;
	ctx.current_player = player;
	this->get_effects()->do_effects(player, ctx);

	if (this->fires_only_once()) {
		trigger::DeactivatedTriggers.push_back(this->get_identifier());

		if (this->is_random()) {
			std::vector<trigger *> &active_triggers = trigger::active_triggers[this->get_type()];
			std::erase(active_triggers, this);
		} else {
			std::vector<const trigger *> &active_triggers = trigger::active_random_triggers[this->get_type()];
			std::erase(active_triggers, this);
		}
	}

	return true;
}

}

void call_trigger(const std::string &identifier)
{
	const trigger *trigger = trigger::try_get(identifier);
	if (trigger != nullptr) {
		if (trigger->get_effects() != nullptr) {
			context ctx;
			ctx.current_player = CPlayer::GetThisPlayer();
			trigger->get_effects()->do_effects(CPlayer::GetThisPlayer(), ctx);
		}
	}
}

/**
**  Register CCL features for triggers.
*/
void TriggerCclRegister()
{
	lua_register(Lua, "AddTrigger", CclAddTrigger);
	lua_register(Lua, "SetDeactivatedTriggers", CclSetDeactivatedTriggers);

	// Conditions
	lua_register(Lua, "GetNumUnitsAt", CclGetNumUnitsAt);
	lua_register(Lua, "IfNearUnit", CclIfNearUnit);
	lua_register(Lua, "IfRescuedNearUnit", CclIfRescuedNearUnit);
}

/**
**  Save the trigger module.
**
**  @param file  Open file to print to
*/
void SaveTriggers(CFile &file)
{
	file.printf("\n--- -----------------------------------------\n");
	file.printf("--- MODULE: triggers\n");

	file.printf("\n");

	file.printf("SetDeactivatedTriggers(");
	for (size_t i = 0; i < wyrmgus::trigger::DeactivatedTriggers.size(); ++i) {
		if (i) {
			file.printf(", ");
		}
		file.printf("\"%s\"", wyrmgus::trigger::DeactivatedTriggers[i].c_str());
	}
	file.printf(")\n");

	file.printf("SetCurrentTriggerId(%d)\n", wyrmgus::trigger::CurrentTriggerId);

	file.printf("\n");
	file.printf("if (Triggers ~= nil) then assert(loadstring(Triggers))() end\n");
	file.printf("\n");

	game::get()->save_game_data(file);
	
	//Wyrmgus start
	if (CurrentQuest != nullptr) {
		file.printf("SetCurrentQuest(\"%s\")\n", CurrentQuest->get_identifier().c_str());
	}
	//Wyrmgus end
}
