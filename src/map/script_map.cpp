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
/**@name script_map.cpp - The map ccl functions. */
//
//      (c) Copyright 1999-2005 by Lutz Sammer and Jimmy Salmon
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

#include "map.h"

#include "iolib.h"
#include "script.h"
#include "tileset.h"
#include "translate.h"
#include "ui.h"
#include "unit.h"
#include "version.h"
#include "video.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**  Parse a map.
**
**  @param l  Lua state.
*/
static int CclStratagusMap(lua_State *l)
{
	int args = lua_gettop(l);
	for (int j = 0; j < args; ++j) {
		const char *value = LuaToString(l, j + 1);
		++j;

		if (!strcmp(value, "version")) {
			//Wyrmgus start
//			char buf[32];
			char buf[64];
			//Wyrmgus end

			const char *version = LuaToString(l, j + 1);
			strncpy(buf, VERSION, sizeof(buf));
			if (strcmp(buf, version)) {
				fprintf(stderr, "Warning not saved with this version.\n");
			}
		} else if (!strcmp(value, "uid")) {
			Map.Info.MapUID = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "description")) {
			Map.Info.Description = LuaToString(l, j + 1);
		} else if (!strcmp(value, "the-map")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				const char *value = LuaToString(l, j + 1, k + 1);
				++k;

				if (!strcmp(value, "size")) {
					lua_rawgeti(l, j + 1, k + 1);
					CclGetPos(l, &Map.Info.MapWidth, &Map.Info.MapHeight);
					lua_pop(l, 1);

					delete[] Map.Fields;
					Map.Fields = new CMapField[Map.Info.MapWidth * Map.Info.MapHeight];
					// FIXME: this should be CreateMap or InitMap?
				} else if (!strcmp(value, "fog-of-war")) {
					Map.NoFogOfWar = false;
					--k;
				} else if (!strcmp(value, "no-fog-of-war")) {
					Map.NoFogOfWar = true;
					--k;
				} else if (!strcmp(value, "filename")) {
					Map.Info.Filename = LuaToString(l, j + 1, k + 1);
				} else if (!strcmp(value, "map-fields")) {
					lua_rawgeti(l, j + 1, k + 1);
					if (!lua_istable(l, -1)) {
						LuaError(l, "incorrect argument");
					}
					const int subsubargs = lua_rawlen(l, -1);
					if (subsubargs != Map.Info.MapWidth * Map.Info.MapHeight) {
						fprintf(stderr, "Wrong tile table length: %d\n", subsubargs);
					}
					for (int i = 0; i < subsubargs; ++i) {
						lua_rawgeti(l, -1, i + 1);
						if (!lua_istable(l, -1)) {
							LuaError(l, "incorrect argument");
						}
						Map.Fields[i].parse(l);
						lua_pop(l, 1);
					}
					lua_pop(l, 1);
				} else {
					LuaError(l, "Unsupported tag: %s" _C_ value);
				}
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	return 0;
}

/**
**  Reveal the complete map.
**
**  @param l  Lua state.
*/
static int CclRevealMap(lua_State *l)
{
	//Wyrmgus start
//	LuaCheckArgs(l, 0);
	//Wyrmgus end
	if (CclInConfigFile || !Map.Fields) {
		FlagRevealMap = 1;
	} else {
		//Wyrmgus start
//		Map.Reveal();
		bool only_person_players = false;
		const int nargs = lua_gettop(l);
		if (nargs == 1) {
			only_person_players = LuaToBoolean(l, 1);
		}
		Map.Reveal(only_person_players);
		//Wyrmgus end
	}
	return 0;
}

/**
**  Center the map.
**
**  @param l  Lua state.
*/
static int CclCenterMap(lua_State *l)
{
	LuaCheckArgs(l, 2);
	const Vec2i pos(LuaToNumber(l, 1), LuaToNumber(l, 2));

	UI.SelectedViewport->Center(Map.TilePosToMapPixelPos_Center(pos));
	return 0;
}

/**
**  Define the starting viewpoint for a given player.
**
**  @param l  Lua state.
*/
static int CclSetStartView(lua_State *l)
{
	LuaCheckArgs(l, 3);

	const int p = LuaToNumber(l, 1);
	Players[p].StartPos.x = LuaToNumber(l, 2);
	Players[p].StartPos.y = LuaToNumber(l, 3);

	return 0;
}

/**
**  Show Map Location
**
**  @param l  Lua state.
*/
static int CclShowMapLocation(lua_State *l)
{
	// Put a unit on map, use its properties, except for
	// what is listed below

	LuaCheckArgs(l, 4);
	const char *unitname = LuaToString(l, 5);
	CUnitType *unitType = UnitTypeByIdent(unitname);
	if (!unitType) {
		DebugPrint("Unable to find UnitType '%s'" _C_ unitname);
		return 0;
	}
	CUnit *target = MakeUnit(*unitType, ThisPlayer);
	if (target != NULL) {
		target->Variable[HP_INDEX].Value = 0;
		target->tilePos.x = LuaToNumber(l, 1);
		target->tilePos.y = LuaToNumber(l, 2);
		target->TTL = GameCycle + LuaToNumber(l, 4);
		target->CurrentSightRange = LuaToNumber(l, 3);
		//Wyrmgus start
		UpdateUnitSightRange(*target);
		//Wyrmgus end
		MapMarkUnitSight(*target);
	} else {
		DebugPrint("Unable to allocate Unit");
	}
	return 0;
}

/**
**  Set fog of war on/off.
**
**  @param l  Lua state.
*/
static int CclSetFogOfWar(lua_State *l)
{
	LuaCheckArgs(l, 1);
	Map.NoFogOfWar = !LuaToBoolean(l, 1);
	if (!CclInConfigFile && Map.Fields) {
		UpdateFogOfWarChange();
		// FIXME: save setting in replay log
		//CommandLog("input", NoUnitP, FlushCommands, -1, -1, NoUnitP, "fow off", -1);
	}
	return 0;
}

static int CclGetFogOfWar(lua_State *l)
{
	LuaCheckArgs(l, 0);
	lua_pushboolean(l, !Map.NoFogOfWar);
	return 1;
}

/**
**  Enable display of terrain in minimap.
**
**  @param l  Lua state.
*/
static int CclSetMinimapTerrain(lua_State *l)
{
	LuaCheckArgs(l, 1);
	UI.Minimap.WithTerrain = LuaToBoolean(l, 1);
	return 0;
}

/**
**  Fog of war opacity.
**
**  @param l  Lua state.
*/
static int CclSetFogOfWarOpacity(lua_State *l)
{
	LuaCheckArgs(l, 1);
	int i = LuaToNumber(l, 1);
	if (i < 0 || i > 255) {
		PrintFunction();
		fprintf(stdout, "Opacity should be 0 - 256\n");
		i = 100;
	}
	FogOfWarOpacity = i;

	if (!CclInConfigFile) {
		Map.Init();
	}
	return 0;
}

/**
**  Set forest regeneration speed.
**
**  @param l  Lua state.
**
**  @return   Old speed
*/
static int CclSetForestRegeneration(lua_State *l)
{
	LuaCheckArgs(l, 1);
	int i = LuaToNumber(l, 1);
	//Wyrmgus start
	/*
	if (i < 0 || i > 255) {
		PrintFunction();
		fprintf(stdout, "Regeneration speed should be 0 - 255\n");
		i = 100;
	}
	*/
	if (i < 0) {
		PrintFunction();
		fprintf(stdout, "Regeneration speed should be greater than 0\n");
		i = 100;
	}
	//Wyrmgus end
	const int old = ForestRegeneration;
	ForestRegeneration = i;

	lua_pushnumber(l, old);
	return 1;
}

/**
**  Set Fog color.
**
**  @param l  Lua state.
*/
static int CclSetFogOfWarColor(lua_State *l)
{
	LuaCheckArgs(l, 3);
	int r = LuaToNumber(l, 1);
	int g = LuaToNumber(l, 2);
	int b = LuaToNumber(l, 3);

	if ((r < 0 || r > 255) ||
		(g < 0 || g > 255) ||
		(b < 0 || b > 255)) {
		LuaError(l, "Arguments must be in the range 0-255");
	}
	FogOfWarColor.R = r;
	FogOfWarColor.G = g;
	FogOfWarColor.B = b;

	return 0;
}

/**
**  Define Fog graphics
**
**  @param l  Lua state.
*/
static int CclSetFogOfWarGraphics(lua_State *l)
{
	std::string FogGraphicFile;

	LuaCheckArgs(l, 1);
	FogGraphicFile = LuaToString(l, 1);
	if (CMap::FogGraphic) {
		CGraphic::Free(CMap::FogGraphic);
	}
	CMap::FogGraphic = CGraphic::New(FogGraphicFile, PixelTileSize.x, PixelTileSize.y);

	return 0;
}

/**
**  Set a tile
**
**  @param tileIndex   Tile number
**  @param pos    coordinate
**  @param value  Value of the tile
*/
void SetTile(unsigned int tileIndex, const Vec2i &pos, int value)
{
	if (!Map.Info.IsPointOnMap(pos)) {
		fprintf(stderr, "Invalid map coordonate : (%d, %d)\n", pos.x, pos.y);
		return;
	}
	if (Map.Tileset->getTileCount() <= tileIndex) {
		fprintf(stderr, "Invalid tile number: %d\n", tileIndex);
		return;
	}
	//Wyrmgus start
//	if (value < 0 || value >= 256) {
	if (value < 0) {
	//Wyrmgus end
		//Wyrmgus start
//		fprintf(stderr, "Invalid tile number: %d\n", tileIndex);
		fprintf(stderr, "Invalid tile value: %d\n", value);
		//Wyrmgus end
		return;
	}
	
	if (Map.Fields) {
		CMapField &mf = *Map.Field(pos);

		mf.setTileIndex(*Map.Tileset, tileIndex, value);
	}
}

//Wyrmgus start
/**
**  Set a tile
**
**  @param tileIndex   Tile number
**  @param pos    coordinate
**  @param value  Value of the tile
*/
void SetTileTerrain(std::string terrain_ident, const Vec2i &pos, int value)
{
	if (!Map.Info.IsPointOnMap(pos)) {
		fprintf(stderr, "Invalid map coordinate : (%d, %d)\n", pos.x, pos.y);
		return;
	}
	
	CTerrainType *terrain = GetTerrainType(terrain_ident);
	
	if (!terrain) {
		fprintf(stderr, "Terrain \"%s\" doesn't exist.\n", terrain_ident.c_str());
		return;
	}
	if (value < 0) {
		fprintf(stderr, "Invalid tile value: %d\n", value);
		return;
	}
	
	if (Map.Fields) {
		CMapField &mf = *Map.Field(pos);

		mf.SetTerrain(terrain);
		mf.Value = value;
	}
}

void SetMapTemplateTileTerrain(std::string map_ident, std::string terrain_ident, int x, int y)
{
	CMapTemplate *map = GetMapTemplate(map_ident);
	
	if (!map) {
		fprintf(stderr, "Map template \"%s\" doesn't exist.\n", map_ident.c_str());
		return;
	}
	
	Vec2i pos(x, y);
	
	if (pos.x < 0 || pos.x >= map->Width || pos.y < 0 || pos.y >= map->Height) {
		fprintf(stderr, "Invalid map coordinate : (%d, %d)\n", pos.x, pos.y);
		return;
	}
	
	CTerrainType *terrain = GetTerrainType(terrain_ident);
	
	if (!terrain) {
		fprintf(stderr, "Terrain \"%s\" doesn't exist.\n", terrain_ident.c_str());
		return;
	}
	
	map->SetTileTerrain(pos, terrain);
}

void SetMapTemplateTileTerrainByID(std::string map_ident, int terrain_id, int x, int y)
{
	CMapTemplate *map = GetMapTemplate(map_ident);
	
	if (!map) {
		fprintf(stderr, "Map template \"%s\" doesn't exist.\n", map_ident.c_str());
		return;
	}
	
	Vec2i pos(x, y);
	
	if (pos.x < 0 || pos.x >= map->Width || pos.y < 0 || pos.y >= map->Height) {
		fprintf(stderr, "Invalid map coordinate : (%d, %d)\n", pos.x, pos.y);
		return;
	}
	
	CTerrainType *terrain = TerrainTypes[terrain_id];
	
	if (!terrain) {
		fprintf(stderr, "Terrain doesn't exist.\n");
		return;
	}

	map->SetTileTerrain(pos, terrain);
}

void ApplyMapTemplate(std::string map_template_ident, int start_x, int start_y)
{
	CMapTemplate *map_template = GetMapTemplate(map_template_ident);
	
	if (!map_template) {
		fprintf(stderr, "Map template \"%s\" doesn't exist.\n", map_template_ident.c_str());
		return;
	}
	
	Vec2i start_pos(start_x, start_y);
	
	if (start_pos.x < 0 || start_pos.x >= map_template->Width || start_pos.y < 0 || start_pos.y >= map_template->Height) {
		fprintf(stderr, "Invalid map coordinate : (%d, %d)\n", start_pos.x, start_pos.y);
		return;
	}
	
	for (int x = 0; x < Map.Info.MapHeight; ++x) {
		if ((start_pos.x + x) >= map_template->Width) {
			break;
		}
		for (int y = 0; y < Map.Info.MapHeight; ++y) {
			if ((start_pos.y + y) >= map_template->Height) {
				break;
			}
			
			Vec2i pos(start_pos.x + x, start_pos.y + y);
			Vec2i real_pos(x, y);
			if (map_template->GetTileTerrain(pos, false)) {
				SetTileTerrain(map_template->GetTileTerrain(pos, false)->Ident, real_pos);
			} else {
				SetTileTerrain(TerrainTypes[0]->Ident, real_pos);
			}
			if (map_template->GetTileTerrain(pos, true)) {
				SetTileTerrain(map_template->GetTileTerrain(pos, true)->Ident, real_pos);
			}
		}
	}
	
	for (size_t i = 0; i < map_template->GeneratedTerrains.size(); ++i) {
		int seed_number = Map.Info.MapWidth * Map.Info.MapHeight / 1024;
		int expansion_number = 0;
		
		int degree_level = map_template->GeneratedTerrains[i].second;
		
		if (degree_level == ExtremelyHighDegreeLevel) {
			expansion_number = Map.Info.MapWidth * Map.Info.MapHeight / 2;
			seed_number = Map.Info.MapWidth * Map.Info.MapHeight / 256;
		} else if (degree_level == VeryHighDegreeLevel) {
			expansion_number = Map.Info.MapWidth * Map.Info.MapHeight / 4;
			seed_number = Map.Info.MapWidth * Map.Info.MapHeight / 512;
		} else if (degree_level == HighDegreeLevel) {
			expansion_number = Map.Info.MapWidth * Map.Info.MapHeight / 8;
			seed_number = Map.Info.MapWidth * Map.Info.MapHeight / 1024;
		} else if (degree_level == MediumDegreeLevel) {
			expansion_number = Map.Info.MapWidth * Map.Info.MapHeight / 16;
			seed_number = Map.Info.MapWidth * Map.Info.MapHeight / 2048;
		} else if (degree_level == LowDegreeLevel) {
			expansion_number = Map.Info.MapWidth * Map.Info.MapHeight / 32;
			seed_number = Map.Info.MapWidth * Map.Info.MapHeight / 4096;
		} else if (degree_level == VeryLowDegreeLevel) {
			expansion_number = Map.Info.MapWidth * Map.Info.MapHeight / 64;
			seed_number = Map.Info.MapWidth * Map.Info.MapHeight / 8192;
		}
		
		seed_number = std::max(1, seed_number);
		
		Map.GenerateTerrain(map_template->GeneratedTerrains[i].first, seed_number, expansion_number, Vec2i(0, 0), Vec2i(Map.Info.MapWidth, Map.Info.MapHeight));
	}
	
	Map.AdjustTileMapIrregularities(false);
	Map.AdjustTileMapIrregularities(true);
	Map.AdjustTileMapTransitions();
	Map.AdjustTileMapIrregularities(false);
	Map.AdjustTileMapIrregularities(true);
}
//Wyrmgus end

/**
**  Define the type of each player available for the map
**
**  @param l  Lua state.
*/
static int CclDefinePlayerTypes(lua_State *l)
{
	int numplayers = lua_gettop(l); /* Number of players == number of arguments */
	if (numplayers < 2) {
		LuaError(l, "Not enough players");
	}

	for (int i = 0; i < numplayers && i < PlayerMax; ++i) {
		if (lua_isnil(l, i + 1)) {
			numplayers = i;
			break;
		}
		const char *type = LuaToString(l, i + 1);
		if (!strcmp(type, "neutral")) {
			Map.Info.PlayerType[i] = PlayerNeutral;
		} else if (!strcmp(type, "nobody")) {
			Map.Info.PlayerType[i] = PlayerNobody;
		} else if (!strcmp(type, "computer")) {
			Map.Info.PlayerType[i] = PlayerComputer;
		} else if (!strcmp(type, "person")) {
			Map.Info.PlayerType[i] = PlayerPerson;
		} else if (!strcmp(type, "rescue-passive")) {
			Map.Info.PlayerType[i] = PlayerRescuePassive;
		} else if (!strcmp(type, "rescue-active")) {
			Map.Info.PlayerType[i] = PlayerRescueActive;
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ type);
		}
	}
	for (int i = numplayers; i < PlayerMax - 1; ++i) {
		Map.Info.PlayerType[i] = PlayerNobody;
	}
	if (numplayers < PlayerMax) {
		Map.Info.PlayerType[PlayerMax - 1] = PlayerNeutral;
	}
	return 0;
}

