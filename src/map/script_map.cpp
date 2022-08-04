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
//      (c) Copyright 1999-2022 by Lutz Sammer, Jimmy Salmon and Andrettin
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

#include "map/map.h"

#include "character.h"
#include "database/defines.h"
#include "database/preferences.h"
#include "editor.h"
#include "game/game.h"
#include "iolib.h"
#include "item/unique_item.h"
#include "map/landmass.h"
#include "map/map_info.h"
#include "map/map_layer.h"
#include "map/map_template.h"
#include "map/region.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/terrain_feature.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "map/tile_flag.h"
#include "map/tileset.h"
#include "map/world.h"
#include "player/civilization.h"
#include "player/faction.h"
#include "player/player.h"
#include "player/player_type.h"
//Wyrmgus start
#include "province.h"
//Wyrmgus end
#include "quest/campaign.h"
#include "script.h"
//Wyrmgus start
#include "settings.h"
//Wyrmgus end
#include "time/season_schedule.h"
#include "time/timeline.h"
#include "time/time_of_day_schedule.h"
#include "translator.h"
#include "ui/ui.h"
#include "unit/historical_unit.h"
#include "unit/unit.h"
#include "unit/unit_class.h"
#include "util/exception_util.h"
#include "util/log_util.h"
#include "util/point_util.h"
#include "util/util.h"
#include "video/video.h"

