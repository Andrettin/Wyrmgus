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
//      (c) Copyright 2016 by Andrettin
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

#include "grand_strategy.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**  Set grand strategy province data.
**
**  @param l  Lua state.
*/
static int CclSetGrandStrategyProvinceData(lua_State *l)
{
	if (lua_gettop(l) < 3) {
		LuaError(l, "incorrect argument");
	}
	std::string province_name = LuaToString(l, 1);
	int province_id = GetProvinceId(province_name);
	if (province_id == -1) {
		LuaError(l, "Province \"%s\" doesn't exist." _C_ province_name.c_str());
	}
	CGrandStrategyProvince *province = GrandStrategyGame.Provinces[province_id];
	
	const char *data = LuaToString(l, 2);
	
	if (!strcmp(data, "Modifier")) {
		LuaCheckArgs(l, 4);
		
		CUpgrade *modifier = CUpgrade::Get(LuaToString(l, 3));
		if (modifier == NULL) {
			LuaError(l, "Modifier doesn't exist.");
		}
		
		bool has_modifier = LuaToBoolean(l, 4);
		
		province->SetModifier(modifier, has_modifier);
	} else if (!strcmp(data, "Governor")) {
		LuaCheckArgs(l, 3);
		
		std::string governor_full_name = LuaToString(l, 3);
		
		province->SetGovernor(governor_full_name);
	} else {
		LuaError(l, "Invalid field: %s" _C_ data);
	}

	return 0;
}

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

		CUpgrade *modifier = CUpgrade::Get(LuaToString(l, 3));
		if (modifier == NULL) {
			LuaError(l, "Modifier doesn't exist.");
		}
		
		lua_pushboolean(l, province->HasModifier(modifier));
		return 1;
	} else if (!strcmp(data, "Modifiers")) {
		lua_createtable(l, province->Modifiers.size(), 0);
		for (size_t i = 1; i <= province->Modifiers.size(); ++i)
		{
			lua_pushstring(l, province->Modifiers[i-1]->Ident.c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "Governor")) {
		if (province->Governor != NULL) {
			lua_pushstring(l, province->Governor->GetFullName().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else {
		LuaError(l, "Invalid field: %s" _C_ data);
	}
	
	return 0;
}

static int CclGetGrandStrategyProvinces(lua_State *l)
{
	lua_createtable(l, GrandStrategyGame.Provinces.size(), 0);
	for (size_t i = 1; i <= GrandStrategyGame.Provinces.size(); ++i)
	{
		lua_pushstring(l, GrandStrategyGame.Provinces[i-1]->Name.c_str());
		lua_rawseti(l, -2, i);
	}
	return 1;
}

/**
**  Register CCL features for provinces.
*/
void GrandStrategyCclRegister()
{
	lua_register(Lua, "SetGrandStrategyProvinceData", CclSetGrandStrategyProvinceData);
	lua_register(Lua, "GetGrandStrategyProvinceData", CclGetGrandStrategyProvinceData);
	lua_register(Lua, "GetGrandStrategyProvinces", CclGetGrandStrategyProvinces);
}

//@}