/**
** Load the lua file which will define the tile models
**
**  @param l  Lua state.
*/
static int CclLoadTileModels(lua_State *l)
{
	if (lua_gettop(l) != 1) {
		LuaError(l, "incorrect argument");
	}
	Map.TileModelsFileName = LuaToString(l, 1);
	const std::string filename = LibraryFileName(Map.TileModelsFileName.c_str());
	if (LuaLoadFile(filename) == -1) {
		DebugPrint("Load failed: %s\n" _C_ filename.c_str());
	}
	return 0;
}

/**
**  Define tileset
**
**  @param l  Lua state.
*/
static int CclDefineTileset(lua_State *l)
{
	Map.Tileset->parse(l);

	//  Load and prepare the tileset
	PixelTileSize = Map.Tileset->getPixelTileSize();

	ShowLoadProgress(_("Loading Tileset \"%s\""), Map.Tileset->ImageFile.c_str());
	Map.TileGraphic = CGraphic::New(Map.Tileset->ImageFile, PixelTileSize.x, PixelTileSize.y);
	Map.TileGraphic->Load();
	//Wyrmgus start
	for (size_t i = 0; i != Map.Tileset->solidTerrainTypes.size(); ++i) {
		if (!Map.Tileset->solidTerrainTypes[i].ImageFile.empty()) {
			Map.SolidTileGraphics[i] = CGraphic::New(Map.Tileset->solidTerrainTypes[i].ImageFile, PixelTileSize.x, PixelTileSize.y);
			Map.SolidTileGraphics[i]->Load();
		}
	}
	//Wyrmgus end
	return 0;
}
/**
** Build tileset tables like humanWallTable or mixedLookupTable
**
** Called after DefineTileset and only for tilesets that have wall,
** trees and rocks. This function will be deleted when removing
** support of walls and alike in the tileset.
*/
static int CclBuildTilesetTables(lua_State *l)
{
	LuaCheckArgs(l, 0);

	Map.Tileset->buildTable(l);
	return 0;
}
/**
**  Set the flags like "water" for a tile of a tileset
**
**  @param l  Lua state.
*/
static int CclSetTileFlags(lua_State *l)
{
	if (lua_gettop(l) < 2) {
		LuaError(l, "No flags defined");
	}
	const unsigned int tilenumber = LuaToNumber(l, 1);

	if (tilenumber >= Map.Tileset->tiles.size()) {
		LuaError(l, "Accessed a tile that's not defined");
	}
	int j = 0;
	int flags = 0;

	ParseTilesetTileFlags(l, &flags, &j);
	Map.Tileset->tiles[tilenumber].flag = flags;
	return 0;
}