/**
**  Parse a map.
**
**  @param l  Lua state.
*/
int CclStratagusMap(lua_State *l)
{
	int args = lua_gettop(l);
	for (int j = 0; j < args; ++j) {
		const char *value = LuaToString(l, j + 1);
		++j;

		if (!strcmp(value, "version")) {
			const std::string save_version = LuaToString(l, j + 1);
			if (save_version != QApplication::applicationVersion().toStdString()) {
				fprintf(stderr, "Warning: not saved with this version.\n");
			}
		} else if (!strcmp(value, "uid")) {
			CMap::get()->Info->MapUID = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "description")) {
			CMap::get()->Info->name = LuaToString(l, j + 1);
		} else if (!strcmp(value, "the-map")) {
			if (!lua_istable(l, j + 1)) {
				//Wyrmgus start
//				LuaError(l, "incorrect argument");
				LuaError(l, "incorrect argument for \"the-map\"");
				//Wyrmgus end
			}
			int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				const char *subvalue = LuaToString(l, j + 1, k + 1);
				++k;

				if (!strcmp(subvalue, "fog-of-war")) {
					CMap::get()->NoFogOfWar = false;
					--k;
				} else if (!strcmp(subvalue, "no-fog-of-war")) {
					CMap::get()->NoFogOfWar = true;
					--k;
				} else if (!strcmp(subvalue, "filename")) {
					const std::filesystem::path filepath = LuaToString(l, j + 1, k + 1);
					CMap::get()->Info->set_presentation_filepath(filepath);
				//Wyrmgus start
				} else if (!strcmp(subvalue, "time-of-day")) {
					lua_rawgeti(l, j + 1, k + 1);
					if (!lua_istable(l, -1)) {
						LuaError(l, "incorrect argument for \"time-of-day\"");
					}
					const int subsubargs = lua_rawlen(l, -1);
					for (int z = 0; z < subsubargs; ++z) {
						const std::unique_ptr<CMapLayer> &map_layer = CMap::get()->MapLayers[z];
						if (!lua_istable(l, -1)) {
							LuaError(l, "incorrect argument for \"time-of-day\"");
						}
						lua_rawgeti(l, -1, z + 1);
						std::string time_of_day_schedule_ident = LuaToString(l, -1, 1);
						if (!time_of_day_schedule_ident.empty()) {
							map_layer->set_time_of_day_schedule(time_of_day_schedule::get(time_of_day_schedule_ident));
						} else {
							map_layer->set_time_of_day_schedule(nullptr);
						}
						unsigned time_of_day = LuaToNumber(l, -1, 2);
						if (map_layer->get_time_of_day_schedule() != nullptr && time_of_day < map_layer->get_time_of_day_schedule()->get_scheduled_times_of_day().size()) {
							map_layer->time_of_day = map_layer->get_time_of_day_schedule()->get_scheduled_times_of_day()[time_of_day].get();
						}
						map_layer->RemainingTimeOfDayHours = LuaToNumber(l, -1, 3);
						lua_pop(l, 1);
					}
					lua_pop(l, 1);
				} else if (!strcmp(subvalue, "season")) {
					lua_rawgeti(l, j + 1, k + 1);
					if (!lua_istable(l, -1)) {
						LuaError(l, "incorrect argument for \"season\"");
					}
					const int subsubargs = lua_rawlen(l, -1);
					for (int z = 0; z < subsubargs; ++z) {
						if (!lua_istable(l, -1)) {
							LuaError(l, "incorrect argument for \"season\"");
						}
						lua_rawgeti(l, -1, z + 1);
						CMap::get()->MapLayers[z]->set_season_schedule(season_schedule::try_get(LuaToString(l, -1, 1)));
						unsigned season = LuaToNumber(l, -1, 2);
						if (CMap::get()->MapLayers[z]->get_season_schedule() && season < CMap::get()->MapLayers[z]->get_season_schedule()->get_scheduled_seasons().size()) {
							CMap::get()->MapLayers[z]->season = CMap::get()->MapLayers[z]->get_season_schedule()->get_scheduled_seasons()[season].get();
						}
						CMap::get()->MapLayers[z]->RemainingSeasonHours = LuaToNumber(l, -1, 3);
						lua_pop(l, 1);
					}
					lua_pop(l, 1);
				} else if (!strcmp(subvalue, "layer-references")) {
					lua_rawgeti(l, j + 1, k + 1);
					if (!lua_istable(l, -1)) {
						LuaError(l, "incorrect argument for \"layer-references\"");
					}
					const int subsubargs = lua_rawlen(l, -1);
					for (int z = 0; z < subsubargs; ++z) {
						if (!lua_istable(l, -1)) {
							LuaError(l, "incorrect argument for \"layer-references\"");
						}
						lua_rawgeti(l, -1, z + 1);
						CMap::get()->MapLayers[z]->world = wyrmgus::world::try_get(LuaToString(l, -1, 1));
						lua_pop(l, 1);
					}
					lua_pop(l, 1);
				//Wyrmgus end
				} else if (!strcmp(subvalue, "map-fields")) {
					//Wyrmgus start
					/*
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
					*/
					lua_rawgeti(l, j + 1, k + 1);
					if (!lua_istable(l, -1)) {
						LuaError(l, "incorrect argument");
					}
					const int subsubargs = lua_rawlen(l, -1);
					for (int z = 0; z < subsubargs; ++z) {
						const std::unique_ptr<CMapLayer> &map_layer = CMap::get()->MapLayers[z];
						
						lua_rawgeti(l, -1, z + 1);
						if (!lua_istable(l, -1)) {
							LuaError(l, "incorrect argument");
						}
						const int subsubsubargs = lua_rawlen(l, -1);
						if (subsubsubargs != CMap::get()->Info->MapWidths[z] * CMap::get()->Info->MapHeights[z]) {
							fprintf(stderr, "Wrong tile table length: %d\n", subsubsubargs);
						}
						for (int i = 0; i < subsubsubargs; ++i) {
							lua_rawgeti(l, -1, i + 1);
							if (!lua_istable(l, -1)) {
								LuaError(l, "incorrect argument");
							}
							wyrmgus::tile &mf = *map_layer->Field(i);
							mf.parse(l);
							if (mf.is_destroyed_tree_tile()) {
								map_layer->destroyed_tree_tiles.push_back(map_layer->GetPosFromIndex(i));
							} else if (mf.get_overlay_terrain() != nullptr && mf.OverlayTerrainDestroyed) {
								map_layer->destroyed_overlay_terrain_tiles.push_back(map_layer->GetPosFromIndex(i));
							}
							lua_pop(l, 1);
						}
						lua_pop(l, 1);
					}
					lua_pop(l, 1);
					//Wyrmgus end
				} else {
					LuaError(l, "Unsupported tag: %s" _C_ subvalue);
				}
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	for (size_t z = 0; z < CMap::get()->MapLayers.size(); ++z) {
		CMap::get()->process_settlement_territory_tiles(z);

		for (int ix = 0; ix < CMap::get()->Info->MapWidths[z]; ++ix) {
			for (int iy = 0; iy < CMap::get()->Info->MapHeights[z]; ++iy) {
				const QPoint tile_pos(ix, iy);
				CMap::get()->CalculateTileOwnershipTransition(tile_pos, z); //so that the correct ownership border is shown after a loaded game
			}
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
	//Wyrmgus start
//	if (CclInConfigFile || !CMap::get()->Fields) {
	if (CclInConfigFile || CMap::get()->MapLayers.size() == 0) {
	//Wyrmgus end
		FlagRevealMap = 1;
	} else {
		//Wyrmgus start
//		Map.Reveal();
		bool only_person_players = false;
		const int nargs = lua_gettop(l);
		if (nargs == 1) {
			only_person_players = LuaToBoolean(l, 1);
		}
		CMap::get()->Reveal(only_person_players);
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

	UI.SelectedViewport->Center(CMap::get()->tile_pos_to_scaled_map_pixel_pos_center(pos));
	return 0;
}

/**
**  Define the starting viewpoint for a given player.
**
**  @param l  Lua state.
*/
static int CclSetStartView(lua_State *l)
{
	//Wyrmgus start
//	LuaCheckArgs(l, 3);
	const int nargs = lua_gettop(l);
	if (nargs < 3 || nargs > 4) {
		LuaError(l, "incorrect argument\n");
	}
	//Wyrmgus end

	const int p = LuaToNumber(l, 1);
	CPlayer::Players[p]->StartPos.x = LuaToNumber(l, 2);
	CPlayer::Players[p]->StartPos.y = LuaToNumber(l, 3);
	
	//Wyrmgus start
	if (nargs >= 4) {
		CPlayer::Players[p]->StartMapLayer = LuaToNumber(l, 4);
	}
	//Wyrmgus end

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
	wyrmgus::unit_type *unitType = wyrmgus::unit_type::get(unitname);
	CUnit *target = MakeUnit(*unitType, CPlayer::GetThisPlayer());
	if (target != nullptr) {
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
	CMap::get()->NoFogOfWar = !LuaToBoolean(l, 1);

	//Wyrmgus start
//	if (!CclInConfigFile && CMap::get()->Fields) {
	if (!CclInConfigFile && CMap::get()->MapLayers.size() > 0) {
	//Wyrmgus end
		UpdateFogOfWarChange();
		// FIXME: save setting in replay log
		//CommandLog("input", NoUnitP, FlushCommands, -1, -1, NoUnitP, "fow off", -1);
	}

	return 0;
}

static int CclGetFogOfWar(lua_State *l)
{
	LuaCheckArgs(l, 0);
	lua_pushboolean(l, !CMap::get()->NoFogOfWar);
	return 1;
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
		CMap::get()->Init();
	}
	return 0;
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
	
	CMap::FogGraphics = CGraphic::New(FogGraphicFile, defines::get()->get_tile_size());

	return 0;
}

/**
**  Set a tile
**
**  @param tileIndex   Tile number
**  @param pos    coordinate
**  @param value  Value of the tile
*/
void SetTile(unsigned int tileIndex, const Vec2i &pos, int value, int z)
{
	if (!CMap::get()->Info->IsPointOnMap(pos, z)) {
		fprintf(stderr, "Invalid map coordonate : (%d, %d)\n", pos.x, pos.y);
		return;
	}
	if (CMap::get()->Tileset->getTileCount() <= tileIndex) {
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
	
	//Wyrmgus start
//	if (Map.Fields) {
	if (static_cast<int>(CMap::get()->MapLayers.size()) >= z) {
	//Wyrmgus end
		//Wyrmgus start
//		wyrmgus::tile &mf = *CMap::get()->Field(pos);
		wyrmgus::tile &mf = *CMap::get()->Field(pos, z);
		//Wyrmgus end

		mf.setTileIndex(*CMap::get()->Tileset, tileIndex, value);
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
void SetTileTerrain(const std::string &terrain_ident, const Vec2i &pos, int value, int z)
{
	if (!CMap::get()->Info->IsPointOnMap(pos, z)) {
		fprintf(stderr, "Invalid map coordinate : (%d, %d)\n", pos.x, pos.y);
		return;
	}
	
	wyrmgus::terrain_type *terrain = wyrmgus::terrain_type::get(terrain_ident);
	
	if (value < 0) {
		fprintf(stderr, "Invalid tile value: %d\n", value);
		return;
	}
	
	if (static_cast<int>(CMap::get()->MapLayers.size()) >= z) {
		wyrmgus::tile &mf = *CMap::get()->Field(pos, z);

		mf.set_value(value);
		mf.SetTerrain(terrain);
	}
}

static int CclSetMapTemplateTile(lua_State *l)
{
	const std::string map_template_ident = LuaToString(l, 1);
	wyrmgus::map_template *map_template = wyrmgus::map_template::get(map_template_ident);

	const int x = LuaToNumber(l, 3);
	const int y = LuaToNumber(l, 4);

	const QPoint tile_pos(x, y);

	try {
		const int tile_number = LuaToNumber(l, 2);
		wyrmgus::terrain_type *terrain = wyrmgus::terrain_type::get_by_tile_number(tile_number);

		map_template->set_tile_terrain(tile_pos, terrain);
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Failed to set tile " + wyrmgus::point::to_string(tile_pos) + " for map template \"" + map_template->get_identifier() + "\"."));
	}
	
	return 1;
}

static int CclSetMapTemplateTileTerrain(lua_State *l)
{
	std::string map_template_ident = LuaToString(l, 1);
	wyrmgus::map_template *map_template = wyrmgus::map_template::get_or_add(map_template_ident, nullptr);

	std::string terrain_ident = LuaToString(l, 2);
	wyrmgus::terrain_type *terrain = nullptr;
	if (!terrain_ident.empty()) {
		terrain = wyrmgus::terrain_type::get(terrain_ident);
	}

	Vec2i pos;
	CclGetPos(l, &pos.x, &pos.y, 3);
	
	CDate date;
	const int nargs = lua_gettop(l);
	if (nargs >= 4) {
		CclGetDate(l, &date, 4);
	}
	
	map_template->HistoricalTerrains.push_back(std::tuple<Vec2i, wyrmgus::terrain_type *, CDate>(pos, terrain, date));

	if (nargs >= 5) {
		map_template->TileLabels[std::pair<int, int>(pos.x, pos.y)] = LuaToString(l, 5);
	}
	
	return 1;
}

static int CclSetMapTemplateTileLabel(lua_State *l)
{
	std::string map_template_ident = LuaToString(l, 1);
	wyrmgus::map_template *map_template = wyrmgus::map_template::get_or_add(map_template_ident, nullptr);

	std::string label_string = LuaToString(l, 2);
	
	Vec2i ipos;
	CclGetPos(l, &ipos.x, &ipos.y, 3);

	map_template->TileLabels[std::pair<int, int>(ipos.x, ipos.y)] = TransliterateText(label_string);
	
	return 1;
}

static int CclSetMapTemplatePathway(lua_State *l)
{
	std::string map_template_ident = LuaToString(l, 1);
	wyrmgus::map_template *map_template = wyrmgus::map_template::get_or_add(map_template_ident, nullptr);

	std::string terrain_ident = LuaToString(l, 2);
	wyrmgus::terrain_type *terrain = nullptr;
	if (!terrain_ident.empty()) {
		terrain = wyrmgus::terrain_type::get(terrain_ident);
	}

	Vec2i start_pos;
	if (lua_istable(l, 3)) { //coordinates
		CclGetPos(l, &start_pos.x, &start_pos.y, 3);
	} else { //site ident
		std::string site_ident = LuaToString(l, 3);
		wyrmgus::site *site = wyrmgus::site::get(site_ident);
		start_pos.x = site->get_pos().x();
		start_pos.y = site->get_pos().y();
	}
	
	Vec2i end_pos;
	if (lua_istable(l, 4)) { //coordinates
		CclGetPos(l, &end_pos.x, &end_pos.y, 4);
	} else { //site ident
		std::string site_ident = LuaToString(l, 4);
		wyrmgus::site *site = wyrmgus::site::get(site_ident);
		end_pos.x = site->get_pos().x();
		end_pos.y = site->get_pos().y();
	}
	
	CDate date;
	const int nargs = lua_gettop(l);
	if (nargs >= 5) {
		CclGetDate(l, &date, 5);
	}
	
	Vec2i pos(start_pos);
	Vec2i pathway_length(end_pos - start_pos);
	Vec2i pathway_change(pathway_length.x ? pathway_length.x / abs(pathway_length.x) : 0, pathway_length.y ? pathway_length.y / abs(pathway_length.y) : 0);
	pathway_length.x = abs(pathway_length.x);
	pathway_length.y = abs(pathway_length.y);
	int offset = 0;
	while (pos != end_pos) {
		Vec2i current_length(pos - start_pos);
		current_length.x = abs(current_length.x);
		current_length.y = abs(current_length.y);
		if (pathway_length.x == pathway_length.y) {
			pos += pathway_change;
		} else if (pathway_length.x > pathway_length.y) {
			pos.x += pathway_change.x;
			if (pathway_length.y && pos.y != end_pos.y) {
				if (pathway_length.x % pathway_length.y != 0 && current_length.x % (pathway_length.x / (pathway_length.x % pathway_length.y)) == 0) {
					offset += 1;
				} else if ((current_length.x - offset) % (std::max(1, pathway_length.x / pathway_length.y)) == 0) {
					map_template->HistoricalTerrains.push_back(std::tuple<Vec2i, wyrmgus::terrain_type *, CDate>(Vec2i(pos), terrain, date));
					pos.y += pathway_change.y;
				}
			}
		} else if (pathway_length.y > pathway_length.x) {
			pos.y += pathway_change.y;
			if (pathway_length.x && pos.x != end_pos.x) {
				if (pathway_length.y % pathway_length.x != 0 && current_length.y % (pathway_length.y / (pathway_length.y % pathway_length.x)) == 0) {
					offset += 1;
				} else if ((current_length.y - offset) % (std::max(1, pathway_length.y / pathway_length.x)) == 0) {
					map_template->HistoricalTerrains.push_back(std::tuple<Vec2i, wyrmgus::terrain_type *, CDate>(Vec2i(pos), terrain, date));
					pos.x += pathway_change.x;
				}
			}
		}

		if (pos.x < 0 || pos.x >= map_template->get_width() || pos.y < 0 || pos.y >= map_template->get_height()) {
			break;
		}

		Vec2i pos_diff(end_pos - pos);
		if ((pos_diff.x < 0 && pathway_change.x >= 0) || (pos_diff.x > 0 && pathway_change.x <= 0) || (pos_diff.y < 0 && pathway_change.y >= 0) || (pos_diff.y > 0 && pathway_change.y <= 0)) {
			break;
		}

		map_template->HistoricalTerrains.push_back(std::tuple<Vec2i, wyrmgus::terrain_type *, CDate>(Vec2i(pos), terrain, date));
	}

	return 1;
}

static int CclSetMapTemplateResource(lua_State *l)
{
	std::string map_template_ident = LuaToString(l, 1);
	wyrmgus::map_template *map_template = wyrmgus::map_template::get_or_add(map_template_ident, nullptr);

	lua_pushvalue(l, 2);
	wyrmgus::unit_type *unittype = CclGetUnitType(l);
	if (unittype == nullptr) {
		LuaError(l, "Bad unittype");
	}
	lua_pop(l, 1);
	Vec2i ipos;
	CclGetPos(l, &ipos.x, &ipos.y, 3);

	int resources_held = 0;
	wyrmgus::unique_item *unique = nullptr;
	
	const int nargs = lua_gettop(l);
	if (nargs >= 4) {
		resources_held = LuaToNumber(l, 4);
	}
	if (nargs >= 5) {
		unique = wyrmgus::unique_item::get(LuaToString(l, 5));
	}
	
	map_template->Resources[std::pair<int, int>(ipos.x, ipos.y)] = std::make_tuple(unittype, resources_held, unique);
	
	return 1;
}

static int CclSetMapTemplateUnit(lua_State *l)
{
	std::string map_template_ident = LuaToString(l, 1);
	wyrmgus::map_template *map_template = wyrmgus::map_template::get_or_add(map_template_ident, nullptr);

	lua_pushvalue(l, 2);
	wyrmgus::unit_type *unittype = CclGetUnitType(l);
	if (unittype == nullptr) {
		LuaError(l, "Bad unittype");
	}
	lua_pop(l, 1);
	Vec2i ipos;
	CclGetPos(l, &ipos.x, &ipos.y, 4);

	std::string faction_name = LuaToString(l, 3);
	wyrmgus::faction *faction = wyrmgus::faction::try_get(faction_name);

	CDate start_date;
	CDate end_date;

	wyrmgus::unique_item *unique = nullptr;

	const int nargs = lua_gettop(l);
	if (nargs >= 5) {
		CclGetDate(l, &start_date, 5);
	}
	if (nargs >= 6) {
		CclGetDate(l, &end_date, 6);
	}
	if (nargs >= 7) {
		unique = wyrmgus::unique_item::get(LuaToString(l, 7));
	}
	
	map_template->Units.push_back(std::make_tuple(ipos, unittype, faction, start_date, end_date, unique));
	
	return 1;
}

static int CclSetMapTemplateHero(lua_State *l)
{
	std::string map_template_ident = LuaToString(l, 1);
	wyrmgus::map_template *map_template = wyrmgus::map_template::get_or_add(map_template_ident, nullptr);

	wyrmgus::character *hero = wyrmgus::character::get(LuaToString(l, 2));

	Vec2i ipos;
	CclGetPos(l, &ipos.x, &ipos.y, 4);

	std::string faction_name = LuaToString(l, 3);
	wyrmgus::faction *faction = wyrmgus::faction::try_get(faction_name);
	if (!faction_name.empty() && !faction) {
		LuaError(l, "Faction \"%s\" doesn't exist.\n" _C_ faction_name.c_str());
	}

	CDate start_date;
	CDate end_date;
	const int nargs = lua_gettop(l);
	if (nargs >= 5) {
		CclGetDate(l, &start_date, 5);
	}
	if (nargs >= 6) {
		CclGetDate(l, &end_date, 6);
	}
	
	map_template->Heroes.push_back(std::tuple<Vec2i, wyrmgus::character *, wyrmgus::faction *, CDate, CDate>(ipos, hero, faction, start_date, end_date));
	
	return 1;
}

static int CclSetMapTemplateLayerConnector(lua_State *l)
{
	std::string map_template_ident = LuaToString(l, 1);
	wyrmgus::map_template *map_template = wyrmgus::map_template::get_or_add(map_template_ident, nullptr);

	lua_pushvalue(l, 2);
	wyrmgus::unit_type *unittype = CclGetUnitType(l);
	if (unittype == nullptr) {
		LuaError(l, "Bad unittype");
	}
	lua_pop(l, 1);
	Vec2i ipos;
	CclGetPos(l, &ipos.x, &ipos.y, 3);

	wyrmgus::unique_item *unique = nullptr;
	
	const int nargs = lua_gettop(l);
	if (nargs >= 5) {
		unique = wyrmgus::unique_item::get(LuaToString(l, 5));
	}
	
	if (lua_isstring(l, 4)) {
		std::string realm = LuaToString(l, 4);
		if (wyrmgus::world::try_get(realm)) {
			map_template->WorldConnectors.push_back(std::make_tuple(ipos, unittype, wyrmgus::world::get(realm), unique));
		} else {
			LuaError(l, "incorrect argument");
		}
	} else {
		LuaError(l, "incorrect argument");
	}
	
	return 1;
}

void ApplyMapTemplate(const std::string &map_template_ident, int template_start_x, int template_start_y, int map_start_x, int map_start_y, int z)
{
	wyrmgus::map_template *map_template = wyrmgus::map_template::get(map_template_ident);

	map_template->apply(Vec2i(template_start_x, template_start_y), Vec2i(map_start_x, map_start_y), z);
}

void ApplyCampaignMap(const std::string &campaign_ident)
{
	try {
		if (SaveGameLoading) {
			return;
		}

		database::load_history();

		const campaign *campaign = campaign::get(campaign_ident);

		for (size_t i = 0; i < campaign->get_map_templates().size(); ++i) {
			map_template *map_template = campaign->get_map_templates()[i];
			QPoint start_pos(0, 0);
			if (i < campaign->MapTemplateStartPos.size()) {
				start_pos = campaign->MapTemplateStartPos[i];
			}

			try {
				map_template->apply(start_pos, QPoint(0, 0), i);
			} catch (...) {
				std::throw_with_nested(std::runtime_error("Failed to apply map template \"" + map_template->get_identifier() + "\"."));
			}
		}
	} catch (const std::exception &exception) {
		exception::report(exception);
		log::log_error("Failed to apply campaign map for campaign \"" + campaign_ident + "\".");
		std::terminate();
	}
}
//Wyrmgus end

/**
**  Define the type of each player available for the map
**
**  @param l  Lua state.
*/
int CclDefinePlayerTypes(lua_State *l)
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
		const std::string type = LuaToString(l, i + 1);
		CMap::get()->Info->player_types[i] = string_to_player_type(type);
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
	CMap::get()->TileModelsFileName = LuaToString(l, 1);
	const std::string filename = LibraryFileName(CMap::get()->TileModelsFileName.c_str());
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
	CMap::get()->Tileset->parse(l);

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

	CMap::get()->Tileset->buildTable();
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

	if (tilenumber >= CMap::get()->Tileset->tiles.size()) {
		LuaError(l, "Accessed a tile that's not defined");
	}
	int j = 0;
	tile_flag flags = tile_flag::none;

	ParseTilesetTileFlags(l, &flags, &j);
	CMap::get()->Tileset->tiles[tilenumber].flag = flags;
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
	const tileset &tileset = *CMap::get()->Tileset;
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
	//Wyrmgus start
//	LuaCheckArgs(l, 2);
	int z = 0;
	const int nargs = lua_gettop(l);
	if (nargs >= 3) {
		z = LuaToNumber(l, 3);
	}
	//Wyrmgus end

	const Vec2i pos(LuaToNumber(l, 1), LuaToNumber(l, 2));

	lua_pushstring(l, CMap::get()->GetTileTopTerrain(pos, false, z)->get_identifier().c_str());

	return 1;
}

/**
**  Check if the tile's terrain has a particular flag.
**
**  @param l  Lua state.
**
**  @return   True if has the flag, false if not.
*/
static int CclGetTileTerrainHasFlag(lua_State *l)
{
	//Wyrmgus start
//	LuaCheckArgs(l, 3);
	int z = 0;
	const int nargs = lua_gettop(l);
	if (nargs >= 4) {
		z = LuaToNumber(l, 4);
	}
	//Wyrmgus end

	const Vec2i pos(LuaToNumber(l, 1), LuaToNumber(l, 2));

	//Wyrmgus start
	if (pos.x < 0 || pos.x >= CMap::get()->Info->MapWidths[z] || pos.y < 0 || pos.y >= CMap::get()->Info->MapHeights[z]) {
		lua_pushboolean(l, 0);
		return 1;
	}
	//Wyrmgus end

	const std::string flag_name = LuaToString(l, 3);
	const tile_flag flag = string_to_tile_flag(flag_name);

	//Wyrmgus start
//	const wyrmgus::tile &mf = *CMap::get()->Field(pos);
	const wyrmgus::tile &mf = *CMap::get()->Field(pos, z);
	//Wyrmgus end

	if (mf.has_flag(flag)) {
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
int CclDefineTerrainType(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string terrain_ident = LuaToString(l, 1);
	wyrmgus::terrain_type *terrain = wyrmgus::terrain_type::get_or_add(terrain_ident, nullptr);
	
	std::string graphics_file;
	std::string elevation_graphics_file;
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Name")) {
			terrain->set_name(LuaToString(l, -1));
		} else if (!strcmp(value, "Character")) {
			const std::string character_str = LuaToString(l, -1);
			terrain->set_character(character_str.front());
		} else if (!strcmp(value, "CharacterAliases")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				const std::string character_str = LuaToString(l, -1, j + 1);
				const char c = character_str.front();
				terrain->map_to_character(c);
			}
		} else if (!strcmp(value, "Color")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			terrain->set_color(QColor(LuaToNumber(l, -1, 1), LuaToNumber(l, -1, 2), LuaToNumber(l, -1, 3)));
		} else if (!strcmp(value, "Overlay")) {
			terrain->overlay = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Buildable")) {
			terrain->buildable = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "AllowSingle")) {
			terrain->allow_single = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Hidden")) {
			terrain->hidden = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "SolidAnimationFrames")) {
			terrain->SolidAnimationFrames = LuaToNumber(l, -1);
		} else if (!strcmp(value, "Resource")) {
			terrain->resource = wyrmgus::resource::get(LuaToString(l, -1));
		} else if (!strcmp(value, "BaseTerrainTypes")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				wyrmgus::terrain_type *base_terrain = wyrmgus::terrain_type::get_or_add(LuaToString(l, -1, j + 1), nullptr);
				terrain->add_base_terrain_type(base_terrain);
			}
		} else if (!strcmp(value, "InnerBorderTerrains")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				wyrmgus::terrain_type *border_terrain = wyrmgus::terrain_type::get(LuaToString(l, -1, j + 1));
				terrain->add_inner_border_terrain_type(border_terrain);
			}
		} else if (!strcmp(value, "OuterBorderTerrains")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				wyrmgus::terrain_type *border_terrain = wyrmgus::terrain_type::get(LuaToString(l, -1, j + 1));
				terrain->add_outer_border_terrain_type(border_terrain);
			}
		} else if (!strcmp(value, "OverlayTerrains")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				wyrmgus::terrain_type *overlay_terrain = wyrmgus::terrain_type::get(LuaToString(l, -1, j + 1));
				overlay_terrain->add_base_terrain_type(terrain);
			}
		} else if (!strcmp(value, "Flags")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			terrain->Flags = tile_flag::none;
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				const std::string tile_flag_str = LuaToString(l, -1, j + 1);
				const tile_flag tile_flag = string_to_tile_flag(tile_flag_str);
				terrain->Flags |= tile_flag;
			}
		} else if (!strcmp(value, "Graphics")) {
			graphics_file = LuaToString(l, -1);
			if (!CanAccessFile(graphics_file.c_str())) {
				LuaError(l, "File \"%s\" doesn't exist." _C_ graphics_file.c_str());
			}
		} else if (!strcmp(value, "ElevationGraphics")) {
			elevation_graphics_file = LuaToString(l, -1);
			if (!CanAccessFile(elevation_graphics_file.c_str())) {
				LuaError(l, "File \"%s\" doesn't exist." _C_ elevation_graphics_file.c_str());
			}
		} else if (!strcmp(value, "SolidTiles")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				terrain->solid_tiles.push_back(LuaToNumber(l, -1, j + 1));
			}
		} else if (!strcmp(value, "DamagedTiles")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				terrain->damaged_tiles.push_back(LuaToNumber(l, -1, j + 1));
			}
		} else if (!strcmp(value, "DestroyedTiles")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				terrain->destroyed_tiles.push_back(LuaToNumber(l, -1, j + 1));
			}
		} else if (!strcmp(value, "TransitionTiles")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				std::string transition_terrain_name = LuaToString(l, -1, j + 1);
				wyrmgus::terrain_type *transition_terrain = nullptr;
				if (transition_terrain_name != "any") {
					transition_terrain = wyrmgus::terrain_type::get(transition_terrain_name);
				}
				++j;
				
				const wyrmgus::tile_transition_type transition_type = GetTransitionTypeIdByName(LuaToString(l, -1, j + 1));
				++j;
				
				terrain->transition_tiles[transition_terrain][transition_type].push_back(LuaToNumber(l, -1, j + 1));
			}
		} else if (!strcmp(value, "AdjacentTransitionTiles")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				std::string transition_terrain_name = LuaToString(l, -1, j + 1);
				wyrmgus::terrain_type *transition_terrain = nullptr;
				if (transition_terrain_name != "any") {
					transition_terrain = wyrmgus::terrain_type::get(transition_terrain_name);
				}
				++j;
				
				const wyrmgus::tile_transition_type transition_type = GetTransitionTypeIdByName(LuaToString(l, -1, j + 1));
				++j;
				
				terrain->adjacent_transition_tiles[transition_terrain][transition_type].push_back(LuaToNumber(l, -1, j + 1));
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	if (!graphics_file.empty()) {
		terrain->graphics = CPlayerColorGraphic::New(graphics_file, defines::get()->get_tile_size(), nullptr);
	}
	if (!elevation_graphics_file.empty()) {
		terrain->elevation_graphics = CGraphic::New(elevation_graphics_file, defines::get()->get_tile_size());
	}
	
	return 0;
}

