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
//      (c) Copyright 2016-2019 by Andrettin
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

#include "world/province.h"

#include "civilization.h"
#include "faction.h"
#include "iolib.h"
#include "map/tileset.h"
#include "religion/deity_domain.h"
#include "script.h"
#include "species/species.h"
#include "time/season_schedule.h"
#include "time/time_of_day_schedule.h"
#include "unit/unit_type.h"
#include "upgrade/upgrade.h"
#include "video/video.h"
#include "world/plane.h"
#include "world/world.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

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
**  Define a region.
**
**  @param l  Lua state.
*/
static int CclDefineRegion(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string region_ident = LuaToString(l, 1);
	CRegion *region = GetRegion(region_ident);
	if (!region) {
		region = new CRegion;
		region->Ident = region_ident;
		region->ID = Regions.size();
		Regions.push_back(region);
	}
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Name")) {
			region->Name = LuaToString(l, -1);
		} else if (!strcmp(value, "HistoricalPopulation")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				int year = LuaToNumber(l, -1, j + 1);
				++j;
				int historical_population = LuaToNumber(l, -1, j + 1);
				region->HistoricalPopulation[year] = historical_population;
			}
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
			CWorld *world = CWorld::GetWorld(LuaToString(l, -1));
			if (world != nullptr) {
				province->World = world;
				world->Provinces.push_back(province);
			} else {
				LuaError(l, "World doesn't exist.");
			}
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
				CCivilization *civilization = CCivilization::Get(LuaToString(l, -1, j + 1));
				++j;
				if (!civilization) {
					continue;
				}

				std::string cultural_name = LuaToString(l, -1, j + 1);
				
				province->CulturalNames[civilization->GetIndex()] = TransliterateText(cultural_name);
			}
		} else if (!strcmp(value, "FactionCulturalNames")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				++j;

				const CFaction *faction = CFaction::Get(LuaToString(l, -1, j + 1));
				if (faction == nullptr) {
					LuaError(l, "Faction doesn't exist.");
				}
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
				
				CFaction *faction = CFaction::Get(LuaToString(l, -1, j + 1));
				if (faction == nullptr) {
					LuaError(l, "Faction doesn't exist.");
				}
				
				province->FactionClaims.push_back(faction);
			}
		} else if (!strcmp(value, "Regions")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				CRegion *region = GetRegion(LuaToString(l, -1, j + 1));
				if (region == nullptr) {
					LuaError(l, "Region doesn't exist.");
				}
				province->Regions.push_back(region);
				region->Provinces.push_back(province);
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
					CFaction *owner_faction = CFaction::Get(owner_faction_name);
					if (owner_faction == nullptr) {
						LuaError(l, "Faction \"%s\" doesn't exist." _C_ owner_faction_name.c_str());
					}
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
				CFaction *claimant_faction = CFaction::Get(claimant_faction_name);
				if (claimant_faction == nullptr) {
					LuaError(l, "Faction \"%s\" doesn't exist." _C_ claimant_faction_name.c_str());
				}
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
					CCivilization *historical_civilization = CCivilization::Get(historical_civilization_name);
					if (historical_civilization) {
						province->HistoricalCultures[year] = historical_civilization->GetIndex();
					}
				}
			}
		} else if (!strcmp(value, "HistoricalPopulation")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				int year = LuaToNumber(l, -1, j + 1);
				++j;
				int historical_population = LuaToNumber(l, -1, j + 1);
				province->HistoricalPopulation[year] = historical_population;
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
				CUpgrade *modifier = CUpgrade::Get(upgrade_ident);
				if (modifier == nullptr) {
					LuaError(l, "Upgrade \"%s\" doesn't exist." _C_ upgrade_ident.c_str());
				}
				++j;
				province->HistoricalModifiers[modifier][year] = LuaToBoolean(l, -1, j + 1);
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	if (province->World == nullptr) {
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
			CWorld *world = CWorld::GetWorld(LuaToString(l, -1));
			if (world != nullptr) {
				tile->World = world;
			} else {
				LuaError(l, "World doesn't exist.");
			}
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

				CCivilization *civilization = CCivilization::Get(LuaToString(l, -1, j + 1));
				++j;
				if (!civilization) {
					continue;
				}

				std::string cultural_name = LuaToString(l, -1, j + 1);
				
				tile->CulturalTerrainNames[std::pair<int,int>(terrain, civilization->GetIndex())].push_back(TransliterateText(cultural_name));
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

				const CFaction *faction = CFaction::Get(LuaToString(l, -1, j + 1));
				if (faction == nullptr) {
					LuaError(l, "Faction doesn't exist.");
				}
				++j;
				
				std::string cultural_name = LuaToString(l, -1, j + 1);
				
				tile->FactionCulturalTerrainNames[std::pair<int, const CFaction *>(terrain, faction)].push_back(TransliterateText(cultural_name));
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

				CCivilization *civilization = CCivilization::Get(LuaToString(l, -1, j + 1));
				++j;
				if (!civilization) {
					continue;
				}

				std::string cultural_name = LuaToString(l, -1, j + 1);
				
				tile->CulturalResourceNames[std::pair<int,int>(resource, civilization->GetIndex())].push_back(TransliterateText(cultural_name));
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
				
				const CFaction *faction = CFaction::Get(LuaToString(l, -1, j + 1));
				if (faction == nullptr) {
					LuaError(l, "Faction doesn't exist.");
				}
				++j;
				
				std::string cultural_name = LuaToString(l, -1, j + 1);
				
				tile->FactionCulturalResourceNames[std::pair<int, const CFaction *>(resource, faction)].push_back(TransliterateText(cultural_name));
			}
		} else if (!strcmp(value, "CulturalSettlementNames")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {

				CCivilization *civilization = CCivilization::Get(LuaToString(l, -1, j + 1));
				++j;
				if (!civilization) {
					continue;
				}

				std::string cultural_name = LuaToString(l, -1, j + 1);
				
				tile->CulturalSettlementNames[civilization->GetIndex()].push_back(TransliterateText(cultural_name));
			}
		} else if (!strcmp(value, "FactionCulturalSettlementNames")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				++j;
				
				const CFaction *faction = CFaction::Get(LuaToString(l, -1, j + 1));
				if (faction == nullptr) {
					LuaError(l, "Faction doesn't exist.");
				}
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
				
				CFaction *faction = CFaction::Get(LuaToString(l, -1, j + 1));
				if (faction == nullptr) {
					LuaError(l, "Faction doesn't exist.");
				}
				
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
					CFaction *owner_faction = CFaction::Get(owner_faction_name);
					if (owner_faction == nullptr) {
						LuaError(l, "Faction \"%s\" doesn't exist." _C_ owner_faction_name.c_str());
					}
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
				CFaction *claimant_faction = CFaction::Get(claimant_faction_name);
				if (claimant_faction == nullptr) {
					LuaError(l, "Faction \"%s\" doesn't exist." _C_ claimant_faction_name.c_str());
				}
				tile->HistoricalClaims[year] = claimant_faction;
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	if (tile->World == nullptr) {
		LuaError(l, "Tile (%d, %d) is not assigned to any world." _C_ tile->Position.x _C_ tile->Position.y);
	}
	
	return 0;
}

