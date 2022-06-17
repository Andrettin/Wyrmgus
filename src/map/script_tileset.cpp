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
/**@name script_tileset.cpp - The tileset ccl functions. */
//
//      (c) Copyright 2000-2007 by Lutz Sammer, Francois Beerten and Jimmy Salmon
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

#include "map/tileset.h"

#include "map/tile_flag.h"
#include "script.h"

static void ModifyFlag(const std::string &flag_name, tile_flag *flag)
{
	*flag |= string_to_tile_flag(flag_name);
}

/**
**  Parse the flag section of a tile definition.
**
**  @param l     Lua state.
**  @param back  pointer for the flags (return).
**  @param j     pointer for the location in the array. in and out
**
*/
void ParseTilesetTileFlags(lua_State *l, tile_flag *back, int *j)
{
	tile_flag flags = static_cast<tile_flag>(3);

	//  Parse the list: flags of the slot
	while (1) {
		lua_rawgeti(l, -1, *j + 1);
		if (!lua_isstring(l, -1)) {
			lua_pop(l, 1);
			break;
		}
		++(*j);
		const char *value = LuaToString(l, -1);
		lua_pop(l, 1);

		//  Flags are only needed for the editor
		ModifyFlag(value, &flags);
	}
	*back = flags;
}

/**
**  Parse the special slot part of a tileset definition
**
**  @param l        Lua state.
*/
void tileset::parseSpecial(lua_State *l)
{
	if (!lua_istable(l, -1)) {
		LuaError(l, "incorrect argument");
	}
	const int args = lua_rawlen(l, -1);
	
	for (int j = 0; j < args; ++j) {
		const char *value = LuaToString(l, -1, j + 1);

		if (!strcmp(value, "top-one-tree")) {
			++j;
		} else if (!strcmp(value, "mid-one-tree")) {
			++j;
		} else if (!strcmp(value, "bot-one-tree")) {
			++j;
		} else if (!strcmp(value, "removed-tree")) {
			++j;
		} else if (!strcmp(value, "growing-tree")) {
			++j;
			// keep for retro compatibility.
			// TODO : remove when game data are updated.
		} else if (!strcmp(value, "top-one-rock")) {
			++j;
		} else if (!strcmp(value, "mid-one-rock")) {
			++j;
		} else if (!strcmp(value, "bot-one-rock")) {
			++j;
		} else if (!strcmp(value, "removed-rock")) {
			++j;
		} else {
			LuaError(l, "special: unsupported tag: %s" _C_ value);
		}
	}
}

/**
**  Parse the solid slot part of a tileset definition
**
**  @param l        Lua state.
*/
void tileset::parseSolid(lua_State *l)
{
	const int index = tiles.size();

	this->tiles.resize(index + 16);
	if (!lua_istable(l, -1)) {
		LuaError(l, "incorrect argument");
	}

	int j = 0;
	const int basic_name = getOrAddSolidTileIndexByName(LuaToString(l, -1, j + 1));
	++j;

	tile_flag f = tile_flag::none;
	ParseTilesetTileFlags(l, &f, &j);
	//  Vector: the tiles.
	lua_rawgeti(l, -1, j + 1);
	if (!lua_istable(l, -1)) {
		LuaError(l, "incorrect argument");
	}
	const int len = lua_rawlen(l, -1);

	j = 0;
	for (int i = 0; i < len; ++i, ++j) {
		lua_rawgeti(l, -1, i + 1);
		if (lua_istable(l, -1)) {
			int k = 0;
			tile_flag tile_flag = tile_flag::none;
			ParseTilesetTileFlags(l, &tile_flag, &k);
			--j;
			lua_pop(l, 1);
			tiles[index + j].flag = tile_flag;
			continue;
		}
		const int pud = LuaToNumber(l, -1);
		lua_pop(l, 1);

		// ugly hack for sc tilesets, remove when fixed
		if (j > 15) {
			this->tiles.resize(index + j);
		}
		CTile &tile = tiles[index + j];

		tile.tile = pud;
		tile.flag = f;
		tile.tileinfo.BaseTerrain = basic_name;
		tile.tileinfo.MixTerrain = 0;
	}
	lua_pop(l, 1);
}

/**
**  Parse the mixed slot part of a tileset definition
**
**  @param l        Lua state.
*/
void tileset::parseMixed(lua_State *l)
{
	int index = tiles.size();
	tiles.resize(index + 256);

	if (!lua_istable(l, -1)) {
		LuaError(l, "incorrect argument");
	}
	int j = 0;
	const int args = lua_rawlen(l, -1);
	const int basic_name = getOrAddSolidTileIndexByName(LuaToString(l, -1, j + 1));
	++j;
	const int mixed_name = getOrAddSolidTileIndexByName(LuaToString(l, -1, j + 1));
	++j;

	tile_flag f = tile_flag::none;
	ParseTilesetTileFlags(l, &f, &j);

	for (; j < args; ++j) {
		lua_rawgeti(l, -1, j + 1);
		if (!lua_istable(l, -1)) {
			LuaError(l, "incorrect argument");
		}
		//  Vector: the tiles.
		const int len = lua_rawlen(l, -1);
		for (int i = 0; i < len; ++i) {
			const int pud = LuaToNumber(l, -1, i + 1);
			CTile &tile = tiles[index + i];

			tile.tile = pud;
			tile.flag = f;
			tile.tileinfo.BaseTerrain = basic_name;
			tile.tileinfo.MixTerrain = mixed_name;
		}
		index += 16;
		lua_pop(l, 1);
	}
}

/**
**  Parse the slot part of a tileset definition
**
**  @param l        Lua state.
**  @param t        FIXME: docu
*/
void tileset::parseSlots(lua_State *l, int t)
{
	tiles.clear();

	//  Parse the list: (still everything could be changed!)
	const int args = lua_rawlen(l, t);
	for (int j = 0; j < args; ++j) {
		const char *value = LuaToString(l, t, j + 1);
		++j;

		if (!strcmp(value, "special")) {
			lua_rawgeti(l, t, j + 1);
			parseSpecial(l);
			lua_pop(l, 1);
		} else if (!strcmp(value, "solid")) {
			lua_rawgeti(l, t, j + 1);
			parseSolid(l);
			lua_pop(l, 1);
		} else if (!strcmp(value, "mixed")) {
			lua_rawgeti(l, t, j + 1);
			parseMixed(l);
			lua_pop(l, 1);
		} else {
			LuaError(l, "slots: unsupported tag: %s" _C_ value);
		}
	}
}

void tileset::parse(lua_State *l)
{
	const int args = lua_gettop(l);
	for (int j = 1; j < args; ++j) {
		const char *value = LuaToString(l, j);
		++j;

		if (!strcmp(value, "name")) {
		} else if (!strcmp(value, "ident")) {
			this->Ident = LuaToString(l, j);
		} else if (!strcmp(value, "image")) {
		} else if (!strcmp(value, "slots")) {
			if (!lua_istable(l, j)) {
				LuaError(l, "incorrect argument");
			}
			parseSlots(l, j);
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
}

void tileset::buildTable()
{
	//Wyrmgus start
	for (size_t i = 0; i != this->solidTerrainTypes.size(); ++i) {
		int default_tile_index = findTileIndex(static_cast<unsigned char>(i), 0);
		if (default_tile_index != -1) {
			this->solidTerrainTypes[i].DefaultTileIndex = default_tile_index;
		}
	}
	//Wyrmgus end
}