//Wyrmgus start
/**
**  Get the ident of the current tileset.
**
**  @param l  Lua state.
**
**  @return   The name of the terrain of the tile.
*/
static int CclGetCurrentTileset(lua_State *l)
{
	const CTileset &tileset = *Map.Tileset;
	lua_pushstring(l, tileset.Ident.c_str());
	return 1;
}
//Wyrmgus end

/**
**  Get the name of the terrain of the tile.
**
**  @param l  Lua state.
**
**  @return   The name of the terrain of the tile.
*/
static int CclGetTileTerrainName(lua_State *l)
{
	LuaCheckArgs(l, 2);

	const Vec2i pos(LuaToNumber(l, 1), LuaToNumber(l, 2));

	//Wyrmgus start
	/*
	const CMapField &mf = *Map.Field(pos);
	const CTileset &tileset = *Map.Tileset;
	const int index = tileset.findTileIndexByTile(mf.getGraphicTile());
	Assert(index != -1);
	const int baseTerrainIdx = tileset.tiles[index].tileinfo.BaseTerrain;

	lua_pushstring(l, tileset.getTerrainName(baseTerrainIdx).c_str());
	*/
	lua_pushstring(l, Map.GetTileTopTerrain(pos)->Ident.c_str());
	//Wyrmgus end
	return 1;
}