/**
**  Define a map template.
**
**  @param l  Lua state.
*/
int CclDefineMapTemplate(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string map_template_ident = LuaToString(l, 1);
	wyrmgus::map_template *map_template = wyrmgus::map_template::get_or_add(map_template_ident, nullptr);
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Name")) {
			map_template->set_name(LuaToString(l, -1));
		} else if (!strcmp(value, "World")) {
			wyrmgus::world *world = world::get(LuaToString(l, -1));
			map_template->world = world;
		} else if (!strcmp(value, "TerrainFile")) {
			map_template->terrain_file = LuaToString(l, -1);
		} else if (!strcmp(value, "OverlayTerrainFile")) {
			map_template->overlay_terrain_file = LuaToString(l, -1);
		} else if (!strcmp(value, "TerrainImage")) {
			map_template->terrain_file = LuaToString(l, -1);
		} else if (!strcmp(value, "OverlayTerrainImage")) {
			map_template->overlay_terrain_file = LuaToString(l, -1);
		} else if (!strcmp(value, "Width")) {
			map_template->size.setWidth(LuaToNumber(l, -1));
		} else if (!strcmp(value, "Height")) {
			map_template->size.setHeight(LuaToNumber(l, -1));
		} else if (!strcmp(value, "OutputTerrainImage")) {
			map_template->output_terrain_image = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "SubtemplatePosition")) {
			Vec2i subtemplate_pos;
			CclGetPos(l, &subtemplate_pos.x, &subtemplate_pos.y);
			map_template->subtemplate_center_pos = subtemplate_pos;
		} else if (!strcmp(value, "SubtemplatePositionTopLeft")) {
			Vec2i subtemplate_pos;
			CclGetPos(l, &subtemplate_pos.x, &subtemplate_pos.y);
			map_template->subtemplate_top_left_pos = subtemplate_pos;
		} else if (!strcmp(value, "MainTemplate")) {
			wyrmgus::map_template *main_template = wyrmgus::map_template::get(LuaToString(l, -1));
			map_template->set_main_template(main_template);
		} else if (!strcmp(value, "BaseTerrainType")) {
			wyrmgus::terrain_type *terrain_type = wyrmgus::terrain_type::get(LuaToString(l, -1));
			map_template->base_terrain_type = terrain_type;
		} else if (!strcmp(value, "BaseOverlayTerrainType")) {
			wyrmgus::terrain_type *terrain_type = wyrmgus::terrain_type::get(LuaToString(l, -1));
			map_template->base_overlay_terrain_type = terrain_type;
		} else if (!strcmp(value, "SurroundingTerrainType")) {
			wyrmgus::terrain_type *terrain_type = wyrmgus::terrain_type::get(LuaToString(l, -1));
			map_template->surrounding_terrain_type = terrain_type;
		} else if (!strcmp(value, "GeneratedNeutralUnits")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				wyrmgus::unit_type *unit_type = wyrmgus::unit_type::get(LuaToString(l, -1, j + 1));
				++j;
				
				int quantity = LuaToNumber(l, -1, j + 1);
				
				map_template->GeneratedNeutralUnits.push_back(std::pair<wyrmgus::unit_type *, int>(unit_type, quantity));
			}
		} else if (!strcmp(value, "PlayerLocationGeneratedNeutralUnits")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				wyrmgus::unit_type *unit_type = wyrmgus::unit_type::get(LuaToString(l, -1, j + 1));
				++j;
				
				int quantity = LuaToNumber(l, -1, j + 1);
				
				map_template->PlayerLocationGeneratedNeutralUnits.push_back(std::pair<wyrmgus::unit_type *, int>(unit_type, quantity));
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	return 0;
}

