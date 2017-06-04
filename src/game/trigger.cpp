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
/**@name trigger.cpp - The trigger handling. */
//
//      (c) Copyright 2002-2015 by Lutz Sammer, Jimmy Salmon and Andrettin
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

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "trigger.h"

#include "interface.h"
#include "iolib.h"
//Wyrmgus start
#include "luacallback.h"
//Wyrmgus end
#include "map.h"
#include "player.h"
//Wyrmgus start
#include "quest.h" // for saving quests
//Wyrmgus end
#include "results.h"
#include "script.h"
#include "unit.h"
#include "unit_find.h"
#include "unittype.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

CTimer GameTimer;               /// The game timer
static int Trigger;
static bool *ActiveTriggers;

/// Some data accessible for script during the game.
TriggerDataType TriggerData;

//Wyrmgus start
std::vector<CTrigger *> Triggers;
std::vector<std::string> DeactivatedTriggers;
std::map<std::string, CTrigger *> TriggerIdentToPointer;
//Wyrmgus end

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
		return ThisPlayer->Index;
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
	return NULL;
}

/**
**  Return the number of units of a given unit-type and player at a location.
*/
static int CclGetNumUnitsAt(lua_State *l)
{
	//Wyrmgus start
//	LuaCheckArgs(l, 4);
	const int nargs = lua_gettop(l);
	if (nargs < 4 || nargs > 5) {
		LuaError(l, "incorrect argument\n");
	}
	//Wyrmgus end

	int plynr = LuaToNumber(l, 1);
	lua_pushvalue(l, 2);
	const CUnitType *unittype = TriggerGetUnitType(l);
	lua_pop(l, 1);

	Vec2i minPos;
	Vec2i maxPos;
	CclGetPos(l, &minPos.x, &minPos.y, 3);
	CclGetPos(l, &maxPos.x, &maxPos.y, 4);
	
	//Wyrmgus start
	minPos.x = std::max<int>(minPos.x, 0);
	minPos.y = std::max<int>(minPos.y, 0);
	//Wyrmgus end

	std::vector<CUnit *> units;

	//Wyrmgus start
	int z = 0;
	if (nargs >= 5) {
		z = LuaToNumber(l, 5);
	}
	
	if (z == 0 && (Map.Info.MapWidths.size() == 0 || Map.Info.MapHeights.size() == 0)) {
		maxPos.x = std::min<int>(maxPos.x, Map.Info.MapWidth - 1);
		maxPos.y = std::min<int>(maxPos.y, Map.Info.MapHeight - 1);
	} else if (z != -1) {
		maxPos.x = std::min<int>(maxPos.x, Map.Info.MapWidths[z] - 1);
		maxPos.y = std::min<int>(maxPos.y, Map.Info.MapHeights[z] - 1);
	}
	
	if (z == -1 || !Map.Info.IsPointOnMap(minPos, z) || !Map.Info.IsPointOnMap(maxPos, z)) {
		lua_pushnumber(l, 0);
		return 1;
	}
	
//	Select(minPos, maxPos, units);
	Select(minPos, maxPos, units, z);
	//Wyrmgus end

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
			|| (unittype == ALL_FOODUNITS && !unit.Type->Building)
			|| (unittype == ALL_BUILDINGS && unit.Type->Building)
			|| (unittype == unit.Type && !unit.Constructed)) {

			// Check the player
			//Wyrmgus start
//			if (plynr == -1 || plynr == unit.Player->Index) {
			if (plynr == -1 || plynr == unit.Player->Index || (plynr == -2 && unit.Player->Type != PlayerNeutral)) { // -2 can now be used for the player field to mean any non-neutral player
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
		LuaError(l, "CclIfNearUnit: not a unit-type valid");
	}
	CompareFunction compare = GetCompareFunction(op);
	if (!compare) {
		LuaError(l, "Illegal comparison operation in if-near-unit: %s" _C_ op);
	}

	//
	// Get all unit types 'near'.
	//

	std::vector<CUnit *> unitsOfType;

	FindUnitsByType(*ut2, unitsOfType);
	for (size_t i = 0; i != unitsOfType.size(); ++i) {
		const CUnit &centerUnit = *unitsOfType[i];

		std::vector<CUnit *> around;
		SelectAroundUnit(centerUnit, 1, around);

		// Count the requested units
		int s = 0;
		for (size_t j = 0; j < around.size(); ++j) {
			const CUnit &unit = *around[j];

			// Check unit type
			if (unittype == ANY_UNIT
				|| (unittype == ALL_FOODUNITS && !unit.Type->Building)
				|| (unittype == ALL_BUILDINGS && unit.Type->Building)
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
					|| (unittype == ALL_FOODUNITS && !unit.Type->Building)
					|| (unittype == ALL_BUILDINGS && unit.Type->Building)
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
		const int unitCount = Players[i].GetUnitCount();

		// This player is our enemy and has units left.
		if ((Players[player].IsEnemy(Players[i])) || (Players[i].IsEnemy(Players[player]))) {
			// Don't count walls
			for (int j = 0; j < unitCount; ++j) {
				if (Players[i].GetUnit(j).Type->BoolFlag[WALL_INDEX].value == false) {
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
	//Wyrmgus start
	/*
	LuaCheckArgs(l, 2);
	if (!lua_isfunction(l, 1)
		|| (!lua_isfunction(l, 2) && !lua_istable(l, 2))) {
		LuaError(l, "incorrect argument");
	}

	// Make a list of all triggers.
	// A trigger is a pair of condition and action
	lua_getglobal(l, "_triggers_");

	if (lua_isnil(l, -1)) {
		DebugPrint("Trigger not set, defining trigger\n");
		lua_pop(l, 1);
		lua_newtable(l);
		lua_setglobal(l, "_triggers_");
		lua_getglobal(l, "_triggers_");
	}

	const int i = lua_rawlen(l, -1);
	if (ActiveTriggers && !ActiveTriggers[i / 2]) {
		lua_pushnil(l);
		lua_rawseti(l, -2, i + 1);
		lua_pushnil(l);
		lua_rawseti(l, -2, i + 2);
	} else {
		lua_pushvalue(l, 1);
		lua_rawseti(l, -2, i + 1);
		lua_newtable(l);
		lua_pushvalue(l, 2);
		lua_rawseti(l, -2, 1);
		lua_rawseti(l, -2, i + 2);
	}
	lua_pop(l, 1);
	*/
	//Wyrmgus end
	
	//Wyrmgus start
	LuaCheckArgs(l, 3);
	
	if (!lua_isfunction(l, 2) || !lua_isfunction(l, 3)) {
		LuaError(l, "incorrect argument");
	}

	std::string trigger_ident = LuaToString(l, 1);
	
	if (std::find(DeactivatedTriggers.begin(), DeactivatedTriggers.end(), trigger_ident) != DeactivatedTriggers.end()) {
		return 0;
	}
	
	CTrigger *trigger = GetTrigger(trigger_ident);
	if (!trigger) {
		trigger = new CTrigger;
		trigger->Ident = trigger_ident;
		Triggers.push_back(trigger);
		TriggerIdentToPointer[trigger_ident] = trigger;
	}
	
	trigger->Conditions = new LuaCallback(l, 2);
	trigger->Effects = new LuaCallback(l, 3);
	
	if (trigger->Conditions == NULL || trigger->Effects == NULL) {
		fprintf(stderr, "Trigger \"%s\" has no conditions or no effects.\n", trigger->Ident.c_str());
	}
	//Wyrmgus end

	return 0;
}

/**
**  Set the trigger values
*/
void SetTrigger(int trigger)
{
	Trigger = trigger;
}

/**
**  Set the active triggers
*/
static int CclSetActiveTriggers(lua_State *l)
{
	const int args = lua_gettop(l);

	ActiveTriggers = new bool[args];
	for (int j = 0; j < args; ++j) {
		ActiveTriggers[j] = LuaToBoolean(l, j + 1);
	}
	return 0;
}

//Wyrmgus start
/**
**  Set the deactivated triggers
*/
static int CclSetDeactivatedTriggers(lua_State *l)
{
	const int args = lua_gettop(l);

	for (int j = 0; j < args; ++j) {
		DeactivatedTriggers.push_back(LuaToString(l, j + 1));
	}
	return 0;
}
//Wyrmgus end

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
	//Wyrmgus start
//	const int base = lua_gettop(Lua);
	//Wyrmgus end

	//Wyrmgus start
//	lua_getglobal(Lua, "_triggers_");
//	int triggers = lua_rawlen(Lua, -1);
	int triggers = Triggers.size();
	//Wyrmgus end

	if (Trigger >= triggers) {
		Trigger = 0;
	}

	if (GamePaused) {
		//Wyrmgus start
//		lua_pop(Lua, 1);
		//Wyrmgus end
		return;
	}

	// Skip to the next trigger
	//Wyrmgus start
	/*
	while (Trigger < triggers) {
		lua_rawgeti(Lua, -1, Trigger + 1);
		if (!lua_isnumber(Lua, -1)) {
			break;
		}
		lua_pop(Lua, 1);
		Trigger += 2;
	}
	*/
	//Wyrmgus end
	if (Trigger < triggers) {
		//Wyrmgus start
//		int currentTrigger = Trigger;
		CTrigger *current_trigger = Triggers[Trigger];
//		Trigger += 2;
		Trigger += 1;
		//Wyrmgus end
		//Wyrmgus start
		/*
		LuaCall(0, 0);
		// If condition is true execute action
		if (lua_gettop(Lua) > base + 1 && lua_toboolean(Lua, -1)) {
			lua_settop(Lua, base + 1);
			if (TriggerExecuteAction(currentTrigger + 1)) {
				TriggerRemoveTrigger(currentTrigger);
			}
		}
		lua_settop(Lua, base + 1);
		*/
		
		if (current_trigger->Conditions && current_trigger->Effects) {
			current_trigger->Conditions->pushPreamble();
			current_trigger->Conditions->run(1);
			if (current_trigger->Conditions->popBoolean()) {
				current_trigger->Effects->pushPreamble();
				current_trigger->Effects->run(1);
				if (current_trigger->Effects->popBoolean() == false) {
					DeactivatedTriggers.push_back(current_trigger->Ident);
					Triggers.erase(std::remove(Triggers.begin(), Triggers.end(), current_trigger), Triggers.end());
					TriggerIdentToPointer.erase(current_trigger->Ident);
					delete current_trigger;
				}
			}
		}
		//Wyrmgus end
	}
	//Wyrmgus start
//	lua_pop(Lua, 1);
	//Wyrmgus end
}

//Wyrmgus start
CTrigger *GetTrigger(std::string trigger_ident)
{
	if (TriggerIdentToPointer.find(trigger_ident) != TriggerIdentToPointer.end()) {
		return TriggerIdentToPointer[trigger_ident];
	}
	
	return NULL;
}

CTrigger::~CTrigger()
{
	delete Conditions;
	delete Effects;
}
//Wyrmgus end

/**
**  Register CCL features for triggers.
*/
void TriggerCclRegister()
{
	lua_register(Lua, "AddTrigger", CclAddTrigger);
	lua_register(Lua, "SetActiveTriggers", CclSetActiveTriggers);
	//Wyrmgus start
	lua_register(Lua, "SetDeactivatedTriggers", CclSetDeactivatedTriggers);
	//Wyrmgus end
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
	//Wyrmgus start
//	lua_getglobal(Lua, "_triggers_");
//	const int triggers = lua_rawlen(Lua, -1);
	//Wyrmgus end

	//Wyrmgus start
	/*
	file.printf("SetActiveTriggers(");
	for (int i = 0; i < triggers; i += 2) {
		lua_rawgeti(Lua, -1, i + 1);
		if (i) {
			file.printf(", ");
		}
		if (!lua_isnil(Lua, -1)) {
			file.printf("true");
		} else {
			file.printf("false");
		}
		lua_pop(Lua, 1);
	}
	file.printf(")\n");
	*/
	file.printf("SetDeactivatedTriggers(");
	for (size_t i = 0; i < DeactivatedTriggers.size(); ++i) {
		if (i) {
			file.printf(", ");
		}
		file.printf("\"%s\"", DeactivatedTriggers[i].c_str());
	}
	file.printf(")\n");
	//Wyrmgus end

	file.printf("SetTrigger(%d)\n", Trigger);

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
	if (CurrentQuest != NULL) {
		file.printf("SetCurrentQuest(\"%s\")\n", CurrentQuest->Ident.c_str());
	}
	//Wyrmgus end
}

/**
**  Initialize the trigger module.
*/
void InitTriggers()
{
	// Setup default triggers

	// FIXME: choose the triggers for game type

	lua_getglobal(Lua, "_triggers_");
	if (lua_isnil(Lua, -1)) {
		lua_getglobal(Lua, "SinglePlayerTriggers");
		LuaCall(0, 1);
	}
	lua_pop(Lua, 1);
}

/**
**  Clean up the trigger module.
*/
void CleanTriggers()
{
	lua_pushnil(Lua);
	lua_setglobal(Lua, "_triggers_");

	lua_pushnil(Lua);
	lua_setglobal(Lua, "Triggers");

	Trigger = 0;

	delete[] ActiveTriggers;
	ActiveTriggers = NULL;
	
	//Wyrmgus start
	for (size_t i = 0; i < Triggers.size(); ++i) {
		delete Triggers[i];
	}
	Triggers.clear();
	TriggerIdentToPointer.clear();
	DeactivatedTriggers.clear();
	
	for (size_t i = 0; i < Quests.size(); ++i) {
		Quests[i]->CurrentCompleted = false;
	}
	//Wyrmgus end

	GameTimer.Reset();
}

//@}