/**
**  Get the name of the mixed terrain of the tile.
**
**  @param l  Lua state.
**
**  @return   The name of the terrain of the tile.
*/
//Wyrmgus start
/*
static int CclGetTileTerrainMixedName(lua_State *l)
{
	LuaCheckArgs(l, 2);

	const Vec2i pos(LuaToNumber(l, 1), LuaToNumber(l, 2));

	const CMapField &mf = *Map.Field(pos);
	const CTileset &tileset = *Map.Tileset;
	//Wyrmgus start
//	const int index = tileset.findTileIndexByTile(mf.getGraphicTile());
	const int index = mf.getTileIndex();
	//Wyrmgus end
	Assert(index != -1);
	const int mixTerrainIdx = tileset.tiles[index].tileinfo.MixTerrain;

	lua_pushstring(l, mixTerrainIdx ? tileset.getTerrainName(mixTerrainIdx).c_str() : "");
	return 1;
}
*/
//Wyrmgus end

/**
**  Check if the tile's terrain has a particular flag.
**
**  @param l  Lua state.
**
**  @return   True if has the flag, false if not.
*/
static int CclGetTileTerrainHasFlag(lua_State *l)
{
	LuaCheckArgs(l, 3);

	const Vec2i pos(LuaToNumber(l, 1), LuaToNumber(l, 2));

	//Wyrmgus start
	if (pos.x < 0 || pos.x >= Map.Info.MapWidth || pos.y < 0 || pos.y >= Map.Info.MapHeight) {
		lua_pushboolean(l, 0);
		return 1;
	}
	
//	unsigned short flag = 0;
	unsigned long flag = 0;
	//Wyrmgus end
	const char *flag_name = LuaToString(l, 3);
	if (!strcmp(flag_name, "water")) {
		flag = MapFieldWaterAllowed;
	} else if (!strcmp(flag_name, "land")) {
		flag = MapFieldLandAllowed;
	} else if (!strcmp(flag_name, "coast")) {
		flag = MapFieldCoastAllowed;
	} else if (!strcmp(flag_name, "no-building")) {
		flag = MapFieldNoBuilding;
	} else if (!strcmp(flag_name, "unpassable")) {
		flag = MapFieldUnpassable;
	//Wyrmgus start
	} else if (!strcmp(flag_name, "air-unpassable")) {
		flag = MapFieldAirUnpassable;
	} else if (!strcmp(flag_name, "dirt")) {
		flag = MapFieldDirt;
	} else if (!strcmp(flag_name, "grass")) {
		flag = MapFieldGrass;
	} else if (!strcmp(flag_name, "gravel")) {
		flag = MapFieldGravel;
	} else if (!strcmp(flag_name, "mud")) {
		flag = MapFieldMud;
	} else if (!strcmp(flag_name, "stone-floor")) {
		flag = MapFieldStoneFloor;
	} else if (!strcmp(flag_name, "stumps")) {
		flag = MapFieldStumps;
	//Wyrmgus end
	} else if (!strcmp(flag_name, "wall")) {
		flag = MapFieldWall;
	} else if (!strcmp(flag_name, "rock")) {
		flag = MapFieldRocks;
	} else if (!strcmp(flag_name, "forest")) {
		flag = MapFieldForest;
	}

	const CMapField &mf = *Map.Field(pos);

	if (mf.getFlag() & flag) {
		lua_pushboolean(l, 1);
	} else {
		lua_pushboolean(l, 0);
	}

	return 1;
}

