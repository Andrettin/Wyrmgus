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
/**@name trigger.cpp - The trigger source file. */
//
//      (c) Copyright 2002-2019 by Lutz Sammer, Jimmy Salmon and Andrettin
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

#include "game/trigger.h"

#include "config.h"
#include "dependency/and_dependency.h"
#include "dependency/dependency.h"
#include "game/trigger_effect.h"
#include "iolib.h"
//Wyrmgus start
#include "luacallback.h"
//Wyrmgus end
#include "map/map.h"
#include "player.h"
#include "quest/campaign.h"
//Wyrmgus start
#include "quest/quest.h" // for saving quests
//Wyrmgus end
#include "results.h"
#include "script.h"
#include "ui/interface.h"
#include "unit/unit.h"
#include "unit/unit_find.h"
#include "unit/unit_type.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

CTimer GameTimer;               /// The game timer

/// Some data accessible for script during the game.
TriggerDataType TriggerData;

std::vector<CTrigger *> CTrigger::Triggers;
std::vector<CTrigger *> CTrigger::ActiveTriggers;
std::map<std::string, CTrigger *> CTrigger::TriggersByIdent;
std::vector<std::string> CTrigger::DeactivatedTriggers;
unsigned int CTrigger::CurrentTriggerId = 0;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

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
		return CPlayer::GetThisPlayer()->Index;
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
const CUnitType *TriggerGetUnitType(lua_State *l)
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
	const CUnitType *unittype = TriggerGetUnitType(l);
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
	
	if (z == 0 && (CMap::Map.Info.MapWidths.size() == 0 || CMap::Map.Info.MapHeights.size() == 0)) {
		maxPos.x = std::min<int>(maxPos.x, CMap::Map.Info.MapWidth - 1);
		maxPos.y = std::min<int>(maxPos.y, CMap::Map.Info.MapHeight - 1);
	} else if (z != -1) {
		maxPos.x = std::min<int>(maxPos.x, CMap::Map.Info.MapWidths[z] - 1);
		maxPos.y = std::min<int>(maxPos.y, CMap::Map.Info.MapHeights[z] - 1);
	}
	
	if (z == -1 || !CMap::Map.Info.IsPointOnMap(minPos, z) || !CMap::Map.Info.IsPointOnMap(maxPos, z)) {
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
//			if (plynr == -1 || plynr == unit.Player->Index) {
			if (plynr == -1 || plynr == unit.Player->Index || (plynr == -2 && unit.Player->Type != PlayerNeutral)) { // -2 can be used for the player field to mean any non-neutral player
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
	const CUnitType *unittype = TriggerGetUnitType(l);
	lua_pop(l, 1);
	lua_pushvalue(l, 5);
	const CUnitType *ut2 = CclGetUnitType(l);
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
	
	//
	// Get all unit types 'near'.
	//

	std::vector<CUnit *> unitsOfType;

	FindUnitsByType(*ut2, unitsOfType);
	for (size_t i = 0; i != unitsOfType.size(); ++i) {
		const CUnit &centerUnit = *unitsOfType[i];

		if (other_plynr != -1 && other_plynr != centerUnit.Player->Index) {
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
				if (plynr == -1 || plynr == unit.Player->Index) {
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
	const CUnitType *unittype = TriggerGetUnitType(l);
	lua_pop(l, 1);
	const CUnitType *ut2 = CclGetUnitType(l);
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

			if (unit.RescuedFrom) { // only rescued units
				// Check unit type

				if (unittype == ANY_UNIT
					|| (unittype == ALL_FOODUNITS && !unit.Type->BoolFlag[BUILDING_INDEX].value)
					|| (unittype == ALL_BUILDINGS && unit.Type->BoolFlag[BUILDING_INDEX].value)
					|| (unittype == unit.Type)) {

					// Check the player
					if (plynr == -1 || plynr == unit.Player->Index) {
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
		if ((CPlayer::Players[player]->IsEnemy(*CPlayer::Players[i])) || (CPlayer::Players[i]->IsEnemy(*CPlayer::Players[player]))) {
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

/**
**  Check the timer value
*/
int GetTimer()
{
	if (!GameTimer.Init) {
		return 0;
	}
	return GameTimer.Cycles;
}

/*---------------------------------------------------------------------------
-- Actions
---------------------------------------------------------------------------*/

/**
**  Stop the running game with a given result
*/
void StopGame(GameResults result)
{
	GameResult = result;
	GamePaused = true;
	GameRunning = false;
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
**  Action set timer
*/
void ActionSetTimer(int cycles, bool increasing)
{
	GameTimer.Cycles = cycles;
	GameTimer.Increasing = increasing;
	GameTimer.Init = true;
	GameTimer.LastUpdate = GameCycle;
}

/**
**  Action start timer
*/
void ActionStartTimer()
{
	GameTimer.Running = true;
	GameTimer.Init = true;
}

/**
**  Action stop timer
*/
void ActionStopTimer()
{
	GameTimer.Running = false;
}

/**
**  Add a trigger.
*/
static int CclAddTrigger(lua_State *l)
{
	LuaCheckArgs(l, 3);
	
	if (!lua_isfunction(l, 2) || !lua_isfunction(l, 3)) {
		LuaError(l, "incorrect argument");
	}

	std::string trigger_ident = LuaToString(l, 1);
	
	if (std::find(CTrigger::DeactivatedTriggers.begin(), CTrigger::DeactivatedTriggers.end(), trigger_ident) != CTrigger::DeactivatedTriggers.end()) {
		return 0;
	}
	
	//this function only adds temporary triggers, that is, ones that will only last for the current game
	
	CTrigger *trigger = new CTrigger;
	trigger->Ident = trigger_ident;
	trigger->Local = true;
	CTrigger::ActiveTriggers.push_back(trigger);
	
	trigger->Conditions = new LuaCallback(l, 2);
	trigger->Effects = new LuaCallback(l, 3);
	
	if (trigger->Conditions == nullptr || trigger->Effects == nullptr) {
		fprintf(stderr, "Trigger \"%s\" has no conditions or no effects.\n", trigger->Ident.c_str());
	}

	return 0;
}

/**
**  Set the trigger values
*/
void SetCurrentTriggerId(unsigned int trigger_id)
{
	CTrigger::CurrentTriggerId = trigger_id;
}

/**
**  Set the deactivated triggers
*/
static int CclSetDeactivatedTriggers(lua_State *l)
{
	const int args = lua_gettop(l);

	for (int j = 0; j < args; ++j) {
		CTrigger::DeactivatedTriggers.push_back(LuaToString(l, j + 1));
	}
	return 0;
}

/**
**  Execute a trigger action
**
**  @param script  Script to execute
**
**  @return        1 if the trigger should be removed
*/
static int TriggerExecuteAction(int script)
{
	const int base = lua_gettop(Lua);
	int ret = 0;

	lua_rawgeti(Lua, -1, script + 1);
	const int args = lua_rawlen(Lua, -1);
	for (int j = 0; j < args; ++j) {
		lua_rawgeti(Lua, -1, j + 1);
		LuaCall(0, 0);
		if (lua_gettop(Lua) > base + 1 && lua_toboolean(Lua, -1)) {
			ret = 1;
		} else {
			ret = 0;
		}
		lua_settop(Lua, base + 1);
	}
	lua_pop(Lua, 1);

	// If action returns false remove it
	return !ret;
}

/**
**  Remove a trigger
**
**  @param trig  Current trigger
*/
static void TriggerRemoveTrigger(int trig)
{
	lua_pushnumber(Lua, -1);
	lua_rawseti(Lua, -2, trig + 1);
	lua_pushnumber(Lua, -1);
	lua_rawseti(Lua, -2, trig + 2);
}

/**
**  Check trigger each game cycle.
*/
void TriggersEachCycle()
{
	if (CTrigger::CurrentTriggerId >= CTrigger::ActiveTriggers.size()) {
		CTrigger::CurrentTriggerId = 0;
	}

	if (GamePaused) {
		return;
	}

	// go to the next trigger
	if (CTrigger::CurrentTriggerId < CTrigger::ActiveTriggers.size()) {
		CTrigger *current_trigger = CTrigger::ActiveTriggers[CTrigger::CurrentTriggerId];

		bool removed_trigger = false;
		
		//old Lua conditions/effects for triggers
		if (current_trigger->Conditions && current_trigger->Effects) {
			current_trigger->Conditions->pushPreamble();
			current_trigger->Conditions->run(1);
			if (current_trigger->Conditions->popBoolean()) {
				current_trigger->Effects->pushPreamble();
				current_trigger->Effects->run(1);
				if (current_trigger->Effects->popBoolean() == false) {
					CTrigger::DeactivatedTriggers.push_back(current_trigger->Ident);
					CTrigger::ActiveTriggers.erase(CTrigger::ActiveTriggers.begin() + CTrigger::CurrentTriggerId);
					removed_trigger = true;
					if (current_trigger->Local) {
						delete current_trigger;
					}
				}
			}
		}
		
		if (!current_trigger->TriggerEffects.empty() && current_trigger->GetRandomChance() > SyncRand(100)) {
			bool triggered = false;
			
			if (current_trigger->Type == CTrigger::TriggerType::GlobalTrigger) {
				if (CheckDependencies(current_trigger, CPlayer::Players[PlayerNumNeutral])) {
					triggered = true;
					for (CTriggerEffect *trigger_effect : current_trigger->TriggerEffects) {
						trigger_effect->Do(CPlayer::Players[PlayerNumNeutral]);
					}
				}
			} else if (current_trigger->Type == CTrigger::TriggerType::PlayerTrigger) {
				for (int i = 0; i < PlayerNumNeutral; ++i) {
					CPlayer *player = CPlayer::Players[i];
					if (player->Type == PlayerNobody) {
						continue;
					}
					if (!CheckDependencies(current_trigger, player)) {
						continue;
					}
					triggered = true;
					for (CTriggerEffect *trigger_effect : current_trigger->TriggerEffects) {
						trigger_effect->Do(player);
					}
					if (current_trigger->OnlyOnce) {
						break;
					}
				}
			}
			
			if (triggered && current_trigger->OnlyOnce) {
				CTrigger::DeactivatedTriggers.push_back(current_trigger->Ident);
				CTrigger::ActiveTriggers.erase(CTrigger::ActiveTriggers.begin() + CTrigger::CurrentTriggerId);
				removed_trigger = true;
				if (current_trigger->Local) {
					delete current_trigger;
				}
			}
		}
		
		if (!removed_trigger) {
			CTrigger::CurrentTriggerId++;
		}
	} else {
		CTrigger::CurrentTriggerId = 0;
	}
}

CTrigger *CTrigger::GetTrigger(const std::string &ident, const bool should_find)
{
	std::map<std::string, CTrigger *>::const_iterator find_iterator = TriggersByIdent.find(ident);
	
	if (find_iterator != TriggersByIdent.end()) {
		return find_iterator->second;
	}
	
	if (should_find) {
		fprintf(stderr, "Invalid trigger: \"%s\".\n", ident.c_str());
	}

	return nullptr;
}

CTrigger *CTrigger::GetOrAddTrigger(const std::string &ident)
{
	CTrigger *trigger = GetTrigger(ident, false);
	
	if (!trigger) {
		trigger = new CTrigger;
		trigger->Ident = ident;
		Triggers.push_back(trigger);
		TriggersByIdent[ident] = trigger;
	}
	
	return trigger;
}

/**
**	Cleanup the trigger module.
*/
void CTrigger::ClearTriggers()
{
	ClearActiveTriggers();

	for (CTrigger *trigger : CTrigger::Triggers) {
		delete trigger;
	}
	
	CTrigger::Triggers.clear();
	CTrigger::TriggersByIdent.clear();
}

/**
**	Initialize the trigger module.
*/
void CTrigger::InitActiveTriggers()
{
	// Setup default triggers

	// FIXME: choose the triggers for game type

	lua_getglobal(Lua, "_triggers_");
	if (lua_isnil(Lua, -1)) {
		lua_getglobal(Lua, "SinglePlayerTriggers");
		LuaCall(0, 1);
	}
	lua_pop(Lua, 1);
	
	for (CTrigger *trigger : CTrigger::Triggers) {
		if (std::find(CTrigger::DeactivatedTriggers.begin(), CTrigger::DeactivatedTriggers.end(), trigger->Ident) != CTrigger::DeactivatedTriggers.end()) {
			continue;
		}
		if (trigger->CampaignOnly && CCampaign::GetCurrentCampaign() == nullptr) {
			continue;
		}
		CTrigger::ActiveTriggers.push_back(trigger);
	}
}

void CTrigger::ClearActiveTriggers()
{
	lua_pushnil(Lua);
	lua_setglobal(Lua, "_triggers_");

	lua_pushnil(Lua);
	lua_setglobal(Lua, "Triggers");

	CTrigger::CurrentTriggerId = 0;

	for (CTrigger *trigger : CTrigger::ActiveTriggers) {
		if (trigger->Local) {
			delete trigger;
		}
	}
	
	CTrigger::ActiveTriggers.clear();
	CTrigger::DeactivatedTriggers.clear();
	
	//Wyrmgus start
	for (CQuest *quest : CQuest::GetAll()) {
		quest->CurrentCompleted = false;
	}
	//Wyrmgus end
	
	GameTimer.Reset();
}

CTrigger::~CTrigger()
{
	if (this->Conditions) {
		delete this->Conditions;
	}
	
	if (this->Effects) {
		delete this->Effects;
	}
	
	for (CTriggerEffect *effect : this->TriggerEffects) {
		delete effect;
	}
}

/**
**	@brief	Process a property in the data provided by a configuration file
**
**	@param	key		The property's key
**	@param	value	The property's value
**
**	@return	True if the property can be processed, or false otherwise
*/
bool CTrigger::ProcessConfigDataProperty(const std::string &key, std::string value)
{
	if (key == "type") {
		if (value == "global_trigger") {
			this->Type = TriggerType::GlobalTrigger;
		} else if (value == "player_trigger") {
			this->Type = TriggerType::PlayerTrigger;
		} else {
			fprintf(stderr, "Invalid trigger type: \"%s\".\n", value.c_str());
		}
	} else if (key == "only_once") {
		this->OnlyOnce = StringToBool(value);
	} else if (key == "campaign_only") {
		this->CampaignOnly = StringToBool(value);
	} else {
		return false;
	}
	
	return true;
}
	
/**
**	@brief	Process a section in the data provided by a configuration file
**
**	@param	section		The section
**
**	@return	True if the section can be processed, or false otherwise
*/
bool CTrigger::ProcessConfigDataSection(const CConfigData *section)
{
	if (section->Tag == "effects") {
		for (const CConfigData *subsection : section->Sections) {
			CTriggerEffect *trigger_effect = CTriggerEffect::FromConfigData(subsection);
			this->TriggerEffects.push_back(trigger_effect);
		}
	} else if (section->Tag == "dependencies") {
		this->Dependency = new CAndDependency;
		this->Dependency->ProcessConfigData(section);
	} else {
		return false;
	}
	
	return true;
}

void CTrigger::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("set_random_chance", "random_chance"), [](CTrigger *trigger, const int random_chance){ trigger->RandomChance = random_chance; });
	ClassDB::bind_method(D_METHOD("get_random_chance"), &CTrigger::GetRandomChance;
	ADD_PROPERTY(PropertyInfo(Variant::INT, "random_chance"), "set_random_chance", "get_random_chance");
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
	for (size_t i = 0; i < CTrigger::DeactivatedTriggers.size(); ++i) {
		if (i) {
			file.printf(", ");
		}
		file.printf("\"%s\"", CTrigger::DeactivatedTriggers[i].c_str());
	}
	file.printf(")\n");

	file.printf("SetCurrentTriggerId(%d)\n", CTrigger::CurrentTriggerId);

	if (GameTimer.Init) {
		file.printf("ActionSetTimer(%ld, %s)\n",
					GameTimer.Cycles, (GameTimer.Increasing ? "true" : "false"));
		if (GameTimer.Running) {
			file.printf("ActionStartTimer()\n");
		}
	}

	file.printf("\n");
	file.printf("if (Triggers ~= nil) then assert(loadstring(Triggers))() end\n");
	file.printf("\n");
	
	//Wyrmgus start
	if (CurrentQuest != nullptr) {
		file.printf("SetCurrentQuest(\"%s\")\n", CurrentQuest->Ident.c_str());
	}
	//Wyrmgus end
}
