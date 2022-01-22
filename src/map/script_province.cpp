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
/**@name script_province.cpp - The province ccl functions. */
//
//      (c) Copyright 2016-2022 by Andrettin
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

#include "province.h"

#include "iolib.h"
#include "map/region.h"
#include "map/world.h"
#include "player/civilization.h"
#include "player/faction.h"
#include "player/player.h"
#include "script.h"
#include "species/species.h"
#include "unit/unit_type.h"
#include "util/util.h"
#include "upgrade/upgrade.h"
#include "video/video.h"

/**
**  Define a world map terrain type.
**
**  @param l  Lua state.
*/
static int CclDefineWorldMapTerrainType(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string terrain_name = LuaToString(l, 1);
	int terrain_id = GetWorldMapTerrainTypeId(terrain_name);
	CWorldMapTerrainType *terrain_type = nullptr;
	if (terrain_id == -1) {
		terrain_type = new CWorldMapTerrainType;
		terrain_type->Name = terrain_name;
		terrain_type->ID = WorldMapTerrainTypes.size();
		WorldMapTerrainTypes.push_back(terrain_type);
		WorldMapTerrainTypeStringToIndex[terrain_name] = terrain_type->ID;
	} else {
		terrain_type = WorldMapTerrainTypes[terrain_id];
	}
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Tag")) {
			terrain_type->Tag = LuaToString(l, -1);
		} else if (!strcmp(value, "HasTransitions")) {
			terrain_type->HasTransitions = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Water")) {
			terrain_type->Water = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "BaseTile")) {
			terrain_type->BaseTile = GetWorldMapTerrainTypeId(LuaToString(l, -1));
		} else if (!strcmp(value, "Variations")) {
			terrain_type->Variations = LuaToNumber(l, -1);
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	return 0;
}

/**
**  Define a province.
**
**  @param l  Lua state.
*/
static int CclDefineProvince(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string province_name = LuaToString(l, 1);
	CProvince *province = GetProvince(province_name);
	if (!province) {
		province = new CProvince;
		province->Name = province_name;
		province->ID = Provinces.size();
		Provinces.push_back(province);
	}
	
	std::string name_type = "province";
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "World")) {
			wyrmgus::world *world = wyrmgus::world::get(LuaToString(l, -1));
			province->world = world;
			world->Provinces.push_back(province);
		} else if (!strcmp(value, "Water")) {
			province->Water = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Coastal")) {
			province->Coastal = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "CulturalNames")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				wyrmgus::civilization *civilization = wyrmgus::civilization::get(LuaToString(l, -1, j + 1));
				++j;

				std::string cultural_name = LuaToString(l, -1, j + 1);
				
				province->CulturalNames[civilization->ID] = TransliterateText(cultural_name);
			}
		} else if (!strcmp(value, "FactionCulturalNames")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				++j;

				wyrmgus::faction *faction = wyrmgus::faction::get(LuaToString(l, -1, j + 1));
				++j;
				
				std::string cultural_name = LuaToString(l, -1, j + 1);
				
				province->FactionCulturalNames[faction] = TransliterateText(cultural_name);
			}
		} else if (!strcmp(value, "Claims")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				++j;
				
				wyrmgus::faction *faction = wyrmgus::faction::get(LuaToString(l, -1, j + 1));
				province->FactionClaims.push_back(faction);
			}
		} else if (!strcmp(value, "Regions")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				wyrmgus::region *region = wyrmgus::region::get(LuaToString(l, -1, j + 1));
				province->Regions.push_back(region);
			}
		} else if (!strcmp(value, "HistoricalOwners")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				int year = LuaToNumber(l, -1, j + 1);
				++j;
				++j;
				std::string owner_faction_name = LuaToString(l, -1, j + 1);
				if (!owner_faction_name.empty()) {
					wyrmgus::faction *owner_faction = wyrmgus::faction::get(owner_faction_name);
					province->HistoricalOwners[year] = owner_faction;
				} else {
					province->HistoricalOwners[year] = nullptr;
				}
			}
		} else if (!strcmp(value, "HistoricalClaims")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				int year = LuaToNumber(l, -1, j + 1);
				++j;
				++j;
				std::string claimant_faction_name = LuaToString(l, -1, j + 1);
				wyrmgus::faction *claimant_faction = wyrmgus::faction::get(claimant_faction_name);
				province->HistoricalClaims[year] = claimant_faction;
			}
		} else if (!strcmp(value, "HistoricalCultures")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				int year = LuaToNumber(l, -1, j + 1);
				++j;
				std::string historical_civilization_name = LuaToString(l, -1, j + 1);
				if (!historical_civilization_name.empty()) {
					wyrmgus::civilization *historical_civilization = wyrmgus::civilization::get(historical_civilization_name);
					province->HistoricalCultures[year] = historical_civilization->ID;
				}
			}
		} else if (!strcmp(value, "HistoricalSettlementBuildings")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				int year = LuaToNumber(l, -1, j + 1);
				++j;
				std::string building_type_ident = LuaToString(l, -1, j + 1);
				int building_type = UnitTypeIdByIdent(building_type_ident);
				if (building_type == -1) {
					LuaError(l, "Unit type \"%s\" doesn't exist." _C_ building_type_ident.c_str());
				}
				++j;
				province->HistoricalSettlementBuildings[building_type][year] = LuaToBoolean(l, -1, j + 1);
			}
		} else if (!strcmp(value, "HistoricalModifiers")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				int year = LuaToNumber(l, -1, j + 1);
				++j;
				std::string upgrade_ident = LuaToString(l, -1, j + 1);
				CUpgrade *modifier = CUpgrade::get(upgrade_ident);
				++j;
				province->HistoricalModifiers[modifier][year] = LuaToBoolean(l, -1, j + 1);
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	if (province->world == nullptr) {
		LuaError(l, "Province \"%s\" is not assigned to any world." _C_ province->Name.c_str());
	}
	
	return 0;
}

