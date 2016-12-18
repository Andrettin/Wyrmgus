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

#include "province.h"

#include "iolib.h"
#include "player.h"
#include "script.h"
#include "tileset.h"
#include "unittype.h"
#include "upgrade.h"
#include "video.h"

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
	CWorldMapTerrainType *terrain_type = NULL;
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
**  Define a plane.
**
**  @param l  Lua state.
*/
static int CclDefinePlane(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string plane_name = LuaToString(l, 1);
	CPlane *plane = GetPlane(plane_name);
	if (!plane) {
		plane = new CPlane;
		plane->Name = plane_name;
		plane->ID = Planes.size();
		Planes.push_back(plane);
	}
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Description")) {
			plane->Description = LuaToString(l, -1);
		} else if (!strcmp(value, "Background")) {
			plane->Background = LuaToString(l, -1);
		} else if (!strcmp(value, "Quote")) {
			plane->Quote = LuaToString(l, -1);
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	return 0;
}

/**
**  Define a world.
**
**  @param l  Lua state.
*/
static int CclDefineWorld(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string world_name = LuaToString(l, 1);
	CWorld *world = GetWorld(world_name);
	if (!world) {
		world = new CWorld;
		world->Name = world_name;
		world->ID = Worlds.size();
		Worlds.push_back(world);
	}
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Description")) {
			world->Description = LuaToString(l, -1);
		} else if (!strcmp(value, "Background")) {
			world->Background = LuaToString(l, -1);
		} else if (!strcmp(value, "Quote")) {
			world->Quote = LuaToString(l, -1);
		} else if (!strcmp(value, "BaseTerrain")) {
			int base_terrain_id = GetWorldMapTerrainTypeId(LuaToString(l, -1));
			if (base_terrain_id == -1) {
				LuaError(l, "Terrain doesn't exist.");
			}
			world->BaseTerrain = WorldMapTerrainTypes[base_terrain_id];
		} else if (!strcmp(value, "Plane")) {
			CPlane *plane = GetPlane(LuaToString(l, -1));
			if (!plane) {
				LuaError(l, "Plane doesn't exist.");
			}
			world->Plane = plane;
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

	std::string region_name = LuaToString(l, 1);
	CRegion *region = GetRegion(region_name);
	if (!region) {
		region = new CRegion;
		region->Name = region_name;
		region->ID = Regions.size();
		Regions.push_back(region);
	}
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "HistoricalPopulation")) {
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
			CWorld *world = GetWorld(LuaToString(l, -1));
			if (world != NULL) {
				province->World = world;
				world->Provinces.push_back(province);
			} else {
				LuaError(l, "World doesn't exist.");
			}
		} else if (!strcmp(value, "Water")) {
			province->Water = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Coastal")) {
			province->Coastal = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "SettlementLocation")) {
			CclGetPos(l, &province->SettlementLocation.x, &province->SettlementLocation.y);
		} else if (!strcmp(value, "Map")) {
			province->Map = LuaToString(l, -1);
		} else if (!strcmp(value, "SettlementTerrain")) {
			province->SettlementTerrain = LuaToString(l, -1);
		} else if (!strcmp(value, "ReferenceProvince")) {
			CProvince *reference_province = GetProvince(LuaToString(l, -1));
			if (reference_province != NULL) {
				province->ReferenceProvince = reference_province->ID;
			} else {
				LuaError(l, "Reference province doesn't exist.");
			}
		} else if (!strcmp(value, "CulturalNames")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				int civilization = PlayerRaces.GetRaceIndexByName(LuaToString(l, -1, j + 1));
				if (civilization == -1) {
					LuaError(l, "Civilization doesn't exist.");
				}
				++j;
				
				std::string cultural_name = LuaToString(l, -1, j + 1);
				
				province->CulturalNames[civilization] = TransliterateText(cultural_name);

				int language = PlayerRaces.GetCivilizationLanguage(civilization);
				if (language != -1) {
					if (PlayerRaces.Languages[language]->TypeNameCount.find(name_type) == PlayerRaces.Languages[language]->TypeNameCount.end()) {
						PlayerRaces.Languages[language]->TypeNameCount[name_type] = 0;
					}
					PlayerRaces.Languages[language]->TypeNameCount[name_type] += 1;
				}
				
				++j;
				if (j >= subargs) {
					break;
				}
				
				std::string next_element = LuaToString(l, -1, j + 1);
				if (next_element == "name-elements") {
					++j;
					lua_rawgeti(l, -1, j + 1);
					if (lua_istable(l, -1)) {
						ParseNameElements(l, name_type);
					}
					lua_pop(l, 1);
				} else if (next_element == "settlement-derived-name") {
					PlayerRaces.Languages[language]->SettlementDerivedProvinceNameCount += 1;
				} else {
					--j;
				}
			}
		} else if (!strcmp(value, "FactionCulturalNames")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				int civilization = PlayerRaces.GetRaceIndexByName(LuaToString(l, -1, j + 1));
				if (civilization == -1) {
					LuaError(l, "Civilization doesn't exist.");
				}
				++j;
				
				int faction = PlayerRaces.GetFactionIndexByName(civilization, LuaToString(l, -1, j + 1));
				if (faction == -1) {
					LuaError(l, "Faction doesn't exist.");
				}
				++j;
				
				std::string cultural_name = LuaToString(l, -1, j + 1);
				
				province->FactionCulturalNames[PlayerRaces.Factions[civilization][faction]] = TransliterateText(cultural_name);
				
				int language = PlayerRaces.GetFactionLanguage(civilization, faction);
				if (language == -1) {
					LuaError(l, "Language doesn't exist.");
				}
				if (PlayerRaces.Languages[language]->TypeNameCount.find(name_type) == PlayerRaces.Languages[language]->TypeNameCount.end()) {
					PlayerRaces.Languages[language]->TypeNameCount[name_type] = 0;
				}
				PlayerRaces.Languages[language]->TypeNameCount[name_type] += 1;
				
				++j;
				if (j >= subargs) {
					break;
				}
				
				std::string next_element = LuaToString(l, -1, j + 1);
				if (next_element == "name-elements") {
					++j;
					lua_rawgeti(l, -1, j + 1);
					if (lua_istable(l, -1)) {
						ParseNameElements(l, name_type);
					}
					lua_pop(l, 1);
				} else if (next_element == "settlement-derived-name") {
					PlayerRaces.Languages[language]->SettlementDerivedProvinceNameCount += 1;
				} else {
					--j;
				}
			}
		} else if (!strcmp(value, "Tiles")) {
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				lua_rawgeti(l, -1, j + 1);
				if (!lua_istable(l, -1)) {
					LuaError(l, "incorrect argument (expected table)");
				}
				Vec2i tile;
				CclGetPos(l, &tile.x, &tile.y);
				province->Tiles.push_back(tile);
				lua_pop(l, 1);
			}
		} else if (!strcmp(value, "Claims")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				int civilization = PlayerRaces.GetRaceIndexByName(LuaToString(l, -1, j + 1));
				if (civilization == -1) {
					LuaError(l, "Civilization doesn't exist.");
				}
				++j;
				
				int faction = PlayerRaces.GetFactionIndexByName(civilization, LuaToString(l, -1, j + 1));
				if (faction == -1) {
					LuaError(l, "Faction doesn't exist.");
				}
				
				province->FactionClaims.push_back(PlayerRaces.Factions[civilization][faction]);
			}
		} else if (!strcmp(value, "Regions")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				CRegion *region = GetRegion(LuaToString(l, -1, j + 1));
				if (region == NULL) {
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
				std::string owner_civilization_name = LuaToString(l, -1, j + 1);
				++j;
				std::string owner_faction_name = LuaToString(l, -1, j + 1);
				if (!owner_civilization_name.empty() && !owner_faction_name.empty()) {
					int owner_civilization = PlayerRaces.GetRaceIndexByName(owner_civilization_name.c_str());
					int owner_faction = PlayerRaces.GetFactionIndexByName(owner_civilization, owner_faction_name);
					if (owner_faction == -1) {
						LuaError(l, "Faction \"%s\" doesn't exist." _C_ owner_faction_name.c_str());
					}
					province->HistoricalOwners[year] = PlayerRaces.Factions[owner_civilization][owner_faction];
				} else {
					province->HistoricalOwners[year] = NULL;
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
				std::string claimant_civilization_name = LuaToString(l, -1, j + 1);
				int claimant_civilization = PlayerRaces.GetRaceIndexByName(claimant_civilization_name.c_str());
				++j;
				std::string claimant_faction_name = LuaToString(l, -1, j + 1);
				int claimant_faction = PlayerRaces.GetFactionIndexByName(claimant_civilization, claimant_faction_name);
				if (claimant_faction == -1) {
					LuaError(l, "Faction \"%s\" doesn't exist." _C_ claimant_faction_name.c_str());
				}
				province->HistoricalClaims[year] = PlayerRaces.Factions[claimant_civilization][claimant_faction];
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
				int historical_civilization = PlayerRaces.GetRaceIndexByName(historical_civilization_name.c_str());
				if (historical_civilization == -1 && !historical_civilization_name.empty()) {
					LuaError(l, "Civilization \"%s\" doesn't exist." _C_ historical_civilization_name.c_str());
				}
				province->HistoricalCultures[year] = historical_civilization;
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
				if (modifier == NULL) {
					LuaError(l, "Upgrade \"%s\" doesn't exist." _C_ upgrade_ident.c_str());
				}
				++j;
				province->HistoricalModifiers[modifier][year] = LuaToBoolean(l, -1, j + 1);
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	if (province->World == NULL) {
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
			CWorld *world = GetWorld(LuaToString(l, -1));
			if (world != NULL) {
				tile->World = world;
				world->Tiles.push_back(tile);
			} else {
				LuaError(l, "World doesn't exist.");
			}
		} else if (!strcmp(value, "Province")) {
			CProvince *province = GetProvince(LuaToString(l, -1));
			if (province != NULL) {
				tile->Province = province;
				if (tile->Position.x != -1 && tile->Position.y != -1) {
					province->Tiles.push_back(tile->Position);
				}
			} else {
				LuaError(l, "Province doesn't exist.");
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

				int civilization = PlayerRaces.GetRaceIndexByName(LuaToString(l, -1, j + 1));
				if (civilization == -1) {
					LuaError(l, "Civilization doesn't exist.");
				}
				++j;
				
				std::string cultural_name = LuaToString(l, -1, j + 1);
				
				tile->CulturalTerrainNames[std::pair<int,int>(terrain, civilization)].push_back(TransliterateText(cultural_name));
				
				int language = PlayerRaces.GetCivilizationLanguage(civilization);
				if (language == -1) {
					LuaError(l, "Language doesn't exist.");
				}
				if (PlayerRaces.Languages[language]->TypeNameCount.find(name_type) == PlayerRaces.Languages[language]->TypeNameCount.end()) {
					PlayerRaces.Languages[language]->TypeNameCount[name_type] = 0;
				}
				PlayerRaces.Languages[language]->TypeNameCount[name_type] += 1;
				
				++j;
				if (j >= subargs) {
					break;
				}
				
				std::string next_element = LuaToString(l, -1, j + 1);
				if (next_element == "name-elements") {
					++j;
					lua_rawgeti(l, -1, j + 1);
					if (lua_istable(l, -1)) {
						ParseNameElements(l, name_type);
					}
					lua_pop(l, 1);
				} else {
					--j;
				}
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

				int civilization = PlayerRaces.GetRaceIndexByName(LuaToString(l, -1, j + 1));
				if (civilization == -1) {
					LuaError(l, "Civilization doesn't exist.");
				}
				++j;
				
				int faction = PlayerRaces.GetFactionIndexByName(civilization, LuaToString(l, -1, j + 1));
				if (faction == -1) {
					LuaError(l, "Faction doesn't exist.");
				}
				++j;
				
				std::string cultural_name = LuaToString(l, -1, j + 1);
				
				tile->FactionCulturalTerrainNames[std::pair<int,CFaction *>(terrain, PlayerRaces.Factions[civilization][faction])].push_back(TransliterateText(cultural_name));
				
				int language = PlayerRaces.GetFactionLanguage(civilization, faction);
				if (language == -1) {
					LuaError(l, "Language doesn't exist.");
				}
				if (PlayerRaces.Languages[language]->TypeNameCount.find(name_type) == PlayerRaces.Languages[language]->TypeNameCount.end()) {
					PlayerRaces.Languages[language]->TypeNameCount[name_type] = 0;
				}
				PlayerRaces.Languages[language]->TypeNameCount[name_type] += 1;
				
				++j;
				if (j >= subargs) {
					break;
				}
				
				std::string next_element = LuaToString(l, -1, j + 1);
				if (next_element == "name-elements") {
					++j;
					lua_rawgeti(l, -1, j + 1);
					if (lua_istable(l, -1)) {
						ParseNameElements(l, name_type);
					}
					lua_pop(l, 1);
				} else {
					--j;
				}
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

				int civilization = PlayerRaces.GetRaceIndexByName(LuaToString(l, -1, j + 1));
				if (civilization == -1) {
					LuaError(l, "Civilization doesn't exist.");
				}
				++j;
				
				std::string cultural_name = LuaToString(l, -1, j + 1);
				
				tile->CulturalResourceNames[std::pair<int,int>(resource, civilization)].push_back(TransliterateText(cultural_name));
				
				int language = PlayerRaces.GetCivilizationLanguage(civilization);
				if (language == -1) {
					LuaError(l, "Language doesn't exist.");
				}
				if (PlayerRaces.Languages[language]->TypeNameCount.find(name_type) == PlayerRaces.Languages[language]->TypeNameCount.end()) {
					PlayerRaces.Languages[language]->TypeNameCount[name_type] = 0;
				}
				PlayerRaces.Languages[language]->TypeNameCount[name_type] += 1;
				
				++j;
				if (j >= subargs) {
					break;
				}
				
				std::string next_element = LuaToString(l, -1, j + 1);
				if (next_element == "name-elements") {
					++j;
					lua_rawgeti(l, -1, j + 1);
					if (lua_istable(l, -1)) {
						ParseNameElements(l, name_type);
					}
					lua_pop(l, 1);
				} else {
					--j;
				}
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

				int civilization = PlayerRaces.GetRaceIndexByName(LuaToString(l, -1, j + 1));
				if (civilization == -1) {
					LuaError(l, "Civilization doesn't exist.");
				}
				++j;
				
				int faction = PlayerRaces.GetFactionIndexByName(civilization, LuaToString(l, -1, j + 1));
				if (faction == -1) {
					LuaError(l, "Faction doesn't exist.");
				}
				++j;
				
				std::string cultural_name = LuaToString(l, -1, j + 1);
				
				tile->FactionCulturalResourceNames[std::pair<int,CFaction *>(resource, PlayerRaces.Factions[civilization][faction])].push_back(TransliterateText(cultural_name));
				
				int language = PlayerRaces.GetFactionLanguage(civilization, faction);
				if (language == -1) {
					LuaError(l, "Language doesn't exist.");
				}
				if (PlayerRaces.Languages[language]->TypeNameCount.find(name_type) == PlayerRaces.Languages[language]->TypeNameCount.end()) {
					PlayerRaces.Languages[language]->TypeNameCount[name_type] = 0;
				}
				PlayerRaces.Languages[language]->TypeNameCount[name_type] += 1;
				
				++j;
				if (j >= subargs) {
					break;
				}
				
				std::string next_element = LuaToString(l, -1, j + 1);
				if (next_element == "name-elements") {
					++j;
					lua_rawgeti(l, -1, j + 1);
					if (lua_istable(l, -1)) {
						ParseNameElements(l, name_type);
					}
					lua_pop(l, 1);
				} else {
					--j;
				}
			}
		} else if (!strcmp(value, "CulturalSettlementNames")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {

				int civilization = PlayerRaces.GetRaceIndexByName(LuaToString(l, -1, j + 1));
				if (civilization == -1) {
					LuaError(l, "Civilization doesn't exist.");
				}
				++j;
				
				std::string cultural_name = LuaToString(l, -1, j + 1);
				
				tile->CulturalSettlementNames[civilization].push_back(TransliterateText(cultural_name));
			}
		} else if (!strcmp(value, "FactionCulturalSettlementNames")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				int civilization = PlayerRaces.GetRaceIndexByName(LuaToString(l, -1, j + 1));
				if (civilization == -1) {
					LuaError(l, "Civilization doesn't exist.");
				}
				++j;
				
				int faction = PlayerRaces.GetFactionIndexByName(civilization, LuaToString(l, -1, j + 1));
				if (faction == -1) {
					LuaError(l, "Faction doesn't exist.");
				}
				++j;
				
				std::string cultural_name = LuaToString(l, -1, j + 1);
				
				tile->FactionCulturalSettlementNames[PlayerRaces.Factions[civilization][faction]].push_back(TransliterateText(cultural_name));
			}
		} else if (!strcmp(value, "Claims")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				int civilization = PlayerRaces.GetRaceIndexByName(LuaToString(l, -1, j + 1));
				if (civilization == -1) {
					LuaError(l, "Civilization doesn't exist.");
				}
				++j;
				
				int faction = PlayerRaces.GetFactionIndexByName(civilization, LuaToString(l, -1, j + 1));
				if (faction == -1) {
					LuaError(l, "Faction doesn't exist.");
				}
				
				tile->FactionClaims.push_back(PlayerRaces.Factions[civilization][faction]);
			}
		} else if (!strcmp(value, "HistoricalOwners")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				int year = LuaToNumber(l, -1, j + 1);
				++j;
				std::string owner_civilization_name = LuaToString(l, -1, j + 1);
				++j;
				std::string owner_faction_name = LuaToString(l, -1, j + 1);
				if (!owner_civilization_name.empty() && !owner_faction_name.empty()) {
					int owner_civilization = PlayerRaces.GetRaceIndexByName(owner_civilization_name.c_str());
					int owner_faction = PlayerRaces.GetFactionIndexByName(owner_civilization, owner_faction_name);
					if (owner_faction == -1) {
						LuaError(l, "Faction \"%s\" doesn't exist." _C_ owner_faction_name.c_str());
					}
					tile->HistoricalOwners[year] = PlayerRaces.Factions[owner_civilization][owner_faction];
				} else {
					tile->HistoricalOwners[year] = NULL;
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
				std::string claimant_civilization_name = LuaToString(l, -1, j + 1);
				int claimant_civilization = PlayerRaces.GetRaceIndexByName(claimant_civilization_name.c_str());
				++j;
				std::string claimant_faction_name = LuaToString(l, -1, j + 1);
				int claimant_faction = PlayerRaces.GetFactionIndexByName(claimant_civilization, claimant_faction_name);
				if (claimant_faction == -1) {
					LuaError(l, "Faction \"%s\" doesn't exist." _C_ claimant_faction_name.c_str());
				}
				tile->HistoricalClaims[year] = PlayerRaces.Factions[claimant_civilization][claimant_faction];
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	if (tile->World == NULL) {
		LuaError(l, "Tile (%d, %d) is not assigned to any world." _C_ tile->Position.x _C_ tile->Position.y);
	}
	
	return 0;
}

/**
**  Define a river.
**
**  @param l  Lua state.
*/
static int CclDefineRiver(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string river_name = LuaToString(l, 1);
	CRiver *river = GetRiver(river_name);
	if (!river) {
		river = new CRiver;
		river->Name = river_name;
		river->ID = Rivers.size();
		Rivers.push_back(river);
	}
	
	std::string name_type = "river";
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "World")) {
			CWorld *world = GetWorld(LuaToString(l, -1));
			if (world != NULL) {
				river->World = world;
				world->Rivers.push_back(river);
			} else {
				LuaError(l, "World doesn't exist.");
			}
		} else if (!strcmp(value, "CulturalNames")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				int civilization = PlayerRaces.GetRaceIndexByName(LuaToString(l, -1, j + 1));
				if (civilization == -1) {
					LuaError(l, "Civilization doesn't exist.");
				}
				++j;
				
				std::string cultural_name = LuaToString(l, -1, j + 1);
				
				river->CulturalNames[civilization] = TransliterateText(cultural_name);

				int language = PlayerRaces.GetCivilizationLanguage(civilization);
				if (language == -1) {
					LuaError(l, "Language doesn't exist.");
				}
				if (PlayerRaces.Languages[language]->TypeNameCount.find(name_type) == PlayerRaces.Languages[language]->TypeNameCount.end()) {
					PlayerRaces.Languages[language]->TypeNameCount[name_type] = 0;
				}
				PlayerRaces.Languages[language]->TypeNameCount[name_type] += 1;
				
				++j;
				if (j >= subargs) {
					break;
				}
				
				std::string next_element = LuaToString(l, -1, j + 1);
				if (next_element == "name-elements") {
					++j;
					lua_rawgeti(l, -1, j + 1);
					if (lua_istable(l, -1)) {
						ParseNameElements(l, name_type);
					}
					lua_pop(l, 1);
				} else {
					--j;
				}
			}
		} else if (!strcmp(value, "FactionCulturalNames")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				int civilization = PlayerRaces.GetRaceIndexByName(LuaToString(l, -1, j + 1));
				if (civilization == -1) {
					LuaError(l, "Civilization doesn't exist.");
				}
				++j;
				
				int faction = PlayerRaces.GetFactionIndexByName(civilization, LuaToString(l, -1, j + 1));
				if (faction == -1) {
					LuaError(l, "Faction doesn't exist.");
				}
				++j;
				
				std::string cultural_name = LuaToString(l, -1, j + 1);
				
				river->FactionCulturalNames[PlayerRaces.Factions[civilization][faction]] = TransliterateText(cultural_name);
				
				int language = PlayerRaces.GetFactionLanguage(civilization, faction);
				if (language == -1) {
					LuaError(l, "Language doesn't exist.");
				}
				if (PlayerRaces.Languages[language]->TypeNameCount.find(name_type) == PlayerRaces.Languages[language]->TypeNameCount.end()) {
					PlayerRaces.Languages[language]->TypeNameCount[name_type] = 0;
				}
				PlayerRaces.Languages[language]->TypeNameCount[name_type] += 1;
				
				++j;
				if (j >= subargs) {
					break;
				}
				
				std::string next_element = LuaToString(l, -1, j + 1);
				if (next_element == "name-elements") {
					++j;
					lua_rawgeti(l, -1, j + 1);
					if (lua_istable(l, -1)) {
						ParseNameElements(l, name_type);
					}
					lua_pop(l, 1);
				} else {
					--j;
				}
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	if (river->World == NULL) {
		LuaError(l, "River \"%s\" is not assigned to any world." _C_ river->Name.c_str());
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
	std::string plane_name = LuaToString(l, 1);
	CPlane *plane = GetPlane(plane_name);
	if (!plane) {
		LuaError(l, "Plane \"%s\" doesn't exist." _C_ plane_name.c_str());
	}
	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Name")) {
		lua_pushstring(l, plane->Name.c_str());
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
			lua_pushstring(l, plane->Species[i-1]->Ident.c_str());
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
	std::string world_name = LuaToString(l, 1);
	CWorld *world = GetWorld(world_name);
	if (!world) {
		LuaError(l, "World \"%s\" doesn't exist." _C_ world_name.c_str());
	}
	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Name")) {
		lua_pushstring(l, world->Name.c_str());
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
	} else if (!strcmp(data, "BaseTerrain")) {
		if (world->BaseTerrain) {
			lua_pushstring(l, world->BaseTerrain->Name.c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Plane")) {
		if (world->Plane) {
			lua_pushstring(l, world->Plane->Name.c_str());
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
			lua_pushstring(l, world->Species[i-1]->Ident.c_str());
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
		if (province->World != NULL) {
			lua_pushstring(l, province->World->Name.c_str());
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
	} else if (!strcmp(data, "Map")) {
		lua_pushstring(l, province->Map.c_str());
		return 1;
	} else if (!strcmp(data, "SettlementTerrain")) {
		lua_pushstring(l, province->SettlementTerrain.c_str());
		return 1;
	} else if (!strcmp(data, "SettlementLocationX")) {
		lua_pushnumber(l, province->SettlementLocation.x);
		return 1;
	} else if (!strcmp(data, "SettlementLocationY")) {
		lua_pushnumber(l, province->SettlementLocation.y);
		return 1;
	} else if (!strcmp(data, "Tiles")) {
		lua_createtable(l, province->Tiles.size() * 2, 0);
		for (size_t i = 1; i <= province->Tiles.size() * 2; ++i)
		{
			lua_pushnumber(l, province->Tiles[(i-1) / 2].x);
			lua_rawseti(l, -2, i);
			++i;

			lua_pushnumber(l, province->Tiles[(i-1) / 2].y);
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else {
		LuaError(l, "Invalid field: %s" _C_ data);
	}

	return 0;
}

/**
**  Get river data.
**
**  @param l  Lua state.
*/
static int CclGetRiverData(lua_State *l)
{
	if (lua_gettop(l) < 2) {
		LuaError(l, "incorrect argument");
	}
	std::string river_name = LuaToString(l, 1);
	CRiver *river = GetRiver(river_name);
	if (!river) {
		LuaError(l, "River \"%s\" doesn't exist." _C_ river_name.c_str());
	}
	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Name")) {
		lua_pushstring(l, river->Name.c_str());
		return 1;
	} else if (!strcmp(data, "World")) {
		if (river->World != NULL) {
			lua_pushstring(l, river->World->Name.c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else {
		LuaError(l, "Invalid field: %s" _C_ data);
	}

	return 0;
}

static int CclGetPlanes(lua_State *l)
{
	lua_createtable(l, Planes.size(), 0);
	for (size_t i = 1; i <= Planes.size(); ++i)
	{
		lua_pushstring(l, Planes[i-1]->Name.c_str());
		lua_rawseti(l, -2, i);
	}
	return 1;
}

static int CclGetWorlds(lua_State *l)
{
	lua_createtable(l, Worlds.size(), 0);
	for (size_t i = 1; i <= Worlds.size(); ++i)
	{
		lua_pushstring(l, Worlds[i-1]->Name.c_str());
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

static int CclGetRivers(lua_State *l)
{
	lua_createtable(l, Rivers.size(), 0);
	for (size_t i = 1; i <= Rivers.size(); ++i)
	{
		lua_pushstring(l, Rivers[i-1]->Name.c_str());
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
	lua_register(Lua, "DefinePlane", CclDefinePlane);
	lua_register(Lua, "DefineWorld", CclDefineWorld);
	lua_register(Lua, "DefineRegion", CclDefineRegion);
	lua_register(Lua, "DefineProvince", CclDefineProvince);
	lua_register(Lua, "DefineWorldMapTile", CclDefineWorldMapTile);
	lua_register(Lua, "DefineRiver", CclDefineRiver);
	lua_register(Lua, "GetPlaneData", CclGetPlaneData);
	lua_register(Lua, "GetWorldData", CclGetWorldData);
	lua_register(Lua, "GetProvinceData", CclGetProvinceData);
	lua_register(Lua, "GetRiverData", CclGetRiverData);
	lua_register(Lua, "GetPlanes", CclGetPlanes);
	lua_register(Lua, "GetWorlds", CclGetWorlds);
	lua_register(Lua, "GetProvinces", CclGetProvinces);
	lua_register(Lua, "GetRivers", CclGetRivers);
}

//@}