//Wyrmgus start
/**
**  Define a terrain type.
**
**  @param l  Lua state.
*/
static int CclDefineTerrainType(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string terrain_ident = LuaToString(l, 1);
	CTerrainType *terrain = GetTerrainType(terrain_ident);
	if (terrain == NULL) {
		terrain = new CTerrainType;
		terrain->Ident = terrain_ident;
		terrain->ID = TerrainTypes.size();
		TerrainTypes.push_back(terrain);
		TerrainTypeStringToIndex[terrain_ident] = terrain->ID;
	}
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Name")) {
			terrain->Name = LuaToString(l, -1);
		} else if (!strcmp(value, "Character")) {
			terrain->Character = LuaToString(l, -1);
			if (TerrainTypeCharacterToPointer.find(terrain->Character) != TerrainTypeCharacterToPointer.end()) {
				LuaError(l, "Character \"%s\" is already used by another terrain type." _C_ terrain->Character.c_str());
			}
			TerrainTypeCharacterToPointer[terrain->Character] = terrain;
		} else if (!strcmp(value, "Overlay")) {
			terrain->Overlay = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Buildable")) {
			terrain->Buildable = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "AllowSingle")) {
			terrain->AllowSingle = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "SolidAnimationFrames")) {
			terrain->SolidAnimationFrames = LuaToNumber(l, -1);
		} else if (!strcmp(value, "BaseTerrains")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				CTerrainType *base_terrain = GetTerrainType(LuaToString(l, -1, j + 1));
				if (base_terrain == NULL) {
					LuaError(l, "Terrain doesn't exist.");
				}
				terrain->BaseTerrains.push_back(base_terrain);
			}
		} else if (!strcmp(value, "InnerBorderTerrains")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				CTerrainType *border_terrain = GetTerrainType(LuaToString(l, -1, j + 1));
				if (border_terrain == NULL) {
					LuaError(l, "Terrain doesn't exist.");
				}
				terrain->InnerBorderTerrains.push_back(border_terrain);
				terrain->BorderTerrains.push_back(border_terrain);
				border_terrain->OuterBorderTerrains.push_back(terrain);
				border_terrain->BorderTerrains.push_back(terrain);
			}
		} else if (!strcmp(value, "OuterBorderTerrains")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				CTerrainType *border_terrain = GetTerrainType(LuaToString(l, -1, j + 1));
				if (border_terrain == NULL) {
					LuaError(l, "Terrain doesn't exist.");
				}
				terrain->OuterBorderTerrains.push_back(border_terrain);
				terrain->BorderTerrains.push_back(border_terrain);
				border_terrain->InnerBorderTerrains.push_back(terrain);
				border_terrain->BorderTerrains.push_back(terrain);
			}
		} else if (!strcmp(value, "Flags")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			terrain->Flags = 0;
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				std::string tile_flag = LuaToString(l, -1, j + 1);
				if (tile_flag == "land") {
					terrain->Flags |= MapFieldLandAllowed;
				} else if (tile_flag == "coast") {
					terrain->Flags |= MapFieldCoastAllowed;
				} else if (tile_flag == "water") {
					terrain->Flags |= MapFieldWaterAllowed;
				} else if (tile_flag == "no-building") {
					terrain->Flags |= MapFieldNoBuilding;
				} else if (tile_flag == "unpassable") {
					terrain->Flags |= MapFieldUnpassable;
				} else if (tile_flag == "wall") {
					terrain->Flags |= MapFieldWall;
				} else if (tile_flag == "rock") {
					terrain->Flags |= MapFieldRocks;
				} else if (tile_flag == "forest") {
					terrain->Flags |= MapFieldForest;
				} else if (tile_flag == "air-unpassable") {
					terrain->Flags |= MapFieldAirUnpassable;
				} else if (tile_flag == "dirt") {
					terrain->Flags |= MapFieldDirt;
				} else if (tile_flag == "grass") {
					terrain->Flags |= MapFieldGrass;
				} else if (tile_flag == "gravel") {
					terrain->Flags |= MapFieldGravel;
				} else if (tile_flag == "mud") {
					terrain->Flags |= MapFieldMud;
				} else if (tile_flag == "stone-floor") {
					terrain->Flags |= MapFieldStoneFloor;
				} else if (tile_flag == "stumps") {
					terrain->Flags |= MapFieldStumps;
				} else {
					LuaError(l, "Flag \"%s\" doesn't exist.");
				}
			}
		} else if (!strcmp(value, "Graphics")) {
			std::string graphics_file = LuaToString(l, -1);
			if (!CanAccessFile(graphics_file.c_str())) {
				LuaError(l, "File \"%s\" doesn't exist." _C_ graphics_file.c_str());
			}
			if (CGraphic::Get(graphics_file) == NULL) {
				CGraphic *graphics = CGraphic::New(graphics_file, 32, 32);
			}
			terrain->Graphics = CGraphic::Get(graphics_file);
		} else if (!strcmp(value, "SolidTiles")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				terrain->SolidTiles.push_back(LuaToNumber(l, -1, j + 1));
			}
		} else if (!strcmp(value, "DamagedTiles")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				terrain->DamagedTiles.push_back(LuaToNumber(l, -1, j + 1));
			}
		} else if (!strcmp(value, "DestroyedTiles")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				terrain->DestroyedTiles.push_back(LuaToNumber(l, -1, j + 1));
			}
		} else if (!strcmp(value, "TransitionTiles")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				std::string transition_terrain_name = LuaToString(l, -1, j + 1);
				CTerrainType *transition_terrain = GetTerrainType(transition_terrain_name);
				if (transition_terrain == NULL && transition_terrain_name != "any") {
					LuaError(l, "Terrain doesn't exist.");
				}
				int transition_terrain_id = transition_terrain_name == "any" ? -1 : transition_terrain->ID;
				++j;
				
				int transition_type = GetTransitionTypeIdByName(LuaToString(l, -1, j + 1));
				if (transition_type == -1) {
					LuaError(l, "Transition type doesn't exist.");
				}
				++j;
				
				terrain->TransitionTiles[std::tuple<int, int>(transition_terrain_id, transition_type)].push_back(LuaToNumber(l, -1, j + 1));
			}
		} else if (!strcmp(value, "AdjacentTransitionTiles")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				std::string transition_terrain_name = LuaToString(l, -1, j + 1);
				CTerrainType *transition_terrain = GetTerrainType(transition_terrain_name);
				if (transition_terrain == NULL && transition_terrain_name != "any") {
					LuaError(l, "Terrain doesn't exist.");
				}
				int transition_terrain_id = transition_terrain_name == "any" ? -1 : transition_terrain->ID;
				++j;
				
				int transition_type = GetTransitionTypeIdByName(LuaToString(l, -1, j + 1));
				if (transition_type == -1) {
					LuaError(l, "Transition type doesn't exist.");
				}
				++j;
				
				terrain->AdjacentTransitionTiles[std::tuple<int, int>(transition_terrain_id, transition_type)].push_back(LuaToNumber(l, -1, j + 1));
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	return 0;
}

