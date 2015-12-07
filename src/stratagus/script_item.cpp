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
**  Define a unique item.
**
**  @param l  Lua state.
*/
static int CclDefineUniqueItem(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string item_name = LuaToString(l, 1);
	CUniqueItem *item = GetUniqueItem(item_name);
	if (!item) {
		item = new CUniqueItem;
		UniqueItems.push_back(item);
		item->Name = item_name;
	}
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Type")) {
			std::string unit_type_ident = LuaToString(l, -1);
			int unit_type_id = UnitTypeIdByIdent(unit_type_ident);
			if (unit_type_id != -1) {
				item->Type = const_cast<CUnitType *>(&(*UnitTypes[unit_type_id]));
				UnitTypes[unit_type_id]->Uniques.push_back(item);
			} else {
				LuaError(l, "Unit type \"%s\" doesn't exist." _C_ unit_type_ident.c_str());
			}
		} else if (!strcmp(value, "Prefix")) {
			std::string affix_ident = LuaToString(l, -1);
			int upgrade_id = UpgradeIdByIdent(affix_ident);
			if (upgrade_id != -1) {
				item->Prefix = const_cast<CUpgrade *>(&(*AllUpgrades[upgrade_id]));
			} else {
				LuaError(l, "Affix upgrade \"%s\" doesn't exist." _C_ affix_ident.c_str());
			}
		} else if (!strcmp(value, "Suffix")) {
			std::string affix_ident = LuaToString(l, -1);
			int upgrade_id = UpgradeIdByIdent(affix_ident);
			if (upgrade_id != -1) {
				item->Suffix = const_cast<CUpgrade *>(&(*AllUpgrades[upgrade_id]));
			} else {
				LuaError(l, "Affix upgrade \"%s\" doesn't exist." _C_ affix_ident.c_str());
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	return 0;
}

// ----------------------------------------------------------------------------

/**
**  Register CCL features for items.
*/
void ItemCclRegister()
{
	lua_register(Lua, "DefineUniqueItem", CclDefineUniqueItem);
}

//@}
