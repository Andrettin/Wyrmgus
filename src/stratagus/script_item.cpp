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
//      (c) Copyright 2015-2019 by Andrettin
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

#include "item/item.h"

#include "item/unique_item.h"
#include "script.h"
#include "spell/spells.h"
#include "ui/icon.h"
#include "unit/unit_type.h"
#include "upgrade/upgrade.h"

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

	std::string item_ident = LuaToString(l, 1);
	UniqueItem *item = UniqueItem::GetOrAdd(item_ident);
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Name")) {
			item->Name = LuaToString(l, -1);
		} else if (!strcmp(value, "Type")) {
			std::string unit_type_ident = LuaToString(l, -1);
			CUnitType *unit_type = CUnitType::Get(unit_type_ident);
			if (unit_type != nullptr) {
				item->Type = unit_type;
			} else {
				LuaError(l, "Unit type \"%s\" doesn't exist." _C_ unit_type_ident.c_str());
			}
		} else if (!strcmp(value, "Icon")) {
			CIcon *icon = CIcon::Get(LuaToString(l, -1));
			item->Icon = icon;
			if (icon != nullptr) {
				icon->Load();
			}
		} else if (!strcmp(value, "Prefix")) {
			std::string affix_ident = LuaToString(l, -1);
			const CUpgrade *upgrade = CUpgrade::Get(affix_ident);
			if (upgrade != nullptr) {
				item->Prefix = upgrade;
			} else {
				LuaError(l, "Affix upgrade \"%s\" doesn't exist." _C_ affix_ident.c_str());
			}
		} else if (!strcmp(value, "Suffix")) {
			std::string affix_ident = LuaToString(l, -1);
			const CUpgrade *upgrade = CUpgrade::Get(affix_ident);
			if (upgrade != nullptr) {
				item->Suffix = upgrade;
			} else {
				LuaError(l, "Affix upgrade \"%s\" doesn't exist." _C_ affix_ident.c_str());
			}
		} else if (!strcmp(value, "Set")) {
			std::string set_ident = LuaToString(l, -1);
			CUpgrade *upgrade = CUpgrade::Get(set_ident);
			if (upgrade != nullptr) {
				item->Set = upgrade;
				item->Set->UniqueItems.push_back(item);
			} else {
				LuaError(l, "Set upgrade \"%s\" doesn't exist." _C_ set_ident.c_str());
			}
		} else if (!strcmp(value, "Spell")) {
			std::string spell_ident = LuaToString(l, -1);
			CSpell *spell = CSpell::GetSpell(spell_ident);
			if (spell != nullptr) {
				item->Spell = spell;
			} else {
				LuaError(l, "Spell \"%s\" doesn't exist." _C_ spell_ident.c_str());
			}
		} else if (!strcmp(value, "Work")) {
			std::string upgrade_ident = LuaToString(l, -1);
			const CUpgrade *upgrade = CUpgrade::Get(upgrade_ident);
			if (upgrade != nullptr) {
				item->Work = upgrade;
			} else {
				LuaError(l, "Literary work upgrade \"%s\" doesn't exist." _C_ upgrade_ident.c_str());
			}
		} else if (!strcmp(value, "Elixir")) {
			std::string upgrade_ident = LuaToString(l, -1);
			const CUpgrade *upgrade = CUpgrade::Get(upgrade_ident);
			if (upgrade != nullptr) {
				item->Elixir = upgrade;
			} else {
				LuaError(l, "Elixir upgrade \"%s\" doesn't exist." _C_ upgrade_ident.c_str());
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
	for (CUnitType *unit_type : CUnitType::GetAll()) {
		if (unit_type->BoolFlag[ITEM_INDEX].value) {
			items.push_back(unit_type);
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
	lua_createtable(l, UniqueItem::GetAll().size(), 0);
	for (size_t i = 1; i <= UniqueItem::GetAll().size(); ++i)
	{
		lua_pushstring(l, UniqueItem::Get(i-1)->Ident.c_str());
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
	std::string item_ident = LuaToString(l, 1);
	const UniqueItem *item = UniqueItem::Get(item_ident);
	if (!item) {
		LuaError(l, "Unique item \"%s\" doesn't exist." _C_ item_ident.c_str());
	}
	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Name")) {
		lua_pushstring(l, item->GetName().utf8().get_data());
		return 1;
	} else if (!strcmp(data, "Description")) {
		lua_pushstring(l, item->GetDescription().utf8().get_data());
		return 1;
	} else if (!strcmp(data, "Background")) {
		lua_pushstring(l, item->GetBackground().utf8().get_data());
		return 1;
	} else if (!strcmp(data, "Quote")) {
		lua_pushstring(l, item->GetQuote().utf8().get_data());
		return 1;
	} else if (!strcmp(data, "ResourcesHeld")) {
		lua_pushnumber(l, item->ResourcesHeld);
		return 1;
	} else if (!strcmp(data, "Type")) {
		if (item->Type != nullptr) {
			lua_pushstring(l, item->Type->Ident.c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Prefix")) {
		if (item->Prefix != nullptr) {
			lua_pushstring(l, item->Prefix->Ident.c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Suffix")) {
		if (item->Suffix != nullptr) {
			lua_pushstring(l, item->Suffix->Ident.c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Set")) {
		if (item->Set != nullptr) {
			lua_pushstring(l, item->Set->Ident.c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Spell")) {
		if (item->Spell != nullptr) {
			lua_pushstring(l, item->Spell->Ident.c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Work")) {
		if (item->Work != nullptr) {
			lua_pushstring(l, item->Work->Ident.c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Elixir")) {
		if (item->Elixir != nullptr) {
			lua_pushstring(l, item->Elixir->Ident.c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "MagicLevel")) {
		lua_pushnumber(l, item->GetMagicLevel());
		return 1;
	} else if (!strcmp(data, "CanDrop")) {
		lua_pushboolean(l, item->CanDrop());
		return 1;
	} else if (!strcmp(data, "Icon")) {
		if (item->GetIcon() != nullptr) {
			lua_pushstring(l, item->GetIcon()->Ident.c_str());
		} else {
			lua_pushstring(l, "");
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