/**
**  Define a world map tile.
**
**  @param l  Lua state.
*/
static int CclDefineWorldMapTile(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::pair<int,int> tile_position;
	CclGetPos(l, &tile_position.first, &tile_position.second, 1);
					
	WorldMapTile *tile = new WorldMapTile;
	tile->Position.x = tile_position.first;
	tile->Position.y = tile_position.second;
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "World")) {
			wyrmgus::world *world = wyrmgus::world::get(LuaToString(l, -1));
			tile->world = world;
		} else if (!strcmp(value, "Terrain")) {
			int terrain = GetWorldMapTerrainTypeId(LuaToString(l, -1));
			if (terrain != -1) {
				tile->Terrain = terrain;
			} else {
				LuaError(l, "Terrain doesn't exist.");
			}
		} else if (!strcmp(value, "Resource")) {
			int resource = GetResourceIdByName(LuaToString(l, -1));
			if (resource != -1) {
				tile->Resource = resource;
			} else {
				LuaError(l, "Resource doesn't exist.");
			}
		} else if (!strcmp(value, "Capital")) {
			tile->Capital = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "CulturalTerrainNames")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				int terrain = GetWorldMapTerrainTypeId(LuaToString(l, -1, j + 1));
				if (terrain == -1) {
					LuaError(l, "Terrain doesn't exist.");
				}
				++j;
				
				std::string name_type = "terrain-" + NameToIdent(WorldMapTerrainTypes[terrain]->Name);

				wyrmgus::civilization *civilization = wyrmgus::civilization::get(LuaToString(l, -1, j + 1));
				++j;

				std::string cultural_name = LuaToString(l, -1, j + 1);
				
				tile->CulturalTerrainNames[std::pair<int,int>(terrain, civilization->ID)].push_back(TransliterateText(cultural_name));
			}
		} else if (!strcmp(value, "FactionCulturalTerrainNames")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				int terrain = GetWorldMapTerrainTypeId(LuaToString(l, -1, j + 1));
				if (terrain == -1) {
					LuaError(l, "Terrain doesn't exist.");
				}
				++j;
				
				std::string name_type = "terrain-" + NameToIdent(WorldMapTerrainTypes[terrain]->Name);

				++j;

				wyrmgus::faction *faction = wyrmgus::faction::get(LuaToString(l, -1, j + 1));
				++j;
				
				std::string cultural_name = LuaToString(l, -1, j + 1);
				
				tile->FactionCulturalTerrainNames[std::pair<int, wyrmgus::faction *>(terrain, faction)].push_back(TransliterateText(cultural_name));
			}
		} else if (!strcmp(value, "CulturalResourceNames")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				int resource = GetResourceIdByName(LuaToString(l, -1, j + 1));
				if (resource == -1) {
					LuaError(l, "Resource doesn't exist.");
				}
				++j;
				
				std::string name_type = "resource-tile-" + DefaultResourceNames[resource];

				wyrmgus::civilization *civilization = wyrmgus::civilization::get(LuaToString(l, -1, j + 1));
				++j;

				std::string cultural_name = LuaToString(l, -1, j + 1);
				
				tile->CulturalResourceNames[std::pair<int,int>(resource, civilization->ID)].push_back(TransliterateText(cultural_name));
			}
		} else if (!strcmp(value, "FactionCulturalResourceNames")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				int resource = GetResourceIdByName(LuaToString(l, -1, j + 1));
				if (resource == -1) {
					LuaError(l, "Resource doesn't exist.");
				}
				++j;
				
				std::string name_type = "resource-tile-" + DefaultResourceNames[resource];

				++j;
				
				wyrmgus::faction *faction = wyrmgus::faction::get(LuaToString(l, -1, j + 1));
				++j;
				
				std::string cultural_name = LuaToString(l, -1, j + 1);
				
				tile->FactionCulturalResourceNames[std::pair<int, wyrmgus::faction *>(resource, faction)].push_back(TransliterateText(cultural_name));
			}
		} else if (!strcmp(value, "CulturalSettlementNames")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {

				wyrmgus::civilization *civilization = wyrmgus::civilization::get(LuaToString(l, -1, j + 1));
				++j;

				std::string cultural_name = LuaToString(l, -1, j + 1);
				
				tile->CulturalSettlementNames[civilization->ID].push_back(TransliterateText(cultural_name));
			}
		} else if (!strcmp(value, "FactionCulturalSettlementNames")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				++j;
				
				wyrmgus::faction *faction = wyrmgus::faction::get(LuaToString(l, -1, j + 1));
				++j;
				
				std::string cultural_name = LuaToString(l, -1, j + 1);
				
				tile->FactionCulturalSettlementNames[faction].push_back(TransliterateText(cultural_name));
			}
		} else if (!strcmp(value, "Claims")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				++j;
				
				wyrmgus::faction *faction = wyrmgus::faction::get(LuaToString(l, -1, j + 1));
				tile->FactionClaims.push_back(faction);
			}
		} else if (!strcmp(value, "HistoricalOwners")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				int year = LuaToNumber(l, -1, j + 1);
				++j;
				++j;
				std::string owner_faction_name = LuaToString(l, -1, j + 1);
				if (!owner_faction_name.empty()) {
					wyrmgus::faction *owner_faction = wyrmgus::faction::get(owner_faction_name);
					tile->HistoricalOwners[year] = owner_faction;
				} else {
					tile->HistoricalOwners[year] = nullptr;
				}
			}
		} else if (!strcmp(value, "HistoricalClaims")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				int year = LuaToNumber(l, -1, j + 1);
				++j;
				++j;
				std::string claimant_faction_name = LuaToString(l, -1, j + 1);
				wyrmgus::faction *claimant_faction = wyrmgus::faction::get(claimant_faction_name);
				tile->HistoricalClaims[year] = claimant_faction;
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	if (tile->world == nullptr) {
		LuaError(l, "Tile (%d, %d) is not assigned to any world." _C_ tile->Position.x _C_ tile->Position.y);
	}
	
	return 0;
}