/**
**  Define a site.
**
**  @param l  Lua state.
*/
int CclDefineSite(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string site_ident = LuaToString(l, 1);
	wyrmgus::site *site = wyrmgus::site::add(site_ident, nullptr);
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Name")) {
			site->set_name(LuaToString(l, -1));
		} else if (!strcmp(value, "BaseUnitType")) {
			site->base_unit_type = wyrmgus::unit_type::get(LuaToString(l, -1));
		} else if (!strcmp(value, "Position")) {
			Vec2i pos;
			CclGetPos(l, &pos.x, &pos.y);
			site->pos = pos;
		} else if (!strcmp(value, "MapTemplate")) {
			wyrmgus::map_template *map_template = wyrmgus::map_template::get(LuaToString(l, -1));
			site->map_template = map_template;
		} else if (!strcmp(value, "CulturalNames")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				wyrmgus::civilization *civilization = wyrmgus::civilization::get(LuaToString(l, -1, j + 1));
				++j;

				std::string cultural_name = LuaToString(l, -1, j + 1);
				
				site->cultural_names[civilization] = cultural_name;
			}
		} else if (!strcmp(value, "HistoricalOwners")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				CDate date;
				lua_rawgeti(l, -1, j + 1);
				CclGetDate(l, &date);
				lua_pop(l, 1);
				++j;
				std::string owner_ident = LuaToString(l, -1, j + 1);
				if (!owner_ident.empty()) {
					wyrmgus::faction *owner_faction = wyrmgus::faction::get(owner_ident);
					site->HistoricalOwners[date] = owner_faction;
				} else {
					site->HistoricalOwners[date] = nullptr;
				}
			}
		} else if (!strcmp(value, "HistoricalUnits")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				CDate start_date;
				lua_rawgeti(l, -1, j + 1);
				CclGetDate(l, &start_date);
				lua_pop(l, 1);
				++j;
				CDate end_date;
				lua_rawgeti(l, -1, j + 1);
				CclGetDate(l, &end_date);
				lua_pop(l, 1);
				++j;
				
				wyrmgus::unit_type *unit_type = wyrmgus::unit_type::get(LuaToString(l, -1, j + 1));
				++j;
				
				int unit_quantity = LuaToNumber(l, -1, j + 1);
				++j;
				
				wyrmgus::faction *unit_owner = nullptr;
				lua_rawgeti(l, -1, j + 1);
				if (lua_isstring(l, -1) && !lua_isnumber(l, -1)) {
					unit_owner = wyrmgus::faction::get(LuaToString(l, -1));
				} else {
					--j;
				}
				lua_pop(l, 1);

				site->HistoricalUnits.push_back(std::tuple<CDate, CDate, wyrmgus::unit_type *, int, wyrmgus::faction *>(start_date, end_date, unit_type, unit_quantity, unit_owner));
			}
		} else if (!strcmp(value, "HistoricalBuildings")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				CDate start_date;
				lua_rawgeti(l, -1, j + 1);
				CclGetDate(l, &start_date);
				lua_pop(l, 1);
				++j;
				CDate end_date;
				lua_rawgeti(l, -1, j + 1);
				CclGetDate(l, &end_date);
				lua_pop(l, 1);
				++j;
				const wyrmgus::unit_class *building_class = wyrmgus::unit_class::get(LuaToString(l, -1, j + 1));
				++j;
				
				wyrmgus::unique_item *unique = nullptr;
				lua_rawgeti(l, -1, j + 1);
				if (lua_isstring(l, -1) && !lua_isnumber(l, -1) && wyrmgus::unique_item::try_get(LuaToString(l, -1)) != nullptr) {
					unique = wyrmgus::unique_item::get(LuaToString(l, -1));
				} else {
					--j;
				}
				lua_pop(l, 1);
				++j;
				
				wyrmgus::faction *building_owner = nullptr;
				lua_rawgeti(l, -1, j + 1);
				if (lua_isstring(l, -1) && !lua_isnumber(l, -1)) {
					building_owner = wyrmgus::faction::get(LuaToString(l, -1));
				} else {
					--j;
				}
				lua_pop(l, 1);

				site->HistoricalBuildings.push_back(std::make_tuple(start_date, end_date, building_class, unique, building_owner));
			}
		} else if (!strcmp(value, "Regions")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				wyrmgus::region *region = wyrmgus::region::get(LuaToString(l, -1, j + 1));
				site->regions.push_back(region);
				region->add_site(site);
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	return 0;
}