/**
**  Define a map template.
**
**  @param l  Lua state.
*/
static int CclDefineMapTemplate(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string map_ident = LuaToString(l, 1);
	CMapTemplate *map = GetMapTemplate(map_ident);
	if (map == NULL) {
		map = new CMapTemplate;
		map->Ident = map_ident;
		MapTemplates.push_back(map);
		MapTemplateIdentToPointer[map_ident] = map;
	}
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Name")) {
			map->Name = LuaToString(l, -1);
		} else if (!strcmp(value, "TerrainFile")) {
			map->TerrainFile = LuaToString(l, -1);
		} else if (!strcmp(value, "Width")) {
			map->Width = LuaToNumber(l, -1);
		} else if (!strcmp(value, "Height")) {
			map->Height = LuaToNumber(l, -1);
		} else if (!strcmp(value, "GeneratedTerrains")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				CTerrainType *terrain = GetTerrainType(LuaToString(l, -1, j + 1));
				if (!terrain) {
					LuaError(l, "Terrain doesn't exist.");
				}
				++j;
				
				int degree_level = GetDegreeLevelIdByName(LuaToString(l, -1, j + 1));
				if (degree_level == -1) {
					LuaError(l, "Degree level doesn't exist.");
				}
				
				map->GeneratedTerrains.push_back(std::pair<CTerrainType *, int>(terrain, degree_level));
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	for (int i = 0; i < map->Width * map->Height; ++i) {
		map->TileTerrains.push_back(NULL);
		map->TileOverlayTerrains.push_back(NULL);
	}
	
	map->ParseTerrainFile();
	
	return 0;
}
//Wyrmgus end