/**
**  Get world data.
**
**  @param l  Lua state.
*/
static int CclGetWorldData(lua_State *l)
{
	if (lua_gettop(l) < 2) {
		LuaError(l, "incorrect argument");
	}
	std::string world_ident = LuaToString(l, 1);
	const wyrmgus::world *world = wyrmgus::world::get(world_ident);
	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Name")) {
		lua_pushstring(l, world->get_name().c_str());
		return 1;
	} else if (!strcmp(data, "ID")) {
		lua_pushnumber(l, world->ID);
		return 1;
	} else if (!strcmp(data, "Description")) {
		lua_pushstring(l, world->get_description().c_str());
		return 1;
	} else if (!strcmp(data, "Background")) {
		lua_pushstring(l, world->get_background().c_str());
		return 1;
	} else if (!strcmp(data, "Quote")) {
		lua_pushstring(l, world->get_quote().c_str());
		return 1;
	} else if (!strcmp(data, "Provinces")) {
		lua_createtable(l, world->Provinces.size(), 0);
		for (size_t i = 1; i <= world->Provinces.size(); ++i)
		{
			lua_pushstring(l, world->Provinces[i-1]->Name.c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "NativeSpecies")) {
		lua_createtable(l, world->get_native_species().size(), 0);
		for (size_t i = 1; i <= world->get_native_species().size(); ++i)
		{
			lua_pushstring(l, world->get_native_species()[i-1]->get_identifier().c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "NativeSapientSpeciesNames")) {
		const std::vector<std::string> species_names = wyrmgus::species::get_name_list(world->get_native_sapient_species());
		lua_createtable(l, species_names.size(), 0);
		for (size_t i = 1; i <= species_names.size(); ++i)
		{
			lua_pushstring(l, species_names[i - 1].c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "NativeFaunaSpeciesNames")) {
		const std::vector<std::string> species_names = wyrmgus::species::get_name_list(world->get_native_fauna_species());
		lua_createtable(l, species_names.size(), 0);
		for (size_t i = 1; i <= species_names.size(); ++i)
		{
			lua_pushstring(l, species_names[i - 1].c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else {
		LuaError(l, "Invalid field: %s" _C_ data);
	}

	return 0;
}

/**
**  Get province data.
**
**  @param l  Lua state.
*/
static int CclGetProvinceData(lua_State *l)
{
	if (lua_gettop(l) < 2) {
		LuaError(l, "incorrect argument");
	}
	std::string province_name = LuaToString(l, 1);
	CProvince *province = GetProvince(province_name);
	if (!province) {
		LuaError(l, "Province \"%s\" doesn't exist." _C_ province_name.c_str());
	}
	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Name")) {
		lua_pushstring(l, province->Name.c_str());
		return 1;
	} else if (!strcmp(data, "World")) {
		if (province->world != nullptr) {
			lua_pushstring(l, province->world->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Water")) {
		lua_pushboolean(l, province->Water);
		return 1;
	} else if (!strcmp(data, "Coastal")) {
		lua_pushboolean(l, province->Coastal);
		return 1;
	} else {
		LuaError(l, "Invalid field: %s" _C_ data);
	}

	return 0;
}

static int CclGetWorlds(lua_State *l)
{
	lua_createtable(l, wyrmgus::world::get_all().size(), 0);
	for (size_t i = 1; i <= wyrmgus::world::get_all().size(); ++i)
	{
		lua_pushstring(l, wyrmgus::world::get_all()[i-1]->get_identifier().c_str());
		lua_rawseti(l, -2, i);
	}
	return 1;
}

static int CclGetProvinces(lua_State *l)
{
	lua_createtable(l, Provinces.size(), 0);
	for (size_t i = 1; i <= Provinces.size(); ++i)
	{
		lua_pushstring(l, Provinces[i-1]->Name.c_str());
		lua_rawseti(l, -2, i);
	}
	return 1;
}

// ----------------------------------------------------------------------------

/**
**  Register CCL features for provinces.
*/
void ProvinceCclRegister()
{
	lua_register(Lua, "DefineWorldMapTerrainType", CclDefineWorldMapTerrainType);
	lua_register(Lua, "DefineProvince", CclDefineProvince);
	lua_register(Lua, "DefineWorldMapTile", CclDefineWorldMapTile);
	lua_register(Lua, "GetWorldData", CclGetWorldData);
	lua_register(Lua, "GetProvinceData", CclGetProvinceData);
	lua_register(Lua, "GetWorlds", CclGetWorlds);
	lua_register(Lua, "GetProvinces", CclGetProvinces);
}
