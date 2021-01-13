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
//      (c) Copyright 2015-2021 by Andrettin
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

#include "stratagus.h"

#include "item/unique_item.h"
#include "player.h"
#include "script.h"
#include "spell/spell.h"
#include "unit/unit_type.h"
#include "upgrade/upgrade.h"

static int CclDefineUniqueItem(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string item_ident = LuaToString(l, 1);
	wyrmgus::unique_item *item = wyrmgus::unique_item::get_or_add(item_ident, nullptr);
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Name")) {
			item->set_name(LuaToString(l, -1));
		} else if (!strcmp(value, "Type")) {
			std::string unit_type_ident = LuaToString(l, -1);
			wyrmgus::unit_type *unit_type = wyrmgus::unit_type::get(unit_type_ident);
			item->unit_type = unit_type;
		} else if (!strcmp(value, "Icon")) {
			item->icon = wyrmgus::icon::get(LuaToString(l, -1));
		} else if (!strcmp(value, "Prefix")) {
			std::string affix_ident = LuaToString(l, -1);
			CUpgrade *upgrade = CUpgrade::get(affix_ident);
			item->prefix = upgrade;
		} else if (!strcmp(value, "Suffix")) {
			std::string affix_ident = LuaToString(l, -1);
			CUpgrade *upgrade = CUpgrade::get(affix_ident);
			item->suffix = upgrade;
		} else if (!strcmp(value, "Set")) {
			std::string set_ident = LuaToString(l, -1);
			CUpgrade *upgrade = CUpgrade::get(set_ident);
			item->set = upgrade;
			item->set->UniqueItems.push_back(item);
		} else if (!strcmp(value, "Spell")) {
			const std::string spell_ident = LuaToString(l, -1);
			wyrmgus::spell *spell = wyrmgus::spell::get(spell_ident);
			item->spell = spell;
		} else if (!strcmp(value, "Work")) {
			std::string upgrade_ident = LuaToString(l, -1);
			CUpgrade *upgrade = CUpgrade::get(upgrade_ident);
			item->work = upgrade;
		} else if (!strcmp(value, "Elixir")) {
			std::string upgrade_ident = LuaToString(l, -1);
			CUpgrade *upgrade = CUpgrade::get(upgrade_ident);
			item->elixir = upgrade;
		} else if (!strcmp(value, "Description")) {
			item->set_description(LuaToString(l, -1));
		} else if (!strcmp(value, "Background")) {
			item->set_background(LuaToString(l, -1));
		} else if (!strcmp(value, "Quote")) {
			item->set_quote(LuaToString(l, -1));
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
	std::vector<const wyrmgus::unit_type *> items;
	for (const wyrmgus::unit_type *unit_type : wyrmgus::unit_type::get_all()) {
		if (unit_type->is_template()) {
			continue;
		}

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
	lua_createtable(l, wyrmgus::unique_item::get_all().size(), 0);
	for (size_t i = 1; i <= wyrmgus::unique_item::get_all().size(); ++i)
	{
		lua_pushstring(l, wyrmgus::unique_item::get_all()[i-1]->get_identifier().c_str());
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
	const wyrmgus::unique_item *item = wyrmgus::unique_item::get(item_ident);

	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Name")) {
		lua_pushstring(l, item->get_name().c_str());
		return 1;
	} else if (!strcmp(data, "Description")) {
		lua_pushstring(l, item->get_description().c_str());
		return 1;
	} else if (!strcmp(data, "Background")) {
		lua_pushstring(l, item->get_background().c_str());
		return 1;
	} else if (!strcmp(data, "Quote")) {
		lua_pushstring(l, item->get_quote().c_str());
		return 1;
	} else if (!strcmp(data, "ResourcesHeld")) {
		lua_pushnumber(l, item->ResourcesHeld);
		return 1;
	} else if (!strcmp(data, "Type")) {
		if (item->get_unit_type() != nullptr) {
			lua_pushstring(l, item->get_unit_type()->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Prefix")) {
		if (item->get_prefix() != nullptr) {
			lua_pushstring(l, item->get_prefix()->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Suffix")) {
		if (item->get_suffix() != nullptr) {
			lua_pushstring(l, item->get_suffix()->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Set")) {
		if (item->get_set() != nullptr) {
			lua_pushstring(l, item->get_set()->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Spell")) {
		if (item->get_spell() != nullptr) {
			lua_pushstring(l, item->get_spell()->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Work")) {
		if (item->get_work() != nullptr) {
			lua_pushstring(l, item->get_work()->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Elixir")) {
		if (item->get_elixir() != nullptr) {
			lua_pushstring(l, item->get_elixir()->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "MagicLevel")) {
		lua_pushnumber(l, item->get_magic_level());
		return 1;
	} else if (!strcmp(data, "CanDrop")) {
		lua_pushboolean(l, item->can_drop());
		return 1;
	} else if (!strcmp(data, "Icon")) {
		if (item->get_icon() != nullptr) {
			lua_pushstring(l, item->get_icon()->get_identifier().c_str());
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