/**
**  Define a terrain feature.
**
**  @param l  Lua state.
*/
int CclDefineTerrainFeature(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string terrain_feature_ident = LuaToString(l, 1);
	wyrmgus::terrain_feature *terrain_feature = wyrmgus::terrain_feature::get_or_add(terrain_feature_ident, nullptr);
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Name")) {
			terrain_feature->set_name(LuaToString(l, -1));
		} else if (!strcmp(value, "Color")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			QColor color;
			color.setRed(LuaToNumber(l, -1, 1));
			color.setGreen(LuaToNumber(l, -1, 2));
			color.setBlue(LuaToNumber(l, -1, 3));
			terrain_feature->set_color(color);
		} else if (!strcmp(value, "TerrainType")) {
			wyrmgus::terrain_type *terrain = wyrmgus::terrain_type::get(LuaToString(l, -1));
			terrain_feature->terrain_type = terrain;
		} else if (!strcmp(value, "CulturalNames")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				const wyrmgus::civilization *civilization = wyrmgus::civilization::get(LuaToString(l, -1, j + 1));
				++j;

				std::string cultural_name = LuaToString(l, -1, j + 1);
				
				terrain_feature->cultural_names[civilization] = cultural_name;
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	return 0;
}

/**
**  Get map template data.
**
**  @param l  Lua state.
*/
static int CclGetMapTemplateData(lua_State *l)
{
	if (lua_gettop(l) < 2) {
		LuaError(l, "incorrect argument");
	}
	std::string map_template_ident = LuaToString(l, 1);
	wyrmgus::map_template *map_template = wyrmgus::map_template::get(map_template_ident);
	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Name")) {
		lua_pushstring(l, map_template->get_name().c_str());
		return 1;
	} else if (!strcmp(data, "World")) {
		if (map_template->get_world() != nullptr) {
			lua_pushstring(l, map_template->get_world()->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "CurrentStartPosX")) {
		lua_pushnumber(l, map_template->current_start_pos.x());
		return 1;
	} else if (!strcmp(data, "CurrentStartPosY")) {
		lua_pushnumber(l, map_template->current_start_pos.y());
		return 1;
	} else if (!strcmp(data, "MapStartPosX")) {
		Vec2i pos = CMap::get()->get_subtemplate_pos(map_template);
		lua_pushnumber(l, pos.x);
		return 1;
	} else if (!strcmp(data, "MapStartPosY")) {
		Vec2i pos = CMap::get()->get_subtemplate_pos(map_template);
		lua_pushnumber(l, pos.y);
		return 1;
	} else if (!strcmp(data, "MapEndPosX")) {
		Vec2i pos = CMap::get()->get_subtemplate_end_pos(map_template);
		lua_pushnumber(l, pos.x);
		return 1;
	} else if (!strcmp(data, "MapEndPosY")) {
		Vec2i pos = CMap::get()->get_subtemplate_end_pos(map_template);
		lua_pushnumber(l, pos.y);
		return 1;
	} else if (!strcmp(data, "MapLayer")) {
		const CMapLayer *map_layer = CMap::get()->get_subtemplate_map_layer(map_template);
		if (map_layer) {
			lua_pushnumber(l, map_layer->ID);
		} else {
			lua_pushnumber(l, -1);
		}
		return 1;
	} else {
		LuaError(l, "Invalid field: %s" _C_ data);
	}

	return 0;
}

