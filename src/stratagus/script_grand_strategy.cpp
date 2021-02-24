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
/**@name script_grand_strategy.cpp - The grand strategy ccl functions. */
//
//      (c) Copyright 2016-2021 by Andrettin
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

#include "grand_strategy.h"
#include "luacallback.h"
#include "map/world.h"
#include "script.h"

/**
**  Get grand strategy province data.
**
**  @param l  Lua state.
*/
static int CclGetGrandStrategyProvinceData(lua_State *l)
{
	if (lua_gettop(l) < 2) {
		LuaError(l, "incorrect argument");
	}
	std::string province_name = LuaToString(l, 1);
	int province_id = GetProvinceId(province_name);
	if (province_id == -1) {
		LuaError(l, "Province \"%s\" doesn't exist." _C_ province_name.c_str());
	}
	CGrandStrategyProvince *province = GrandStrategyGame.Provinces[province_id];
	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Water")) {
		lua_pushboolean(l, province->Water);
		return 1;
	} else if (!strcmp(data, "Coastal")) {
		lua_pushboolean(l, province->Coastal);
		return 1;
	} else if (!strcmp(data, "Modifier")) {
		LuaCheckArgs(l, 3);

		CUpgrade *modifier = CUpgrade::get(LuaToString(l, 3));
		lua_pushboolean(l, province->HasModifier(modifier));
		return 1;
	} else if (!strcmp(data, "Modifiers")) {
		lua_createtable(l, province->Modifiers.size(), 0);
		for (size_t i = 1; i <= province->Modifiers.size(); ++i)
		{
			lua_pushstring(l, province->Modifiers[i-1]->get_identifier().c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "BordersModifier")) {
		LuaCheckArgs(l, 3);

		CUpgrade *modifier = CUpgrade::get(LuaToString(l, 3));
		lua_pushboolean(l, province->BordersModifier(modifier));
		return 1;
	} else if (!strcmp(data, "Governor")) {
		if (province->Governor != nullptr) {
			lua_pushstring(l, province->Governor->get_full_name().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else {
		LuaError(l, "Invalid field: %s" _C_ data);
	}
	
	return 0;
}

/**
**  Define a grand strategy event.
**
**  @param l  Lua state.
*/
static int CclDefineGrandStrategyEvent(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string event_name = LuaToString(l, 1);
	CGrandStrategyEvent *event = GetGrandStrategyEvent(event_name);
	if (!event) {
		auto new_event = std::make_unique<CGrandStrategyEvent>();
		event = new_event.get();
		event->Name = event_name;
		event->ID = GrandStrategyEvents.size();
		GrandStrategyEventStringToPointer[event_name] = event;
		GrandStrategyEvents.push_back(std::move(new_event));
	}
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Description")) {
			event->Description = LuaToString(l, -1);
		} else if (!strcmp(value, "Persistent")) {
			event->Persistent = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "MinYear")) {
			event->MinYear = LuaToNumber(l, -1);
		} else if (!strcmp(value, "MaxYear")) {
			event->MaxYear = LuaToNumber(l, -1);
		} else if (!strcmp(value, "HistoricalYear")) {
			event->HistoricalYear = LuaToNumber(l, -1);
		} else if (!strcmp(value, "World")) {
			wyrmgus::world *world = wyrmgus::world::get(LuaToString(l, -1));
			event->World = world;
		} else if (!strcmp(value, "Conditions")) {
			event->Conditions = std::make_unique<LuaCallback>(l, -1);
		} else if (!strcmp(value, "Options")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				std::string option_string = LuaToString(l, -1, j + 1);
				event->Options.push_back(option_string);
			}
		} else if (!strcmp(value, "OptionEffects")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				lua_rawgeti(l, -1, j + 1);
				event->OptionEffects.push_back(std::make_unique<LuaCallback>(l, -1));
				lua_pop(l, 1);
			}
		} else if (!strcmp(value, "OptionConditions")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				lua_rawgeti(l, -1, j + 1);
				event->OptionConditions.push_back(std::make_unique<LuaCallback>(l, -1));
				lua_pop(l, 1);
			}
		} else if (!strcmp(value, "OptionTooltips")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				std::string option_tooltip = LuaToString(l, -1, j + 1);
				event->OptionTooltips.push_back(option_tooltip);
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	return 0;
}

/**
**  Get grand strategy event data.
**
**  @param l  Lua state.
*/
static int CclGetGrandStrategyEventData(lua_State *l)
{
	if (lua_gettop(l) < 2) {
		LuaError(l, "incorrect argument");
	}
	std::string event_name = LuaToString(l, 1);
	CGrandStrategyEvent *event = GetGrandStrategyEvent(event_name);
	if (event == nullptr) {
		LuaError(l, "Event \"%s\" doesn't exist." _C_ event_name.c_str());
	}
	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Description")) {
		lua_pushstring(l, event->Description.c_str());
		return 1;
	} else if (!strcmp(data, "World")) {
		if (event->World != nullptr) {
			lua_pushstring(l, event->World->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Persistent")) {
		lua_pushboolean(l, event->Persistent);
		return 1;
	} else if (!strcmp(data, "MinYear")) {
		lua_pushnumber(l, event->MinYear);
		return 1;
	} else if (!strcmp(data, "MaxYear")) {
		lua_pushnumber(l, event->MaxYear);
		return 1;
	} else if (!strcmp(data, "HistoricalYear")) {
		lua_pushnumber(l, event->HistoricalYear);
		return 1;
	} else if (!strcmp(data, "Options")) {
		lua_createtable(l, event->Options.size(), 0);
		for (size_t i = 1; i <= event->Options.size(); ++i)
		{
			lua_pushstring(l, event->Options[i-1].c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "OptionCondition")) {
		LuaCheckArgs(l, 3);

		int option = LuaToNumber(l, 3) - 1;
		if ((int) event->OptionConditions.size() > option && event->OptionConditions[option]) {
			event->OptionConditions[option]->pushPreamble();
			event->OptionConditions[option]->run(1);
			if (event->OptionConditions[option]->popBoolean() == false) {
				lua_pushboolean(l, event->OptionConditions[option]->popBoolean());
			}
		} else {
			lua_pushboolean(l, true);
		}
		return 1;
	} else if (!strcmp(data, "OptionEffect")) {
		LuaCheckArgs(l, 3);

		int option = LuaToNumber(l, 3) - 1;
		if ((int) event->OptionEffects.size() > option && event->OptionEffects[option]) {
			event->OptionEffects[option]->pushPreamble();
			event->OptionEffects[option]->run();
		}
		return 1;
	} else if (!strcmp(data, "OptionTooltips")) {
		lua_createtable(l, event->OptionTooltips.size(), 0);
		for (size_t i = 1; i <= event->OptionTooltips.size(); ++i)
		{
			lua_pushstring(l, event->OptionTooltips[i-1].c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else {
		LuaError(l, "Invalid field: %s" _C_ data);
	}
	
	return 0;
}

/**
**  Register CCL features for provinces.
*/
void GrandStrategyCclRegister()
{
	lua_register(Lua, "GetGrandStrategyProvinceData", CclGetGrandStrategyProvinceData);
	lua_register(Lua, "DefineGrandStrategyEvent", CclDefineGrandStrategyEvent);
	lua_register(Lua, "GetGrandStrategyEventData", CclGetGrandStrategyEventData);
}
