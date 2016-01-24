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
#include "spells.h"
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
		} else if (!strcmp(value, "Spell")) {
			std::string spell_ident = LuaToString(l, -1);
			SpellType *spell = SpellTypeByIdent(spell_ident);
			if (spell != NULL) {
				item->Spell = const_cast<SpellType *>(&(*spell));
			} else {
				LuaError(l, "Spell \"%s\" doesn't exist." _C_ spell_ident.c_str());
			}
		} else if (!strcmp(value, "Work")) {
			std::string affix_ident = LuaToString(l, -1);
			int upgrade_id = UpgradeIdByIdent(affix_ident);
			if (upgrade_id != -1) {
				item->Work = const_cast<CUpgrade *>(&(*AllUpgrades[upgrade_id]));
			} else {
				LuaError(l, "Literary work upgrade \"%s\" doesn't exist." _C_ affix_ident.c_str());
			}
		} else if (!strcmp(value, "Description")) {
			item->Description = LuaToString(l, -1);
		} else if (!strcmp(value, "Background")) {
			item->Background = LuaToString(l, -1);
		} else if (!strcmp(value, "Quote")) {
			item->Quote = LuaToString(l, -1);
		} else if (!strcmp(value, "ResourcesHeld")) {
			item->ResourcesHeld = LuaToNumber(l, -1);
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	return 0;
}

static int CclGetItems(lua_State *l)
{
	std::vector<CUnitType *> items;
	for (size_t i = 0; i < UnitTypes.size(); ++i) {
		if (UnitTypes[i]->BoolFlag[ITEM_INDEX].value) {
			items.push_back(UnitTypes[i]);
		}
	}
		
	lua_createtable(l, items.size(), 0);
	for (size_t i = 1; i <= items.size(); ++i)
	{
		lua_pushstring(l, items[i-1]->Ident.c_str());
		lua_rawseti(l, -2, i);
	}
	return 1;
}

static int CclGetUniqueItems(lua_State *l)
{
	lua_createtable(l, UniqueItems.size(), 0);
	for (size_t i = 1; i <= UniqueItems.size(); ++i)
	{
		lua_pushstring(l, UniqueItems[i-1]->Name.c_str());
		lua_rawseti(l, -2, i);
	}
	return 1;
}

/**
**  Get unique item data.
**
**  @param l  Lua state.
*/
static int CclGetUniqueItemData(lua_State *l)
{
	if (lua_gettop(l) < 2) {
		LuaError(l, "incorrect argument");
	}
	std::string item_name = LuaToString(l, 1);
	const CUniqueItem *item = GetUniqueItem(item_name);
	if (!item) {
		LuaError(l, "Unique item \"%s\" doesn't exist." _C_ item_name.c_str());
	}
	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Description")) {
		lua_pushstring(l, item->Description.c_str());
		return 1;
	} else if (!strcmp(data, "Background")) {
		lua_pushstring(l, item->Background.c_str());
		return 1;
	} else if (!strcmp(data, "Quote")) {
		lua_pushstring(l, item->Quote.c_str());
		return 1;
	} else if (!strcmp(data, "ResourcesHeld")) {
		lua_pushnumber(l, item->ResourcesHeld);
		return 1;
	} else if (!strcmp(data, "Type")) {
		if (item->Type != NULL) {
			lua_pushstring(l, item->Type->Ident.c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Prefix")) {
		if (item->Prefix != NULL) {
			lua_pushstring(l, item->Prefix->Ident.c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Suffix")) {
		if (item->Suffix != NULL) {
			lua_pushstring(l, item->Suffix->Ident.c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Spell")) {
		if (item->Spell != NULL) {
			lua_pushstring(l, item->Spell->Ident.c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Work")) {
		if (item->Work != NULL) {
			lua_pushstring(l, item->Work->Ident.c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Droppers")) { // unit types which can drop this one
		std::vector<CUnitType *> droppers;
		for (size_t i = 0; i < UnitTypes.size(); ++i) {
			if (
				std::find(UnitTypes[i]->Drops.begin(), UnitTypes[i]->Drops.end(), item->Type->Slot) != UnitTypes[i]->Drops.end()
				|| std::find(UnitTypes[i]->AiDrops.begin(), UnitTypes[i]->AiDrops.end(), item->Type->Slot) != UnitTypes[i]->AiDrops.end()
			) {
				if (
					(item->Prefix == NULL || std::find(UnitTypes[i]->DropAffixes.begin(), UnitTypes[i]->DropAffixes.end(), item->Prefix) != UnitTypes[i]->DropAffixes.end() || std::find(item->Type->Affixes.begin(), item->Type->Affixes.end(), item->Prefix) != item->Type->Affixes.end())
					&& (item->Suffix == NULL || std::find(UnitTypes[i]->DropAffixes.begin(), UnitTypes[i]->DropAffixes.end(), item->Suffix) != UnitTypes[i]->DropAffixes.end() || std::find(item->Type->Affixes.begin(), item->Type->Affixes.end(), item->Suffix) != item->Type->Affixes.end())
					&& (item->Spell == NULL || std::find(UnitTypes[i]->DropSpells.begin(), UnitTypes[i]->DropSpells.end(), item->Spell) != UnitTypes[i]->DropSpells.end())
					&& (item->Work == NULL || std::find(UnitTypes[i]->DropAffixes.begin(), UnitTypes[i]->DropAffixes.end(), item->Work) != UnitTypes[i]->DropAffixes.end() || std::find(item->Type->Affixes.begin(), item->Type->Affixes.end(), item->Work) != item->Type->Affixes.end())
				) {
					droppers.push_back(UnitTypes[i]);
				}
			}
		}
		
		lua_createtable(l, droppers.size(), 0);
		for (size_t i = 1; i <= droppers.size(); ++i)
		{
			lua_pushstring(l, droppers[i-1]->Ident.c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else {
		LuaError(l, "Invalid field: %s" _C_ data);
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
	lua_register(Lua, "GetItems", CclGetItems);
	lua_register(Lua, "GetUniqueItems", CclGetUniqueItems);
	lua_register(Lua, "GetUniqueItemData", CclGetUniqueItemData);
}

//@}
