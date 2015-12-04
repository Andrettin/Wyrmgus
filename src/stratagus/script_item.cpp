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
/**@name script_item.cpp - The item ccl functions. */
//
//      (c) Copyright 2015 by Andrettin
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

#include "item.h"

#include "player.h"
#include "script.h"
#include "unittype.h"
#include "upgrade.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**  Define an item type.
**
**  @param l  Lua state.
*/
static int CclDefineItemType(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string item_type_name = LuaToString(l, 1);
	CItemType *item_type = GetItemType(item_type_name);
	if (!item_type) {
		item_type = new CItemType;
		ItemTypes.push_back(item_type);
	}
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Class")) {
			item_type->Class = GetItemClassIdByName(LuaToString(l, -1));
		} else if (!strcmp(value, "UnitType")) {
			std::string unit_type_ident = LuaToString(l, -1);
			int unit_type_id = UnitTypeIdByIdent(unit_type_ident);
			if (unit_type_id != -1) {
				item_type->UnitType = const_cast<CUnitType *>(&(*UnitTypes[unit_type_id]));
			} else {
				LuaError(l, "Unit type \"%s\" doesn't exist." _C_ unit_type_ident.c_str());
			}
		} else if (!strcmp(value, "Icon")) {
			item_type->Icon.Name = LuaToString(l, -1);
			item_type->Icon.Icon = NULL;
			item_type->Icon.Load();
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	return 0;
}

/**
**  Get item type data.
**
**  @param l  Lua state.
*/
static int CclGetItemTypeData(lua_State *l)
{
	if (lua_gettop(l) < 2) {
		LuaError(l, "incorrect argument");
	}
	std::string item_type_name = LuaToString(l, 1);
	CItemType *item_type = GetItemType(item_type_name);
	if (!item_type) {
		LuaError(l, "Item type \"%s\" doesn't exist." _C_ item_type_name.c_str());
	}
	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Name")) {
		lua_pushstring(l, item_type->Name.c_str());
		return 1;
	} else if (!strcmp(data, "Class")) {
		lua_pushstring(l, GetItemClassNameById(item_type->Class).c_str());
		return 1;
	} else if (!strcmp(data, "UnitType")) {
		if (item_type->UnitType != NULL) {
			lua_pushstring(l, item_type->UnitType->Ident.c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Icon")) {
		lua_pushstring(l, item_type->Icon.Name.c_str());
		return 1;
	} else {
		LuaError(l, "Invalid field: %s" _C_ data);
	}

	return 0;
}

static int CclGetItemTypes(lua_State *l)
{
	lua_createtable(l, ItemTypes.size(), 0);
	for (size_t i = 1; i <= ItemTypes.size(); ++i)
	{
		lua_pushstring(l, ItemTypes[i-1]->Name.c_str());
		lua_rawseti(l, -2, i);
	}
	return 1;
}

// ----------------------------------------------------------------------------

/**
**  Register CCL features for items.
*/
void ItemCclRegister()
{
	lua_register(Lua, "DefineItemType", CclDefineItemType);
	lua_register(Lua, "GetItemTypeData", CclGetItemTypeData);
	lua_register(Lua, "GetItemTypes", CclGetItemTypes);
}

//@}