/**
**  Get plane data.
**
**  @param l  Lua state.
*/
static int CclGetPlaneData(lua_State *l)
{
	if (lua_gettop(l) < 2) {
		LuaError(l, "incorrect argument");
	}
	std::string plane_ident = LuaToString(l, 1);
	CPlane *plane = CPlane::GetPlane(plane_ident);
	if (!plane) {
		LuaError(l, "Plane \"%s\" doesn't exist." _C_ plane_ident.c_str());
	}
	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Name")) {
		lua_pushstring(l, plane->Name.c_str());
		return 1;
	} else if (!strcmp(data, "Index")) {
		lua_pushnumber(l, plane->GetIndex());
		return 1;
	} else if (!strcmp(data, "Description")) {
		lua_pushstring(l, plane->Description.c_str());
		return 1;
	} else if (!strcmp(data, "Background")) {
		lua_pushstring(l, plane->Background.c_str());
		return 1;
	} else if (!strcmp(data, "Quote")) {
		lua_pushstring(l, plane->Quote.c_str());
		return 1;
	} else if (!strcmp(data, "Species")) {
		lua_createtable(l, plane->Species.size(), 0);
		for (size_t i = 1; i <= plane->Species.size(); ++i)
		{
			lua_pushstring(l, plane->Species[i-1]->GetIdent().utf8().get_data());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else {
		LuaError(l, "Invalid field: %s" _C_ data);
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
	CWorld *world = CWorld::GetWorld(world_ident);
	if (!world) {
		LuaError(l, "World \"%s\" doesn't exist." _C_ world_ident.c_str());
	}
	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Name")) {
		lua_pushstring(l, world->Name.c_str());
		return 1;
	} else if (!strcmp(data, "Index")) {
		lua_pushnumber(l, world->GetIndex());
		return 1;
	} else if (!strcmp(data, "Description")) {
		lua_pushstring(l, world->Description.c_str());
		return 1;
	} else if (!strcmp(data, "Background")) {
		lua_pushstring(l, world->Background.c_str());
		return 1;
	} else if (!strcmp(data, "Quote")) {
		lua_pushstring(l, world->Quote.c_str());
		return 1;
	} else if (!strcmp(data, "Plane")) {
		if (world->Plane) {
			lua_pushstring(l, world->Plane->Ident.c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Provinces")) {
		lua_createtable(l, world->Provinces.size(), 0);
		for (size_t i = 1; i <= world->Provinces.size(); ++i)
		{
			lua_pushstring(l, world->Provinces[i-1]->Name.c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "Species")) {
		lua_createtable(l, world->Species.size(), 0);
		for (size_t i = 1; i <= world->Species.size(); ++i)
		{
			lua_pushstring(l, world->Species[i-1]->GetIdent().utf8().get_data());
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
		if (province->World != nullptr) {
			lua_pushstring(l, province->World->Ident.c_str());
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

static int CclGetPlanes(lua_State *l)
{
	lua_createtable(l, CPlane::Planes.size(), 0);
	for (size_t i = 1; i <= CPlane::Planes.size(); ++i)
	{
		lua_pushstring(l, CPlane::Planes[i-1]->Ident.c_str());
		lua_rawseti(l, -2, i);
	}
	return 1;
}

static int CclGetWorlds(lua_State *l)
{
	lua_createtable(l, CWorld::Worlds.size(), 0);
	for (size_t i = 1; i <= CWorld::Worlds.size(); ++i)
	{
		lua_pushstring(l, CWorld::Worlds[i-1]->Ident.c_str());
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
	lua_register(Lua, "DefineRegion", CclDefineRegion);
	lua_register(Lua, "DefineProvince", CclDefineProvince);
	lua_register(Lua, "DefineWorldMapTile", CclDefineWorldMapTile);
	lua_register(Lua, "GetPlaneData", CclGetPlaneData);
	lua_register(Lua, "GetWorldData", CclGetWorldData);
	lua_register(Lua, "GetProvinceData", CclGetProvinceData);
	lua_register(Lua, "GetPlanes", CclGetPlanes);
	lua_register(Lua, "GetWorlds", CclGetWorlds);
	lua_register(Lua, "GetProvinces", CclGetProvinces);
}