/**
**  Get site data.
**
**  @param l  Lua state.
*/
static int CclGetSiteData(lua_State *l)
{
	if (lua_gettop(l) < 2) {
		LuaError(l, "incorrect argument");
	}
	const std::string site_ident = LuaToString(l, 1);
	const wyrmgus::site *site = wyrmgus::site::get(site_ident);
	const char *data = LuaToString(l, 2);

	const wyrmgus::site_game_data *site_game_data = site->get_game_data();

	if (!strcmp(data, "Name")) {
		lua_pushstring(l, site->get_name().c_str());
		return 1;
	} else if (!strcmp(data, "PosX")) {
		lua_pushnumber(l, site->get_pos().x());
		return 1;
	} else if (!strcmp(data, "PosY")) {
		lua_pushnumber(l, site->get_pos().y());
		return 1;
	} else if (!strcmp(data, "MapPosX")) {
		if (site_game_data->get_site_unit() != nullptr) {
			lua_pushnumber(l, site_game_data->get_site_unit()->tilePos.x);
		} else {
			lua_pushnumber(l, site_game_data->get_map_pos().x());
		}
		return 1;
	} else if (!strcmp(data, "MapPosY")) {
		if (site_game_data->get_site_unit() != nullptr) {
			lua_pushnumber(l, site_game_data->get_site_unit()->tilePos.y);
		} else {
			lua_pushnumber(l, site_game_data->get_map_pos().y());
		}
		return 1;
	} else if (!strcmp(data, "MapCenterPosX")) {
		if (site_game_data->get_site_unit() != nullptr) {
			lua_pushnumber(l, site_game_data->get_site_unit()->get_center_tile_pos().x());
		} else {
			lua_pushnumber(l, site_game_data->get_map_pos().x());
		}
		return 1;
	} else if (!strcmp(data, "MapCenterPosY")) {
		if (site_game_data->get_site_unit() != nullptr) {
			lua_pushnumber(l, site_game_data->get_site_unit()->get_center_tile_pos().y());
		} else {
			lua_pushnumber(l, site_game_data->get_map_pos().y());
		}
		return 1;
	} else if (!strcmp(data, "MapLayer")) {
		if (site_game_data->get_site_unit() != nullptr && site_game_data->get_site_unit()->MapLayer != nullptr) {
			lua_pushnumber(l, site_game_data->get_site_unit()->MapLayer->ID);
		} else if (site_game_data->get_map_layer() != nullptr) {
			lua_pushnumber(l, site_game_data->get_map_layer()->ID);
		} else {
			lua_pushnumber(l, -1);
		}
		return 1;
	} else if (!strcmp(data, "SiteUnit")) {
		if (site_game_data->get_site_unit() != nullptr) {
			lua_pushnumber(l, UnitNumber(*site_game_data->get_site_unit()));
		} else {
			lua_pushnumber(l, -1);
		}
		return 1;
	} else {
		LuaError(l, "Invalid field: %s" _C_ data);
	}

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

	lua_register(Lua, "SetFogOfWarGraphics", CclSetFogOfWarGraphics);
	lua_register(Lua, "SetFogOfWarOpacity", CclSetFogOfWarOpacity);
	lua_register(Lua, "SetFogOfWarColor", CclSetFogOfWarColor);
	
	lua_register(Lua, "LoadTileModels", CclLoadTileModels);
	lua_register(Lua, "DefinePlayerTypes", CclDefinePlayerTypes);

	lua_register(Lua, "DefineTileset", CclDefineTileset);
	lua_register(Lua, "SetTileFlags", CclSetTileFlags);
	lua_register(Lua, "BuildTilesetTables", CclBuildTilesetTables);

	//Wyrmgus start
	lua_register(Lua, "GetCurrentTileset", CclGetCurrentTileset);
	//Wyrmgus end
	lua_register(Lua, "GetTileTerrainName", CclGetTileTerrainName);
	lua_register(Lua, "GetTileTerrainHasFlag", CclGetTileTerrainHasFlag);
	
	//Wyrmgus start
	lua_register(Lua, "DefineTerrainType", CclDefineTerrainType);
	lua_register(Lua, "DefineMapTemplate", CclDefineMapTemplate);
	lua_register(Lua, "DefineSite", CclDefineSite);
	lua_register(Lua, "DefineTerrainFeature", CclDefineTerrainFeature);
	lua_register(Lua, "GetMapTemplateData", CclGetMapTemplateData);
	lua_register(Lua, "GetSiteData", CclGetSiteData);
	lua_register(Lua, "SetMapTemplateTile", CclSetMapTemplateTile);
	lua_register(Lua, "SetMapTemplateTileTerrain", CclSetMapTemplateTileTerrain);
	lua_register(Lua, "SetMapTemplateTileLabel", CclSetMapTemplateTileLabel);
	lua_register(Lua, "SetMapTemplatePathway", CclSetMapTemplatePathway);
	lua_register(Lua, "SetMapTemplateResource", CclSetMapTemplateResource);
	lua_register(Lua, "SetMapTemplateUnit", CclSetMapTemplateUnit);
	lua_register(Lua, "SetMapTemplateHero", CclSetMapTemplateHero);
	lua_register(Lua, "SetMapTemplateLayerConnector", CclSetMapTemplateLayerConnector);
//	lua_register(Lua, "CreateMapTemplateTerrainFile", CclCreateMapTemplateTerrainFile);
	//Wyrmgus end
}