/**
**  Register CCL features for map.
*/
void MapCclRegister()
{
	lua_register(Lua, "StratagusMap", CclStratagusMap);
	lua_register(Lua, "RevealMap", CclRevealMap);
	lua_register(Lua, "CenterMap", CclCenterMap);
	lua_register(Lua, "SetStartView", CclSetStartView);
	lua_register(Lua, "ShowMapLocation", CclShowMapLocation);

	lua_register(Lua, "SetFogOfWar", CclSetFogOfWar);
	lua_register(Lua, "GetFogOfWar", CclGetFogOfWar);
	lua_register(Lua, "SetMinimapTerrain", CclSetMinimapTerrain);

	lua_register(Lua, "SetFogOfWarGraphics", CclSetFogOfWarGraphics);
	lua_register(Lua, "SetFogOfWarOpacity", CclSetFogOfWarOpacity);
	lua_register(Lua, "SetFogOfWarColor", CclSetFogOfWarColor);

	lua_register(Lua, "SetForestRegeneration", CclSetForestRegeneration);

	lua_register(Lua, "LoadTileModels", CclLoadTileModels);
	lua_register(Lua, "DefinePlayerTypes", CclDefinePlayerTypes);

	lua_register(Lua, "DefineTileset", CclDefineTileset);
	lua_register(Lua, "SetTileFlags", CclSetTileFlags);
	lua_register(Lua, "BuildTilesetTables", CclBuildTilesetTables);

	//Wyrmgus start
	lua_register(Lua, "GetCurrentTileset", CclGetCurrentTileset);
	//Wyrmgus end
	lua_register(Lua, "GetTileTerrainName", CclGetTileTerrainName);
	//Wyrmgus start
//	lua_register(Lua, "GetTileTerrainMixedName", CclGetTileTerrainMixedName);
	//Wyrmgus end
	lua_register(Lua, "GetTileTerrainHasFlag", CclGetTileTerrainHasFlag);
	
	//Wyrmgus start
	lua_register(Lua, "DefineTerrainType", CclDefineTerrainType);
	lua_register(Lua, "DefineMapTemplate", CclDefineMapTemplate);
	//Wyrmgus end
}

//@}
