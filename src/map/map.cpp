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
/**@name map.cpp - The map source file. */
//
//      (c) Copyright 1998-2020 by Lutz Sammer, Vladi Shabanski,
//                                 Francois Beerten and Andrettin
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

#include "map/map.h"

//Wyrmgus start
#include "editor.h"
#include "game.h" // for the SaveGameLoading variable
//Wyrmgus end
#include "iolib.h"
#include "map/map_layer.h"
#include "map/map_template.h"
#include "map/site.h"
#include "map/terrain_type.h"
#include "map/tileset.h"
#include "plane.h"
#include "player.h"
//Wyrmgus start
#include "province.h"
#include "quest.h"
#include "settings.h"
#include "sound/sound_server.h"
//Wyrmgus end
#include "time/calendar.h"
#include "time/season.h"
#include "time/season_schedule.h"
#include "time/time_of_day.h"
#include "time/time_of_day_schedule.h"
//Wyrmgus start
#include "translate.h"
//Wyrmgus end
#include "ui/ui.h"
#include "unit/unit.h"
//Wyrmgus start
#include "unit/unit_find.h"
//Wyrmgus end
#include "unit/unit_manager.h"
#include "unit/unit_type_type.h"
//Wyrmgus start
#include "upgrade/upgrade.h"
//Wyrmgus end
#include "util/vector_util.h"
#include "version.h"
#include "video.h"
#include "world.h"

#ifdef USE_OAML
#include <oaml.h>

extern oamlApi *oaml;
extern bool enableOAML;
#endif

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

//Wyrmgus start
std::vector<CTerrainFeature *> TerrainFeatures;
std::map<std::string, CTerrainFeature *> TerrainFeatureIdentToPointer;
std::map<std::tuple<int, int, int>, int> TerrainFeatureColorToIndex;
//Wyrmgus end
CMap CMap::Map; //the current map
int FlagRevealMap; //flag must reveal the map
int ReplayRevealMap; //reveal Map is replay
int ForestRegeneration; //forest regeneration
char CurrentMapPath[1024]; //path of the current map

//Wyrmgus start
/**
**  Get a terrain feature
*/
CTerrainFeature *GetTerrainFeature(const std::string &terrain_feature_ident)
{
	if (terrain_feature_ident.empty()) {
		return nullptr;
	}
	
	std::map<std::string, CTerrainFeature *>::const_iterator find_iterator = TerrainFeatureIdentToPointer.find(terrain_feature_ident);
	
	if (find_iterator != TerrainFeatureIdentToPointer.end()) {
		return find_iterator->second;
	}
	
	return nullptr;
}
//Wyrmgus end

/*----------------------------------------------------------------------------
--  Visible and explored handling
----------------------------------------------------------------------------*/

/**
**  Marks seen tile -- used mainly for the Fog Of War
**
**  @param mf  MapField-position.
*/
//Wyrmgus start
//void CMap::MarkSeenTile(CMapField &mf)
void CMap::MarkSeenTile(CMapField &mf, int z)
//Wyrmgus end
{
	//Wyrmgus start
//	const unsigned int tile = mf.getGraphicTile();
//	const unsigned int seentile = mf.playerInfo.SeenTile;
	//Wyrmgus end

	//  Nothing changed? Seeing already the correct tile.
	//Wyrmgus start
//	if (tile == seentile) {
	if (mf.IsSeenTileCorrect()) {
	//Wyrmgus end
		return;
	}
	//Wyrmgus start
//	mf.playerInfo.SeenTile = tile;
	mf.UpdateSeenTile();
	//Wyrmgus end

#ifdef MINIMAP_UPDATE
	//rb - GRRRRRRRRRRRR
	//Wyrmgus start
//	const unsigned int index = &mf - Map.Fields;
//	const int y = index / Info.MapWidth;
//	const int x = index - (y * Info.MapWidth);
	const CMapLayer *map_layer = Map.MapLayers[z];
	const unsigned int index = &mf - map_layer->Fields;
	const int y = index / map_layer->GetWidth();
	const int x = index - (y * map_layer->GetWidth());
	//Wyrmgus end
	const Vec2i pos = {x, y}
#endif

	//Wyrmgus start
	/*
	if (this->Tileset->TileTypeTable.empty() == false) {
#ifndef MINIMAP_UPDATE
		//rb - GRRRRRRRRRRRR
		const unsigned int index = &mf - Map.Fields;
		const int y = index / Info.MapWidth;
		const int x = index - (y * Info.MapWidth);
		const Vec2i pos(x, y);
#endif

		//  Handle wood changes. FIXME: check if for growing wood correct?
		if (tile == this->Tileset->getRemovedTreeTile()) {
			FixNeighbors(MapFieldForest, 1, pos);
		} else if (seentile == this->Tileset->getRemovedTreeTile()) {
			FixTile(MapFieldForest, 1, pos);
		} else if (mf.ForestOnMap()) {
			FixTile(MapFieldForest, 1, pos);
			FixNeighbors(MapFieldForest, 1, pos);

			// Handle rock changes.
		} else if (tile == Tileset->getRemovedRockTile()) {
			FixNeighbors(MapFieldRocks, 1, pos);
		} else if (seentile == Tileset->getRemovedRockTile()) {
			FixTile(MapFieldRocks, 1, pos);
		} else if (mf.RockOnMap()) {
			FixTile(MapFieldRocks, 1, pos);
			FixNeighbors(MapFieldRocks, 1, pos);

			//  Handle Walls changes.
		} else if (this->Tileset->isAWallTile(tile)
				   || this->Tileset->isAWallTile(seentile)) {
		//Wyrmgus end
			MapFixSeenWallTile(pos);
			MapFixSeenWallNeighbors(pos);
		}
	}
	*/
	//Wyrmgus end

#ifdef MINIMAP_UPDATE
	//Wyrmgus start
//	UI.Minimap.UpdateXY(pos);
	UI.Minimap.UpdateXY(pos, z);
	//Wyrmgus end
#endif
}

/**
**  Reveal the entire map.
*/
//Wyrmgus start
//void CMap::Reveal()
void CMap::Reveal(bool only_person_players)
//Wyrmgus end
{
	//  Mark every explored tile as visible. 1 turns into 2.
	//Wyrmgus start
	/*
	for (int i = 0; i != this->Info.MapWidth * this->Info.MapHeight; ++i) {
		CMapField &mf = *this->Field(i);
		CMapFieldPlayerInfo &playerInfo = mf.playerInfo;
		for (int p = 0; p < PlayerMax; ++p) {
			//Wyrmgus start
//			playerInfo.Visible[p] = std::max<unsigned short>(1, playerInfo.Visible[p]);
			if (Players[p].Type == PlayerPerson || !only_person_players) {
				playerInfo.Visible[p] = std::max<unsigned short>(1, playerInfo.Visible[p]);
			}
			//Wyrmgus end
		}
		MarkSeenTile(mf);
	}
	*/
	for (size_t z = 0; z < this->MapLayers.size(); ++z) {
		for (int i = 0; i != this->Info.MapWidths[z] * this->Info.MapHeights[z]; ++i) {
			CMapField &mf = *this->Field(i, z);
			CMapFieldPlayerInfo &playerInfo = mf.playerInfo;
			for (int p = 0; p < PlayerMax; ++p) {
				if (CPlayer::Players[p]->Type == PlayerPerson || !only_person_players) {
					playerInfo.Visible[p] = std::max<unsigned short>(1, playerInfo.Visible[p]);
				}
			}
			MarkSeenTile(mf, z);
		}
	}
	//Wyrmgus end
	//  Global seen recount. Simple and effective.
	for (CUnitManager::Iterator it = UnitManager.begin(); it != UnitManager.end(); ++it) {
		CUnit &unit = **it;
		//  Reveal neutral buildings. Gold mines:)
		if (unit.Player->Type == PlayerNeutral) {
			for (int p = 0; p < PlayerMax; ++p) {
				//Wyrmgus start
//				if (CPlayer::Players[p]->Type != PlayerNobody && (!(unit.Seen.ByPlayer & (1 << p)))) {
				if (CPlayer::Players[p]->Type != PlayerNobody && (CPlayer::Players[p]->Type == PlayerPerson || !only_person_players) && (!(unit.Seen.ByPlayer & (1 << p)))) {
				//Wyrmgus end
					UnitGoesOutOfFog(unit, *CPlayer::Players[p]);
					UnitGoesUnderFog(unit, *CPlayer::Players[p]);
				}
			}
		}
		UnitCountSeen(unit);
	}
}

/*----------------------------------------------------------------------------
--  Map queries
----------------------------------------------------------------------------*/

Vec2i CMap::MapPixelPosToTilePos(const PixelPos &mapPos, const int map_layer) const
{
	const Vec2i tilePos(mapPos.x / GetMapLayerPixelTileSize(map_layer).x, mapPos.y / GetMapLayerPixelTileSize(map_layer).y);

	return tilePos;
}

PixelPos CMap::TilePosToMapPixelPos_TopLeft(const Vec2i &tilePos, const CMapLayer *map_layer) const
{
	PixelPos mapPixelPos(tilePos.x * GetMapLayerPixelTileSize(map_layer ? map_layer->ID : -1).x, tilePos.y * GetMapLayerPixelTileSize(map_layer ? map_layer->ID : -1).y);

	return mapPixelPos;
}

PixelPos CMap::TilePosToMapPixelPos_Center(const Vec2i &tilePos, const CMapLayer *map_layer) const
{
	return TilePosToMapPixelPos_TopLeft(tilePos, map_layer) + GetMapLayerPixelTileSize(map_layer ? map_layer->ID : -1) / 2;
}

//Wyrmgus start
CTerrainType *CMap::GetTileTerrain(const Vec2i &pos, const bool overlay, const int z) const
{
	if (!Map.Info.IsPointOnMap(pos, z)) {
		return nullptr;
	}
	
	CMapField &mf = *this->Field(pos, z);
	
	return mf.GetTerrain(overlay);
}

CTerrainType *CMap::GetTileTopTerrain(const Vec2i &pos, const bool seen, const int z, const bool ignore_destroyed) const
{
	if (!Map.Info.IsPointOnMap(pos, z)) {
		return nullptr;
	}
	
	CMapField &mf = *this->Field(pos, z);
	
	return mf.GetTopTerrain();
}

int CMap::GetTileLandmass(const Vec2i &pos, int z) const
{
	if (!Map.Info.IsPointOnMap(pos, z)) {
		return 0;
	}
	
	CMapField &mf = *this->Field(pos, z);
	
	return mf.Landmass;
}

Vec2i CMap::GenerateUnitLocation(const CUnitType *unit_type, const CFaction *faction, const Vec2i &min_pos, const Vec2i &max_pos, const int z) const
{
	if (SaveGameLoading) {
		return Vec2i(-1, -1);
	}
	
	CPlayer *player = GetFactionPlayer(faction);
	
	Vec2i random_pos(-1, -1);
	
	std::vector<CTerrainType *> allowed_terrains;
	if (unit_type->BoolFlag[FAUNA_INDEX].value && unit_type->Species) { //if the unit is a fauna one, it has to start on terrain it is native to
		for (size_t i = 0; i < unit_type->Species->Terrains.size(); ++i) {
			allowed_terrains.push_back(unit_type->Species->Terrains[i]);
		}
	}
	
	for (size_t i = 0; i < unit_type->SpawnUnits.size(); ++i) {
		CUnitType *spawned_type = unit_type->SpawnUnits[i];
		if (spawned_type->BoolFlag[FAUNA_INDEX].value && spawned_type->Species) {
			for (size_t j = 0; j < spawned_type->Species->Terrains.size(); ++j) {
				allowed_terrains.push_back(spawned_type->Species->Terrains[j]);
			}
		}
	}

	std::vector<Vec2i> potential_positions;
	for (int x = min_pos.x; x <= max_pos.x; ++x) {
		for (int y = min_pos.y; y <= max_pos.y; ++y) {
			potential_positions.push_back(Vec2i(x, y));
		}
	}
	
	while (!potential_positions.empty()) {
		random_pos = potential_positions[SyncRand(potential_positions.size())];
		potential_positions.erase(std::remove(potential_positions.begin(), potential_positions.end(), random_pos), potential_positions.end());
		
		if (!this->Info.IsPointOnMap(random_pos, z) || (this->is_point_in_a_subtemplate_area(random_pos, z) && GameCycle == 0)) {
			continue;
		}
		
		if (allowed_terrains.size() > 0 && std::find(allowed_terrains.begin(), allowed_terrains.end(), GetTileTopTerrain(random_pos, false, z)) == allowed_terrains.end()) { //if the unit is a fauna one, it has to start on terrain it is native to
			continue;
		}
		
		std::vector<CUnit *> table;
		if (player != nullptr) {
			Select(random_pos - Vec2i(32, 32), random_pos + Vec2i(unit_type->TileSize.x - 1, unit_type->TileSize.y - 1) + Vec2i(32, 32), table, z, MakeAndPredicate(HasNotSamePlayerAs(*player), HasNotSamePlayerAs(*CPlayer::Players[PlayerNumNeutral])));
		} else if (!unit_type->GivesResource) {
			if (unit_type->BoolFlag[PREDATOR_INDEX].value || (unit_type->BoolFlag[PEOPLEAVERSION_INDEX].value && unit_type->UnitType == UnitTypeType::Fly)) {
				Select(random_pos - Vec2i(16, 16), random_pos + Vec2i(unit_type->TileSize.x - 1, unit_type->TileSize.y - 1) + Vec2i(16, 16), table, z, MakeOrPredicate(HasNotSamePlayerAs(*CPlayer::Players[PlayerNumNeutral]), HasSameTypeAs(*SettlementSiteUnitType)));
			} else {
				Select(random_pos - Vec2i(8, 8), random_pos + Vec2i(unit_type->TileSize.x - 1, unit_type->TileSize.y - 1) + Vec2i(8, 8), table, z, HasNotSamePlayerAs(*CPlayer::Players[PlayerNumNeutral]));
			}
		} else if (unit_type->GivesResource && !unit_type->BoolFlag[BUILDING_INDEX].value) { //for non-building resources (i.e. wood piles), place them within a certain distance of player units, to prevent them from blocking the way
			Select(random_pos - Vec2i(4, 4), random_pos + Vec2i(unit_type->TileSize.x - 1, unit_type->TileSize.y - 1) + Vec2i(4, 4), table, z, HasNotSamePlayerAs(*CPlayer::Players[PlayerNumNeutral]));
		}
		
		if (table.size() == 0) {
			bool passable_surroundings = true; //check if the unit won't be placed next to unpassable terrain
			for (int x = random_pos.x - 1; x < random_pos.x + unit_type->TileSize.x + 1; ++x) {
				for (int y = random_pos.y - 1; y < random_pos.y + unit_type->TileSize.y + 1; ++y) {
					if (Map.Info.IsPointOnMap(x, y, z) && Map.Field(x, y, z)->CheckMask(MapFieldUnpassable)) {
						passable_surroundings = false;
					}
				}
			}
			if (passable_surroundings && UnitTypeCanBeAt(*unit_type, random_pos, z) && (!unit_type->BoolFlag[BUILDING_INDEX].value || CanBuildUnitType(nullptr, *unit_type, random_pos, 0, true, z))) {
				return random_pos;
			}
		}
	}
	
	return Vec2i(-1, -1);
}
//Wyrmgus end

/**
**  Wall on map tile.
**
**  @param pos  map tile position.
**
**  @return    True if wall, false otherwise.
*/
bool CMap::WallOnMap(const Vec2i &pos, int z) const
{
	Assert(Map.Info.IsPointOnMap(pos, z));
	return Field(pos, z)->isAWall();
}

//Wyrmgus start
bool CMap::CurrentTerrainCanBeAt(const Vec2i &pos, bool overlay, int z)
{
	CMapField &mf = *this->Field(pos, z);
	CTerrainType *terrain = nullptr;
	
	if (overlay) {
		terrain = mf.OverlayTerrain;
	} else {
		terrain = mf.Terrain;
	}
	
	if (!terrain) {
		return true;
	}
	
	if (terrain->AllowSingle) {
		return true;
	}

	std::vector<int> transition_directions;
	
	for (int x_offset = -1; x_offset <= 1; ++x_offset) {
		for (int y_offset = -1; y_offset <= 1; ++y_offset) {
			if (x_offset != 0 || y_offset != 0) {
				Vec2i adjacent_pos(pos.x + x_offset, pos.y + y_offset);
				if (Map.Info.IsPointOnMap(adjacent_pos, z)) {
					CMapField &adjacent_mf = *this->Field(adjacent_pos, z);
						
					CTerrainType *adjacent_terrain = this->GetTileTerrain(adjacent_pos, overlay, z);
					if (overlay && adjacent_terrain && this->Field(adjacent_pos, z)->OverlayTerrainDestroyed) {
						adjacent_terrain = nullptr;
					}
					if (terrain != adjacent_terrain) { // also happens if terrain is null, so that i.e. tree transitions display correctly when adjacent to tiles without overlays
						transition_directions.push_back(GetDirectionFromOffset(x_offset, y_offset));
					}
				}
			}
		}
	}
	
	if (std::find(transition_directions.begin(), transition_directions.end(), North) != transition_directions.end() && std::find(transition_directions.begin(), transition_directions.end(), South) != transition_directions.end()) {
		return false;
	} else if (std::find(transition_directions.begin(), transition_directions.end(), West) != transition_directions.end() && std::find(transition_directions.begin(), transition_directions.end(), East) != transition_directions.end()) {
		return false;
	}

	return true;
}

bool CMap::TileBordersTerrain(const Vec2i &pos, const CTerrainType *terrain_type, const int z) const
{
	bool overlay = terrain_type != nullptr ? terrain_type->Overlay : false;

	for (int sub_x = -1; sub_x <= 1; ++sub_x) {
		for (int sub_y = -1; sub_y <= 1; ++sub_y) {
			Vec2i adjacent_pos(pos.x + sub_x, pos.y + sub_y);
			if (!this->Info.IsPointOnMap(adjacent_pos, z) || (sub_x == 0 && sub_y == 0)) {
				continue;
			}

			if (this->GetTileTopTerrain(adjacent_pos, overlay, z) == terrain_type) {
				return true;
			}
		}
	}

	return false;
}

/**
**	@brief	Get whether a given tile borders only tiles with the same terrain as itself
**
**	@param	pos					The tile's position
**	@param	new_terrain_type	The potential new terrain type for the tile
**	@param	z					The tile's map layer
**
**	@return	True if the tile borders only tiles with the same terrain as itself, false otherwise
*/
bool CMap::TileBordersOnlySameTerrain(const Vec2i &pos, const CTerrainType *new_terrain_type, const int z) const
{
	for (int sub_x = -1; sub_x <= 1; ++sub_x) {
		for (int sub_y = -1; sub_y <= 1; ++sub_y) {
			Vec2i adjacent_pos(pos.x + sub_x, pos.y + sub_y);
			if (!this->Info.IsPointOnMap(adjacent_pos, z) || (sub_x == 0 && sub_y == 0)) {
				continue;
			}
			if (this->is_point_in_a_subtemplate_area(pos, z) && !this->is_point_in_a_subtemplate_area(adjacent_pos, z)) {
				continue;
			}
			CTerrainType *top_terrain = GetTileTopTerrain(pos, false, z);
			CTerrainType *adjacent_top_terrain = GetTileTopTerrain(adjacent_pos, false, z);
			if (!new_terrain_type->Overlay) {
				if (
					adjacent_top_terrain
					&& adjacent_top_terrain != top_terrain
					&& (std::find(top_terrain->InnerBorderTerrains.begin(), top_terrain->InnerBorderTerrains.end(), adjacent_top_terrain) == top_terrain->InnerBorderTerrains.end() || std::find(new_terrain_type->InnerBorderTerrains.begin(), new_terrain_type->InnerBorderTerrains.end(), adjacent_top_terrain) == new_terrain_type->InnerBorderTerrains.end())
					&& adjacent_top_terrain != new_terrain_type
				) {
					return false;
				}
			} else {
				if (
					adjacent_top_terrain
					&& adjacent_top_terrain != top_terrain
					&& std::find(top_terrain->BaseTerrainTypes.begin(), top_terrain->BaseTerrainTypes.end(), adjacent_top_terrain) == top_terrain->BaseTerrainTypes.end() && std::find(adjacent_top_terrain->BaseTerrainTypes.begin(), adjacent_top_terrain->BaseTerrainTypes.end(), top_terrain) == adjacent_top_terrain->BaseTerrainTypes.end()
					&& adjacent_top_terrain != new_terrain_type
				) {
					return false;
				}
			}
		}
	}
		
	return true;
}

bool CMap::TileBordersFlag(const Vec2i &pos, int z, int flag, bool reverse)
{
	for (int sub_x = -1; sub_x <= 1; ++sub_x) {
		for (int sub_y = -1; sub_y <= 1; ++sub_y) {
			Vec2i adjacent_pos(pos.x + sub_x, pos.y + sub_y);
			if (!this->Info.IsPointOnMap(adjacent_pos, z) || (sub_x == 0 && sub_y == 0)) {
				continue;
			}
			CMapField &mf = *Map.Field(adjacent_pos, z);
			
			if ((!reverse && mf.CheckMask(flag)) || (reverse && !mf.CheckMask(flag))) {
				return true;
			}
		}
	}
		
	return false;
}

bool CMap::TileBordersBuilding(const Vec2i &pos, int z)
{
	for (int sub_x = -1; sub_x <= 1; ++sub_x) {
		for (int sub_y = -1; sub_y <= 1; ++sub_y) {
			Vec2i adjacent_pos(pos.x + sub_x, pos.y + sub_y);
			if (!this->Info.IsPointOnMap(adjacent_pos, z) || (sub_x == 0 && sub_y == 0)) {
				continue;
			}
			CMapField &mf = *Map.Field(adjacent_pos, z);
			
			if (mf.CheckMask(MapFieldBuilding)) {
				return true;
			}
		}
	}
		
	return false;
}

bool CMap::TileBordersPathway(const Vec2i &pos, int z, bool only_railroad)
{
	for (int sub_x = -1; sub_x <= 1; ++sub_x) {
		for (int sub_y = -1; sub_y <= 1; ++sub_y) {
			Vec2i adjacent_pos(pos.x + sub_x, pos.y + sub_y);
			if (!this->Info.IsPointOnMap(adjacent_pos, z) || (sub_x == 0 && sub_y == 0)) {
				continue;
			}
			CMapField &mf = *Map.Field(adjacent_pos, z);
			
			if (
				(!only_railroad && mf.CheckMask(MapFieldRoad))
				|| mf.CheckMask(MapFieldRailroad)
			) {
				return true;
			}
		}
	}
		
	return false;
}

bool CMap::TileBordersUnit(const Vec2i &pos, int z)
{
	for (int sub_x = -1; sub_x <= 1; ++sub_x) {
		for (int sub_y = -1; sub_y <= 1; ++sub_y) {
			Vec2i adjacent_pos(pos.x + sub_x, pos.y + sub_y);
			if (!this->Info.IsPointOnMap(adjacent_pos, z) || (sub_x == 0 && sub_y == 0)) {
				continue;
			}
			CMapField &mf = *Map.Field(adjacent_pos, z);
			
			const CUnitCache &cache = mf.UnitCache;
			for (size_t i = 0; i != cache.size(); ++i) {
				CUnit &unit = *cache[i];
				if (unit.IsAliveOnMap()) {
					return true;
				}
			}
		}
	}
		
	return false;
}

/**
**	@brief	Get whether the given tile has any bordering terrains which are incompatible with a given terrain type
**
**	@param	pos					The tile's position
**	@param	new_terrain_type	The terrain type to check
**	@param	z					The tile's map layer
**
**	@return	True if the tile borders only tiles with the same terrain as itself, false otherwise
*/
bool CMap::TileBordersTerrainIncompatibleWithTerrain(const Vec2i &pos, const CTerrainType *terrain_type, const int z) const
{
	if (!terrain_type || !terrain_type->Overlay) {
		return false;
	}
	
	CTerrainType *tile_terrain = this->GetTileTerrain(pos, false, z);
	
	for (int sub_x = -1; sub_x <= 1; ++sub_x) {
		for (int sub_y = -1; sub_y <= 1; ++sub_y) {
			Vec2i adjacent_pos(pos.x + sub_x, pos.y + sub_y);
			
			if (!this->Info.IsPointOnMap(adjacent_pos, z) || (sub_x == 0 && sub_y == 0)) {
				continue;
			}
			
			CTerrainType *adjacent_terrain = this->GetTileTerrain(adjacent_pos, false, z);
			
			if (adjacent_terrain == nullptr) {
				continue;
			}

			if (tile_terrain == adjacent_terrain) {
				continue;
			}
			
			if (terrain_type->Overlay) {
				if ( //if the terrain type is an overlay one, the adjacent tile terrain is incompatible with it if it both cannot be a base terrain for the overlay terrain type, and it "expands into" the tile (that is, the tile has the adjacent terrain as an inner border terrain)
					std::find(tile_terrain->InnerBorderTerrains.begin(), tile_terrain->InnerBorderTerrains.end(), adjacent_terrain) != tile_terrain->InnerBorderTerrains.end()
					&& std::find(terrain_type->BaseTerrainTypes.begin(), terrain_type->BaseTerrainTypes.end(), adjacent_terrain) == terrain_type->BaseTerrainTypes.end()
				) {
					return true;
				}
			} else {
				//if the terrain type is not an overlay one, the adjacent tile terrain is incompatible with it if it cannot border the terrain type
				if (std::find(terrain_type->BorderTerrains.begin(), terrain_type->BorderTerrains.end(), adjacent_terrain) == terrain_type->BorderTerrains.end()) {
					return true;
				}
			}
		}
	}
		
	return false;
}

bool CMap::TileBordersTerrainIncompatibleWithTerrainPair(const Vec2i &pos, const CTerrainType *terrain_type, const CTerrainType *overlay_terrain_type, const int z) const
{
	if (!terrain_type) {
		return false;
	}

	for (int sub_x = -1; sub_x <= 1; ++sub_x) {
		for (int sub_y = -1; sub_y <= 1; ++sub_y) {
			Vec2i adjacent_pos(pos.x + sub_x, pos.y + sub_y);

			if (!this->Info.IsPointOnMap(adjacent_pos, z) || (sub_x == 0 && sub_y == 0)) {
				continue;
			}

			CTerrainType *adjacent_terrain = this->GetTileTerrain(adjacent_pos, false, z);

			if (adjacent_terrain == nullptr) {
				continue;
			}

			if (terrain_type == adjacent_terrain) {
				continue;
			}

			//the adjacent tile terrain is incompatible with the non-overlay terrain type if it cannot border the terrain type
			if (std::find(terrain_type->BorderTerrains.begin(), terrain_type->BorderTerrains.end(), adjacent_terrain) == terrain_type->BorderTerrains.end()) {
				return true;
			}

			if (overlay_terrain_type != nullptr) {
				if ( //if the terrain type is an overlay one, the adjacent tile terrain is incompatible with it if it both cannot be a base terrain for the overlay terrain type, and it "expands into" the tile (that is, the tile has the adjacent terrain as an inner border terrain)
					std::find(terrain_type->InnerBorderTerrains.begin(), terrain_type->InnerBorderTerrains.end(), adjacent_terrain) != terrain_type->InnerBorderTerrains.end()
					&& std::find(overlay_terrain_type->BaseTerrainTypes.begin(), overlay_terrain_type->BaseTerrainTypes.end(), adjacent_terrain) == overlay_terrain_type->BaseTerrainTypes.end()
					) {
					return true;
				}
			}
		}
	}

	return false;
}

/**
**	@brief	Get whether a tile has units that are incompatible with a given terrain type
**
**	@param	pos				The tile's position
**	@param	terrain_type	The terrain type
**	@param	z				The tile's map layer
**
**	@return	Whether the tile has units that are incompatible with the given terrain type
*/
bool CMap::TileHasUnitsIncompatibleWithTerrain(const Vec2i &pos, const CTerrainType *terrain_type, const int z)
{
	CMapField &mf = *Map.Field(pos, z);
	
	const CUnitCache &cache = mf.UnitCache;
	for (size_t i = 0; i != cache.size(); ++i) {
		const CUnit &unit = *cache[i];
		if (unit.IsAliveOnMap() && (terrain_type->Flags & unit.Type->MovementMask) != 0) {
			return true;
		}
	}

	return false;
}

/**
**	@brief	Get whether a given tile is in a subtemplate area
**
**	@param	pos				The tile's position
**	@param	z				The tile's map layer
**	@param	subtemplate		Optional subtemplate argument, if not null then will only return true if the point is in that specific subtemplate area; if it is null, then true will be returned if the point is in any subtemplate area
**
**	@return	True if the tile is in a subtemplate area, or false otherwise
*/
bool CMap::is_point_in_a_subtemplate_area(const Vec2i &pos, const int z, const stratagus::map_template *subtemplate) const
{
	for (size_t i = 0; i < this->MapLayers[z]->subtemplate_areas.size(); ++i) {
		if (subtemplate && subtemplate != std::get<2>(this->MapLayers[z]->subtemplate_areas[i])) {
			continue;
		}
		
		Vec2i min_pos = std::get<0>(this->MapLayers[z]->subtemplate_areas[i]);
		Vec2i max_pos = std::get<1>(this->MapLayers[z]->subtemplate_areas[i]);
		if (pos.x >= min_pos.x && pos.y >= min_pos.y && pos.x <= max_pos.x && pos.y <= max_pos.y) {
			return true;
		}
	}

	return false;
}

std::pair<Vec2i, Vec2i> CMap::get_subtemplate_rect(const stratagus::map_template *subtemplate) const
{
	if (!subtemplate) {
		return std::make_pair(Vec2i(-1, -1), Vec2i(-1, -1));
	}

	const stratagus::map_template *main_template = subtemplate->GetTopMapTemplate();
	if (main_template && subtemplate != main_template && main_template->Plane && main_template->World) {
		const int z = GetMapLayer(main_template->Plane->Ident, main_template->World->Ident, main_template->SurfaceLayer);
		if (z != -1) {
			for (size_t i = 0; i < this->MapLayers[z]->subtemplate_areas.size(); ++i) {
				if (subtemplate == std::get<2>(this->MapLayers[z]->subtemplate_areas[i])) {
					return std::make_pair(std::get<0>(CMap::Map.MapLayers[z]->subtemplate_areas[i]), std::get<1>(CMap::Map.MapLayers[z]->subtemplate_areas[i]));
				}
			}
		}
	}
}

/**
**	@brief	Get the applied map position of a given subtemplate
**
**	@param	subtemplate		The subtemplate
**
**	@return	The subtemplate's position if found, or (-1, -1) otherwise
*/
Vec2i CMap::get_subtemplate_pos(const stratagus::map_template *subtemplate) const
{
	std::pair<Vec2i, Vec2i> subtemplate_rect = this->get_subtemplate_rect(subtemplate);
	return subtemplate_rect.first;
}

Vec2i CMap::get_subtemplate_center_pos(const stratagus::map_template *subtemplate) const
{
	std::pair<Vec2i, Vec2i> subtemplate_rect = this->get_subtemplate_rect(subtemplate);

	const Vec2i &start_pos = subtemplate_rect.first;
	const Vec2i &end_pos = subtemplate_rect.second;

	return start_pos + ((end_pos - start_pos) / 2);
}

/**
**	@brief	Get the applied end map position of a given subtemplate
**
**	@param	subtemplate		The subtemplate
**
**	@return	The subtemplate's end position if found, or (-1, -1) otherwise
*/
Vec2i CMap::get_subtemplate_end_pos(const stratagus::map_template *subtemplate) const
{
	std::pair<Vec2i, Vec2i> subtemplate_rect = this->get_subtemplate_rect(subtemplate);
	return subtemplate_rect.second;
}

/**
**	@brief	Get the applied map layer of a given subtemplate
**
**	@param	subtemplate		The subtemplate
**
**	@return	The subtemplate's map layer if found, or null otherwise
*/
CMapLayer *CMap::get_subtemplate_map_layer(const stratagus::map_template *subtemplate) const
{
	if (!subtemplate) {
		return nullptr;
	}
	
	const stratagus::map_template *main_template = subtemplate->GetTopMapTemplate();
	if (main_template && subtemplate != main_template && main_template->Plane && main_template->World) {
		const int z = GetMapLayer(main_template->Plane->Ident, main_template->World->Ident, main_template->SurfaceLayer);
		if (z != -1) {
			for (size_t i = 0; i < this->MapLayers[z]->subtemplate_areas.size(); ++i) {
				if (subtemplate == std::get<2>(this->MapLayers[z]->subtemplate_areas[i])) {
					return this->MapLayers[z];
				}
			}
		}
	}
	
	return nullptr;
}

/**
**	@brief	Get the map layer connectors in a given map template
**
**	@param	subtemplate		The subtemplate
**
**	@return	A list of the connector units
*/
std::vector<CUnit *> CMap::get_map_template_layer_connectors(const stratagus::map_template *map_template) const
{
	std::vector<CUnit *> layer_connectors;
	
	if (!map_template) {
		return layer_connectors;
	}
	
	const stratagus::map_template *main_template = map_template->GetTopMapTemplate();
	if (main_template && main_template->Plane && main_template->World) {
		const bool is_main_template = main_template == map_template;
		const int z = GetMapLayer(main_template->Plane->Ident, main_template->World->Ident, main_template->SurfaceLayer);
		if (z != -1) {
			for (size_t i = 0; i < this->MapLayers[z]->LayerConnectors.size(); ++i) {
				CUnit *connector_unit = this->MapLayers[z]->LayerConnectors[i];
				const Vec2i unit_pos = connector_unit->GetTileCenterPos();
				
				if (is_main_template && this->is_point_in_a_subtemplate_area(unit_pos, z)) {
					continue;
				} else if (!is_main_template && !this->is_point_in_a_subtemplate_area(unit_pos, z, map_template)) {
					continue;
				}

				layer_connectors.push_back(connector_unit);
			}
		}
	}
	
	return layer_connectors;
}

/**
**	@brief	Get whether a given tile is adjacent to non-subtemplate area tiles
**
**	@param	pos		The tile's position
**	@param	z		The tile's map layer
**
**	@return	True if the tile is adjacent to a non-subtemplate area tile, or false otherwise
*/
bool CMap::is_point_adjacent_to_non_subtemplate_area(const Vec2i &pos, const int z) const
{
	for (int x_offset = -1; x_offset <= 1; ++x_offset) {
		for (int y_offset = -1; y_offset <= 1; ++y_offset) {
			if (x_offset == 0 && y_offset == 0) {
				continue;
			}
			
			Vec2i adjacent_pos(pos.x + x_offset, pos.y + y_offset);
			
			if (Map.Info.IsPointOnMap(adjacent_pos, z) && !this->is_point_in_a_subtemplate_area(adjacent_pos, z)) {
				return true;
			}
		}
	}
	
	return false;
}

bool CMap::IsLayerUnderground(int z) const
{
	if (GameSettings.Inside) {
		return true;
	}
	
	if (this->MapLayers[z]->SurfaceLayer > 0) {
		return true;
	}

	return false;
}

void CMap::SetCurrentPlane(CPlane *plane)
{
	if (UI.CurrentMapLayer->Plane == plane) {
		return;
	}
	
	int map_layer = -1;
	
	for (size_t z = 0; z < Map.MapLayers.size(); ++z) {
		if (Map.MapLayers[z]->Plane == plane && Map.MapLayers[z]->SurfaceLayer == this->GetCurrentSurfaceLayer()) {
			map_layer = z;
			break;
		}
	}
	
	if (map_layer == -1) {
		for (size_t z = 0; z < Map.MapLayers.size(); ++z) {
			if (Map.MapLayers[z]->Plane == plane) {
				map_layer = z;
				break;
			}
		}
	}
	
	if (map_layer != -1) {
		ChangeCurrentMapLayer(map_layer);
	}
}

void CMap::SetCurrentWorld(CWorld *world)
{
	if (UI.CurrentMapLayer->World == world) {
		return;
	}
	
	int map_layer = -1;
	
	for (size_t z = 0; z < Map.MapLayers.size(); ++z) {
		if (Map.MapLayers[z]->World == world && Map.MapLayers[z]->SurfaceLayer == this->GetCurrentSurfaceLayer()) {
			map_layer = z;
			break;
		}
	}
	
	if (map_layer == -1) {
		for (size_t z = 0; z < Map.MapLayers.size(); ++z) {
			if (Map.MapLayers[z]->World == world) {
				map_layer = z;
				break;
			}
		}
	}
	
	if (map_layer != -1) {
		ChangeCurrentMapLayer(map_layer);
	}
}

void CMap::SetCurrentSurfaceLayer(int surface_layer)
{
	if (UI.CurrentMapLayer->SurfaceLayer == surface_layer) {
		return;
	}
	
	int map_layer = -1;
	
	for (size_t z = 0; z < Map.MapLayers.size(); ++z) {
		if (Map.MapLayers[z]->Plane == this->GetCurrentPlane() && Map.MapLayers[z]->World == this->GetCurrentWorld() && Map.MapLayers[z]->SurfaceLayer == surface_layer) {
			map_layer = z;
			break;
		}
	}
	
	if (map_layer != -1) {
		ChangeCurrentMapLayer(map_layer);
	}
}

CPlane *CMap::GetCurrentPlane() const
{
	if (UI.CurrentMapLayer) {
		return UI.CurrentMapLayer->Plane;
	} else {
		return nullptr;
	}
}

CWorld *CMap::GetCurrentWorld() const
{
	if (UI.CurrentMapLayer) {
		return UI.CurrentMapLayer->World;
	} else {
		return nullptr;
	}
}

int CMap::GetCurrentSurfaceLayer() const
{
	if (UI.CurrentMapLayer) {
		return UI.CurrentMapLayer->SurfaceLayer;
	} else {
		return 0;
	}
}

PixelSize CMap::GetCurrentPixelTileSize() const
{
	if (UI.CurrentMapLayer) {
		return UI.CurrentMapLayer->PixelTileSize;
	} else {
		return PixelSize(32, 32);
	}
}

PixelSize CMap::GetMapLayerPixelTileSize(int map_layer) const
{
	if (map_layer >= 0 && map_layer < (int) Map.MapLayers.size()) {
		return Map.MapLayers[map_layer]->PixelTileSize;
	} else {
		return PixelSize(32, 32);
	}
}
//Wyrmgus end

/**
**  Can move to this point, applying mask.
**
**  @param pos   map tile position.
**  @param mask  Mask for movement to apply.
**
**  @return      True if could be entered, false otherwise.
*/
bool CheckedCanMoveToMask(const Vec2i &pos, int mask, int z)
{
	return CMap::Map.Info.IsPointOnMap(pos, z) && CanMoveToMask(pos, mask, z);
}

/**
**  Can a unit of unit-type be placed at this point.
**
**  @param type  unit-type to be checked.
**  @param pos   map tile position.
**
**  @return      True if could be entered, false otherwise.
*/
//Wyrmgus start
//bool UnitTypeCanBeAt(const CUnitType &type, const Vec2i &pos)
bool UnitTypeCanBeAt(const CUnitType &type, const Vec2i &pos, int z)
//Wyrmgus end
{
	const int mask = type.MovementMask;
	//Wyrmgus start
//	unsigned int index = pos.y * CMap::Map.Info.MapWidth;
	unsigned int index = pos.y * CMap::Map.Info.MapWidths[z];
	//Wyrmgus end

	for (int addy = 0; addy < type.TileSize.y; ++addy) {
		for (int addx = 0; addx < type.TileSize.x; ++addx) {
			if (CMap::Map.Info.IsPointOnMap(pos.x + addx, pos.y + addy, z) == false
				|| CMap::Map.Field(pos.x + addx + index, z)->CheckMask(mask) == true) {
				return false;
			}
		}
		//Wyrmgus start
//		index += CMap::Map.Info.MapWidth;
		index += CMap::Map.Info.MapWidths[z];
		//Wyrmgus end
	}
	return true;
}

/**
**  Can a unit be placed to this point.
**
**  @param unit  unit to be checked.
**  @param pos   map tile position.
**
**  @return      True if could be placeded, false otherwise.
*/
//Wyrmgus start
//bool UnitCanBeAt(const CUnit &unit, const Vec2i &pos)
bool UnitCanBeAt(const CUnit &unit, const Vec2i &pos, int z)
//Wyrmgus end
{
	Assert(unit.Type);
	if (unit.Type->BoolFlag[NONSOLID_INDEX].value) {
		return true;
	}
	//Wyrmgus start
//	return UnitTypeCanBeAt(*unit.Type, pos);
	return UnitTypeCanBeAt(*unit.Type, pos, z);
	//Wyrmgus end
}

/**
**  Fixes initially the wood and seen tiles.
*/
void PreprocessMap()
{
	ShowLoadProgress("%s", _("Initializing Map"));
		
	//Wyrmgus start
	/*
	for (int ix = 0; ix < Map.Info.MapWidth; ++ix) {
		for (int iy = 0; iy < Map.Info.MapHeight; ++iy) {
			CMapField &mf = *Map.Field(ix, iy);
			mf.playerInfo.SeenTile = mf.getGraphicTile();
		}
	}
	*/
	for (size_t z = 0; z < CMap::Map.MapLayers.size(); ++z) {
		for (int ix = 0; ix < CMap::Map.Info.MapWidths[z]; ++ix) {
			for (int iy = 0; iy < CMap::Map.Info.MapHeights[z]; ++iy) {
				CMapField &mf = *CMap::Map.Field(ix, iy, z);
				CMap::Map.CalculateTileTransitions(Vec2i(ix, iy), false, z);
				CMap::Map.CalculateTileTransitions(Vec2i(ix, iy), true, z);
				CMap::Map.CalculateTileLandmass(Vec2i(ix, iy), z);
				CMap::Map.CalculateTileOwnership(Vec2i(ix, iy), z);
				CMap::Map.CalculateTileTerrainFeature(Vec2i(ix, iy), z);
				mf.UpdateSeenTile();
				UI.Minimap.UpdateXY(Vec2i(ix, iy), z);
				if (mf.playerInfo.IsTeamVisible(*CPlayer::GetThisPlayer())) {
					CMap::Map.MarkSeenTile(mf, z);
				}
			}
		}
	}
	//Wyrmgus end
	//Wyrmgus start
	/*
	// it is required for fixing the wood that all tiles are marked as seen!
	if (Map.Tileset->TileTypeTable.empty() == false) {
		Vec2i pos;
		for (pos.x = 0; pos.x < Map.Info.MapWidth; ++pos.x) {
			for (pos.y = 0; pos.y < Map.Info.MapHeight; ++pos.y) {
				MapFixWallTile(pos);
				MapFixSeenWallTile(pos);
			}
		}
	}
	*/
	//Wyrmgus end
}

//Wyrmgus start
int GetMapLayer(const std::string &plane_ident, const std::string &world_ident, const int surface_layer)
{
	CPlane *plane = CPlane::GetPlane(plane_ident, false);
	CWorld *world = CWorld::GetWorld(world_ident, false);

	for (size_t z = 0; z < CMap::Map.MapLayers.size(); ++z) {
		if (CMap::Map.MapLayers[z]->Plane == plane && CMap::Map.MapLayers[z]->World == world && CMap::Map.MapLayers[z]->SurfaceLayer == surface_layer) {
			return z;
		}
	}
	
	return -1;
}

/**
**	@brief	Change the map layer currently being displayed to the previous one
*/
void ChangeToPreviousMapLayer()
{
	if (!UI.PreviousMapLayer) {
		return;
	}
	
	ChangeCurrentMapLayer(UI.PreviousMapLayer->ID);
}

/**
**	@brief	Change the map layer currently being displayed
**
**	@param	z	The map layer
*/
void ChangeCurrentMapLayer(const int z)
{
	if (z < 0 || z >= static_cast<int>(CMap::Map.MapLayers.size()) || UI.CurrentMapLayer->ID == z) {
		return;
	}
	
	Vec2i new_viewport_map_pos(UI.SelectedViewport->MapPos.x * CMap::Map.Info.MapWidths[z] / UI.CurrentMapLayer->GetWidth(), UI.SelectedViewport->MapPos.y * CMap::Map.Info.MapHeights[z] / UI.CurrentMapLayer->GetHeight());
	
	UI.PreviousMapLayer = UI.CurrentMapLayer;
	UI.CurrentMapLayer = CMap::Map.MapLayers[z];
	UI.Minimap.UpdateCache = true;
	UI.SelectedViewport->Set(new_viewport_map_pos, CMap::Map.GetCurrentPixelTileSize() / 2);
	UpdateSurfaceLayerButtons();
}

/**
**	@brief	Set the current time of day for a particular map layer
**
**	@param	time_of_day_ident	The time of day's string identifier
**	@param	z					The map layer
*/
void SetTimeOfDay(const std::string &time_of_day_ident, int z)
{
	if (time_of_day_ident.empty()) {
		CMap::Map.MapLayers[z]->SetTimeOfDay(nullptr);
		CMap::Map.MapLayers[z]->RemainingTimeOfDayHours = 0;
	} else {
		CTimeOfDaySchedule *schedule = CMap::Map.MapLayers[z]->TimeOfDaySchedule;
		if (schedule) {
			for (size_t i = 0; i < schedule->ScheduledTimesOfDay.size(); ++i) {
				CScheduledTimeOfDay *time_of_day = schedule->ScheduledTimesOfDay[i];
				if (time_of_day->TimeOfDay->Ident == time_of_day_ident)  {
					CMap::Map.MapLayers[z]->SetTimeOfDay(time_of_day);
					CMap::Map.MapLayers[z]->RemainingTimeOfDayHours = time_of_day->GetHours(CMap::Map.MapLayers[z]->GetSeason());
					break;
				}
			}
		}
	}
}

/**
**	@brief	Set the time of day schedule for a particular map layer
**
**	@param	time_of_day_schedule_ident	The time of day schedule's string identifier
**	@param	z							The map layer
*/
void SetTimeOfDaySchedule(const std::string &time_of_day_schedule_ident, const int z)
{
	if (z >= static_cast<int>(CMap::Map.MapLayers.size())) {
		fprintf(stderr, "Error in CMap::SetTimeOfDaySchedule: the given map layer index (%d) is not valid given the map layer quantity (%d).\n", z, CMap::Map.MapLayers.size());
		return;
	}

	if (time_of_day_schedule_ident.empty()) {
		CMap::Map.MapLayers[z]->TimeOfDaySchedule = nullptr;
		CMap::Map.MapLayers[z]->SetTimeOfDay(nullptr);
		CMap::Map.MapLayers[z]->RemainingTimeOfDayHours = 0;
	} else {
		CTimeOfDaySchedule *schedule = CTimeOfDaySchedule::GetTimeOfDaySchedule(time_of_day_schedule_ident);
		if (schedule) {
			CMap::Map.MapLayers[z]->TimeOfDaySchedule = schedule;
			CMap::Map.MapLayers[z]->SetTimeOfDay(schedule->ScheduledTimesOfDay.front());
			CMap::Map.MapLayers[z]->RemainingTimeOfDayHours = CMap::Map.MapLayers[z]->TimeOfDay->GetHours(CMap::Map.MapLayers[z]->GetSeason());
		}
	}
}

/**
**	@brief	Set the current season for a particular map layer
**
**	@param	season_ident		The season's string identifier
**	@param	z					The map layer
*/
void SetSeason(const std::string &season_ident, int z)
{
	if (season_ident.empty()) {
		CMap::Map.MapLayers[z]->SetSeason(nullptr);
		CMap::Map.MapLayers[z]->RemainingSeasonHours = 0;
	} else {
		CSeasonSchedule *schedule = CMap::Map.MapLayers[z]->SeasonSchedule;
		if (schedule) {
			for (size_t i = 0; i < schedule->ScheduledSeasons.size(); ++i) {
				CScheduledSeason *season = schedule->ScheduledSeasons[i];
				if (season->Season->Ident == season_ident)  {
					CMap::Map.MapLayers[z]->SetSeason(season);
					CMap::Map.MapLayers[z]->RemainingSeasonHours = season->Hours;
					break;
				}
			}
		}
	}
}

/**
**	@brief	Set the season schedule for a particular map layer
**
**	@param	season_schedule_ident		The season schedule's string identifier
**	@param	z							The map layer
*/
void SetSeasonSchedule(const std::string &season_schedule_ident, int z)
{
	if (season_schedule_ident.empty()) {
		CMap::Map.MapLayers[z]->SeasonSchedule = nullptr;
		CMap::Map.MapLayers[z]->SetSeason(nullptr);
		CMap::Map.MapLayers[z]->RemainingSeasonHours = 0;
	} else {
		CSeasonSchedule *schedule = CSeasonSchedule::GetSeasonSchedule(season_schedule_ident);
		if (schedule) {
			CMap::Map.MapLayers[z]->SeasonSchedule = schedule;
			CMap::Map.MapLayers[z]->SetSeason(schedule->ScheduledSeasons.front());
			CMap::Map.MapLayers[z]->RemainingSeasonHours = CMap::Map.MapLayers[z]->Season->Hours;
		}
	}
}
//Wyrmgus end

/**
**	@brief	Get whether a given coordinate is a valid point on the map
**
**	@param	x	The x coordinate
**	@param	y	The y coordinate
**	@param	z	The map layer
**
**	@return	True if the coordinate is valid, false otherwise
*/
bool CMapInfo::IsPointOnMap(const int x, const int y, const int z) const
{
	return (z >= 0 && z < (int) MapWidths.size() && z < (int) MapHeights.size() && x >= 0 && y >= 0 && x < MapWidths[z] && y < MapHeights[z]);
}

/**
**	@brief	Get whether a given coordinate is a valid point on the map
**
**	@param	pos	The coordinate position
**	@param	z	The map layer
**
**	@return	True if the coordinate is valid, false otherwise
*/
bool CMapInfo::IsPointOnMap(const Vec2i &pos, const int z) const
{
	return IsPointOnMap(pos.x, pos.y, z);
}

/**
**	@brief	Get whether a given coordinate is a valid point on the map
**
**	@param	x			The x coordinate
**	@param	y			The y coordinate
**	@param	map_layer	The map layer
**
**	@return	True if the coordinate is valid, false otherwise
*/
bool CMapInfo::IsPointOnMap(const int x, const int y, const CMapLayer *map_layer) const
{
	return (map_layer && x >= 0 && y >= 0 && x < map_layer->GetWidth() && y < map_layer->GetHeight());
}

/**
**	@brief	Get whether a given coordinate is a valid point on the map
**
**	@param	pos			The coordinate position
**	@param	map_layer	The map layer
**
**	@return	True if the coordinate is valid, false otherwise
*/
bool CMapInfo::IsPointOnMap(const Vec2i &pos, const CMapLayer *map_layer) const
{
	return IsPointOnMap(pos.x, pos.y, map_layer);
}

/**
**	@brief	Clear CMapInfo
*/
void CMapInfo::Clear()
{
	this->Description.clear();
	this->Filename.clear();
	this->MapWidth = this->MapHeight = 0;
	//Wyrmgus start
	this->MapWidths.clear();
	this->MapHeights.clear();
	//Wyrmgus end
	memset(this->PlayerSide, 0, sizeof(this->PlayerSide));
	memset(this->PlayerType, 0, sizeof(this->PlayerType));
	this->MapUID = 0;
}

CMap::CMap() : NoFogOfWar(false), TileGraphic(nullptr), Landmasses(0), BorderTerrain(nullptr)
{
	Tileset = new CTileset;
}

CMap::~CMap()
{
	delete Tileset;
}

unsigned int CMap::getIndex(int x, int y, int z) const
{
	return x + y * this->Info.MapWidths[z];
}

unsigned int CMap::getIndex(const Vec2i &pos, int z) const
{
	return getIndex(pos.x, pos.y, z);
}

/**
**	@brief	Get the map field at a given location
**
**	@param	index	The index of the map field
**	@param	z		The map layer of the map field
**
**	@return	The map field
*/
CMapField *CMap::Field(const unsigned int index, const int z) const
{
	return this->MapLayers[z]->Field(index);
}

/**
**	@brief	Get the map field at a given location
**
**	@param	x	The x coordinate of the map field
**	@param	y	The y coordinate of the map field
**	@param	z	The map layer of the map field
**
**	@return	The map field
*/
CMapField *CMap::Field(const int x, const int y, const int z) const
{
	return this->MapLayers[z]->Field(x, y);
}

/**
**	@brief	Allocate and initialize map table
*/
void CMap::Create()
{
	Assert(this->MapLayers.size() == 0);

	CMapLayer *map_layer = new CMapLayer(this->Info.MapWidth, this->Info.MapHeight);
	map_layer->ID = this->MapLayers.size();
	this->MapLayers.push_back(map_layer);
	this->Info.MapWidths.push_back(this->Info.MapWidth);
	this->Info.MapHeights.push_back(this->Info.MapHeight);
	
	if (Editor.Running == EditorNotRunning) {
		map_layer->SeasonSchedule = CSeasonSchedule::DefaultSeasonSchedule;
		map_layer->SetSeasonByHours(CDate::CurrentTotalHours);
		
		if (!GameSettings.Inside && !GameSettings.NoTimeOfDay) {
			map_layer->TimeOfDaySchedule = CTimeOfDaySchedule::DefaultTimeOfDaySchedule;
			map_layer->SetTimeOfDayByHours(CDate::CurrentTotalHours);
		} else {
			map_layer->TimeOfDaySchedule = nullptr;
			map_layer->SetTimeOfDay(nullptr); // make indoors have no time of day setting until it is possible to make light sources change their surrounding "time of day" // indoors it is always dark (maybe would be better to allow a special setting to have bright indoor places?
		}
	}
}

/**
**  Initialize the fog of war.
**  Build tables, setup functions.
*/
void CMap::Init()
{
	for (std::map<PixelSize, CGraphic *>::iterator iterator = this->FogGraphics.begin(); iterator != this->FogGraphics.end(); ++iterator) {
		InitFogOfWar(iterator->first);
	}
}

/**
**  Cleanup the map module.
*/
void CMap::Clean()
{
	UI.CurrentMapLayer = nullptr;
	UI.PreviousMapLayer = nullptr;
	this->Landmasses = 0;

	//Wyrmgus start
	this->ClearMapLayers();
	this->BorderLandmasses.clear();
	this->SiteUnits.clear();
	//Wyrmgus end

	// Tileset freed by Tileset?

	this->Info.Clear();
	this->NoFogOfWar = false;
	this->Tileset->clear();
	this->TileModelsFileName.clear();
	CGraphic::Free(this->TileGraphic);
	this->TileGraphic = nullptr;

	FlagRevealMap = 0;
	ReplayRevealMap = 0;

	UI.Minimap.Destroy();
}

void CMap::ClearMapLayers()
{
	for (size_t z = 0; z < this->MapLayers.size(); ++z) {
		delete this->MapLayers[z];
	}
	this->MapLayers.clear();
}

/**
** Save the complete map.
**
** @param file Output file.
*/
void CMap::Save(CFile &file) const
{
	file.printf("\n--- -----------------------------------------\n");
	file.printf("--- MODULE: map\n");
	file.printf("LoadTileModels(\"%s\")\n\n", this->TileModelsFileName.c_str());
	file.printf("StratagusMap(\n");
	file.printf("  \"version\", \"%s\",\n", VERSION);
	file.printf("  \"description\", \"%s\",\n", this->Info.Description.c_str());
	file.printf("  \"the-map\", {\n");
	file.printf("  \"size\", {%d, %d},\n", this->Info.MapWidth, this->Info.MapHeight);
	file.printf("  \"%s\",\n", this->NoFogOfWar ? "no-fog-of-war" : "fog-of-war");
	file.printf("  \"filename\", \"%s\",\n", this->Info.Filename.c_str());
	//Wyrmgus start
	file.printf("  \"extra-map-layers\", {\n");
	for (size_t z = 1; z < this->MapLayers.size(); ++z) {
		file.printf("  {%d, %d},\n", this->Info.MapWidths[z], this->Info.MapHeights[z]);
	}
	file.printf("  },\n");
	file.printf("  \"time-of-day\", {\n");
	for (size_t z = 0; z < this->MapLayers.size(); ++z) {
		file.printf("  {\"%s\", %d, %d},\n", this->MapLayers[z]->TimeOfDaySchedule ? this->MapLayers[z]->TimeOfDaySchedule->Ident.c_str() : "", this->MapLayers[z]->TimeOfDay ? this->MapLayers[z]->TimeOfDay->ID : 0, this->MapLayers[z]->RemainingTimeOfDayHours);
	}
	file.printf("  },\n");
	file.printf("  \"season\", {\n");
	for (size_t z = 0; z < this->MapLayers.size(); ++z) {
		file.printf("  {\"%s\", %d, %d},\n", this->MapLayers[z]->SeasonSchedule ? this->MapLayers[z]->SeasonSchedule->Ident.c_str() : "", this->MapLayers[z]->Season ? this->MapLayers[z]->Season->ID : 0, this->MapLayers[z]->RemainingSeasonHours);
	}
	file.printf("  },\n");
	file.printf("  \"pixel-tile-size\", {\n");
	for (size_t z = 0; z < this->MapLayers.size(); ++z) {
		file.printf("  {%d, %d},\n", this->MapLayers[z]->PixelTileSize.x, this->MapLayers[z]->PixelTileSize.y);
	}
	file.printf("  },\n");
	file.printf("  \"layer-references\", {\n");
	for (size_t z = 0; z < this->MapLayers.size(); ++z) {
		file.printf("  {\"%s\", \"%s\", %d},\n", this->MapLayers[z]->Plane ? this->MapLayers[z]->Plane->Ident.c_str() : "", this->MapLayers[z]->World ? this->MapLayers[z]->World->Ident.c_str() : "", this->MapLayers[z]->SurfaceLayer);
	}
	file.printf("  },\n");
	file.printf("  \"landmasses\", {\n");
	for (int i = 1; i <= this->Landmasses; ++i) {
		file.printf("  {");
		for (size_t j = 0; j < this->BorderLandmasses[i].size(); ++j) {
			file.printf("%d, ", this->BorderLandmasses[i][j]);
		}
		file.printf("},\n");
	}
	file.printf("  },\n");
	//Wyrmgus end

	file.printf("  \"map-fields\", {\n");
	//Wyrmgus start
	/*
	for (int h = 0; h < this->Info.MapHeight; ++h) {
		file.printf("  -- %d\n", h);
		for (int w = 0; w < this->Info.MapWidth; ++w) {
			const CMapField &mf = *this->Field(w, h);

			mf.Save(file);
			if (w & 1) {
				file.printf(",\n");
			} else {
				file.printf(", ");
			}
		}
	}
	*/
	for (size_t z = 0; z < this->MapLayers.size(); ++z) {
		file.printf("  {\n");
		for (int h = 0; h < this->Info.MapHeights[z]; ++h) {
			file.printf("  -- %d\n", h);
			for (int w = 0; w < this->Info.MapWidths[z]; ++w) {
				const CMapField &mf = *this->Field(w, h, z);

				mf.Save(file);
				if (w & 1) {
					file.printf(",\n");
				} else {
					file.printf(", ");
				}
			}
		}
		file.printf("  },\n");
	}
	//Wyrmgus end
	file.printf("}})\n");
}

/*----------------------------------------------------------------------------
-- Map Tile Update Functions
----------------------------------------------------------------------------*/

/**
**  Correct the seen wood field, depending on the surrounding.
**
**  @param type  type of tile to update
**  @param seen  1 if updating seen value, 0 for real
**  @param pos   Map tile-position.
*/
//Wyrmgus start
/*
void CMap::FixTile(unsigned short type, int seen, const Vec2i &pos)
{
	Assert(type == MapFieldForest || type == MapFieldRocks);

	//  Outside of map or no wood.
	if (!Info.IsPointOnMap(pos)) {
		return;
	}
	unsigned int index = getIndex(pos);
	CMapField &mf = *this->Field(index);

	if (!((type == MapFieldForest && Tileset->isAWoodTile(mf.playerInfo.SeenTile))
		  || (type == MapFieldRocks && Tileset->isARockTile(mf.playerInfo.SeenTile)))) {
		if (seen) {
			return;
		}
	}

	if (!seen && !(mf.getFlag() & type)) {
		return;
	}

	// Select Table to lookup
	int removedtile;
	int flags;
	if (type == MapFieldForest) {
		removedtile = this->Tileset->getRemovedTreeTile();
		flags = (MapFieldForest | MapFieldUnpassable);
	} else { // (type == MapFieldRocks)
		removedtile = this->Tileset->getRemovedRockTile();
		flags = (MapFieldRocks | MapFieldUnpassable);
	}
	//  Find out what each tile has with respect to wood, or grass.
	int ttup;
	int ttdown;
	int ttleft;
	int ttright;

	if (pos.y - 1 < 0) {
		ttup = -1; //Assign trees in all directions
	} else {
		const CMapField &new_mf = *(&mf - this->Info.MapWidth);
		ttup = seen ? new_mf.playerInfo.SeenTile : new_mf.getGraphicTile();
	}
	if (pos.x + 1 >= this->Info.MapWidth) {
		ttright = -1; //Assign trees in all directions
	} else {
		const CMapField &new_mf = *(&mf + 1);
		ttright = seen ? new_mf.playerInfo.SeenTile : new_mf.getGraphicTile();
	}
	if (pos.y + 1 >= this->Info.MapHeight) {
		ttdown = -1; //Assign trees in all directions
	} else {
		const CMapField &new_mf = *(&mf + this->Info.MapWidth);
		ttdown = seen ? new_mf.playerInfo.SeenTile : new_mf.getGraphicTile();
	}
	if (pos.x - 1 < 0) {
		ttleft = -1; //Assign trees in all directions
	} else {
		const CMapField &new_mf = *(&mf - 1);
		ttleft = seen ? new_mf.playerInfo.SeenTile : new_mf.getGraphicTile();
	}
	int tile = this->Tileset->getTileBySurrounding(type, ttup, ttright, ttdown, ttleft);

	//Update seen tile.
	if (tile == -1) { // No valid wood remove it.
		if (seen) {
			mf.playerInfo.SeenTile = removedtile;
			this->FixNeighbors(type, seen, pos);
		} else {
			mf.setGraphicTile(removedtile);
			mf.Flags &= ~flags;
			mf.Value = 0;
			UI.Minimap.UpdateXY(pos);
		}
	} else if (seen && this->Tileset->isEquivalentTile(tile, mf.playerInfo.SeenTile)) { //Same Type
		return;
	} else {
		if (seen) {
			mf.playerInfo.SeenTile = tile;
		} else {
			mf.setGraphicTile(tile);
		}
	}

	//maybe isExplored
	if (mf.playerInfo.IsExplored(*ThisPlayer)) {
		UI.Minimap.UpdateSeenXY(pos);
		if (!seen) {
			MarkSeenTile(mf);
		}
	}
}
*/
//Wyrmgus end

/**
**  Correct the surrounding fields.
**
**  @param type  Tiletype of tile to adjust
**  @param seen  1 if updating seen value, 0 for real
**  @param pos   Map tile-position.
*/
//Wyrmgus start
/*
void CMap::FixNeighbors(unsigned short type, int seen, const Vec2i &pos)
{
	const Vec2i offset[] = {Vec2i(1, 0), Vec2i(-1, 0), Vec2i(0, 1), Vec2i(0, -1),
							Vec2i(-1, -1), Vec2i(-1, 1), Vec2i(1, -1), Vec2i(1, 1)
						   };

	for (unsigned int i = 0; i < sizeof(offset) / sizeof(*offset); ++i) {
		FixTile(type, seen, pos + offset[i]);
	}
}
*/
//Wyrmgus end

//Wyrmgus start
void CMap::SetTileTerrain(const Vec2i &pos, CTerrainType *terrain, int z)
{
	if (!terrain) {
		return;
	}
	
	CMapField &mf = *this->Field(pos, z);
	
	CTerrainType *old_terrain = this->GetTileTerrain(pos, terrain->Overlay, z);
	
	if (terrain->Overlay) {
		if (mf.OverlayTerrain == terrain) {
			return;
		}
	} else {
		if (mf.Terrain == terrain) {
			return;
		}
	}
	
	mf.SetTerrain(terrain);
	
	if (terrain->Overlay) {
		//remove decorations if the overlay terrain has changed
		std::vector<CUnit *> table;
		Select(pos, pos, table, z);
		for (size_t i = 0; i != table.size(); ++i) {
			if (table[i] && table[i]->IsAlive() && table[i]->Type->UnitType == UnitTypeType::Land && table[i]->Type->BoolFlag[DECORATION_INDEX].value) {
				if (Editor.Running == EditorNotRunning) {
					LetUnitDie(*table[i]);
				} else {
					EditorActionRemoveUnit(*table[i], false);
				}
			}
		}
	}
	
	this->CalculateTileTransitions(pos, false, z); //recalculate both, since one may have changed the other
	this->CalculateTileTransitions(pos, true, z);
	this->CalculateTileTerrainFeature(pos, z);
	
	if (mf.playerInfo.IsTeamVisible(*CPlayer::GetThisPlayer())) {
		MarkSeenTile(mf, z);
	}
	UI.Minimap.UpdateXY(pos, z);
	
	for (int x_offset = -1; x_offset <= 1; ++x_offset) {
		for (int y_offset = -1; y_offset <= 1; ++y_offset) {
			if (x_offset != 0 || y_offset != 0) {
				Vec2i adjacent_pos(pos.x + x_offset, pos.y + y_offset);
				if (Map.Info.IsPointOnMap(adjacent_pos, z)) {
					CMapField &adjacent_mf = *this->Field(adjacent_pos, z);
					
					if (terrain->Overlay && adjacent_mf.OverlayTerrain != terrain && Editor.Running == EditorNotRunning) {
						continue;
					}
					
					this->CalculateTileTransitions(adjacent_pos, false, z);
					this->CalculateTileTransitions(adjacent_pos, true, z);
					
					if (adjacent_mf.playerInfo.IsTeamVisible(*CPlayer::GetThisPlayer())) {
						MarkSeenTile(adjacent_mf, z);
					}
					UI.Minimap.UpdateXY(adjacent_pos, z);
				}
			}
		}
	}
	
	if (terrain->Overlay) {
		if ((terrain->Flags & MapFieldUnpassable) || (old_terrain && (old_terrain->Flags & MapFieldUnpassable))) {
			Map.CalculateTileOwnership(pos, z);
			
			for (int x_offset = -16; x_offset <= 16; ++x_offset) {
				for (int y_offset = -16; y_offset <= 16; ++y_offset) {
					if (x_offset != 0 || y_offset != 0) {
						Vec2i adjacent_pos(pos.x + x_offset, pos.y + y_offset);
						if (Map.Info.IsPointOnMap(adjacent_pos, z)) {
							Map.CalculateTileOwnership(adjacent_pos, z);
						}
					}
				}
			}
		}
	}
}

//Wyrmgus start
//void CMap::RemoveTileOverlayTerrain(const Vec2i &pos)
void CMap::RemoveTileOverlayTerrain(const Vec2i &pos, int z)
//Wyrmgus end
{
	CMapField &mf = *this->Field(pos, z);
	
	if (!mf.OverlayTerrain) {
		return;
	}
	
	CTerrainType *old_terrain = mf.OverlayTerrain;
	
	mf.RemoveOverlayTerrain();
	
	this->CalculateTileTransitions(pos, true, z);
	this->CalculateTileTerrainFeature(pos, z);
	
	if (mf.playerInfo.IsTeamVisible(*CPlayer::GetThisPlayer())) {
		MarkSeenTile(mf, z);
	}
	UI.Minimap.UpdateXY(pos, z);
	
	for (int x_offset = -1; x_offset <= 1; ++x_offset) {
		for (int y_offset = -1; y_offset <= 1; ++y_offset) {
			if (x_offset != 0 || y_offset != 0) {
				Vec2i adjacent_pos(pos.x + x_offset, pos.y + y_offset);
				if (Map.Info.IsPointOnMap(adjacent_pos, z)) {
					CMapField &adjacent_mf = *this->Field(adjacent_pos, z);
					
					this->CalculateTileTransitions(adjacent_pos, true, z);
					
					if (adjacent_mf.playerInfo.IsTeamVisible(*CPlayer::GetThisPlayer())) {
						MarkSeenTile(adjacent_mf, z);
					}
					UI.Minimap.UpdateXY(adjacent_pos, z);
				}
			}
		}
	}
	
	if (old_terrain->Flags & MapFieldUnpassable) {
		Map.CalculateTileOwnership(pos, z);
			
		for (int x_offset = -16; x_offset <= 16; ++x_offset) {
			for (int y_offset = -16; y_offset <= 16; ++y_offset) {
				if (x_offset != 0 || y_offset != 0) {
					Vec2i adjacent_pos(pos.x + x_offset, pos.y + y_offset);
					if (Map.Info.IsPointOnMap(adjacent_pos, z)) {
						Map.CalculateTileOwnership(adjacent_pos, z);
					}
				}
			}
		}
	}
}

void CMap::SetOverlayTerrainDestroyed(const Vec2i &pos, bool destroyed, int z)
{
	CMapLayer *map_layer = this->MapLayers[z];
	
	if (!map_layer) {
		return;
	}
	
	CMapField &mf = *map_layer->Field(pos);
	
	if (!mf.OverlayTerrain || mf.OverlayTerrainDestroyed == destroyed) {
		return;
	}
	
	mf.SetOverlayTerrainDestroyed(destroyed);
	
	if (destroyed) {
		if (mf.OverlayTerrain->Flags & MapFieldForest) {
			mf.Flags &= ~(MapFieldForest | MapFieldUnpassable);
			mf.Flags |= MapFieldStumps;
			map_layer->DestroyedForestTiles.push_back(pos);
		} else if (mf.OverlayTerrain->Flags & MapFieldRocks) {
			mf.Flags &= ~(MapFieldRocks | MapFieldUnpassable);
			mf.Flags |= MapFieldGravel;
		} else if (mf.OverlayTerrain->Flags & MapFieldWall) {
			mf.Flags &= ~(MapFieldWall | MapFieldUnpassable);
			if (GameSettings.Inside) {
				mf.Flags &= ~(MapFieldAirUnpassable);
			}
			mf.Flags |= MapFieldGravel;
		}
		mf.Value = 0;
	} else {
		if (mf.Flags & MapFieldStumps) { //if is a cleared tree tile regrowing trees
			mf.Flags &= ~(MapFieldStumps);
			mf.Flags |= MapFieldForest | MapFieldUnpassable;
			mf.Value = CResource::Resources[WoodCost]->DefaultAmount;
		}
	}
	
	this->CalculateTileTransitions(pos, true, z);
	
	if (mf.playerInfo.IsTeamVisible(*CPlayer::GetThisPlayer())) {
		MarkSeenTile(mf, z);
	}
	UI.Minimap.UpdateXY(pos, z);
	
	for (int x_offset = -1; x_offset <= 1; ++x_offset) {
		for (int y_offset = -1; y_offset <= 1; ++y_offset) {
			if (x_offset != 0 || y_offset != 0) {
				Vec2i adjacent_pos(pos.x + x_offset, pos.y + y_offset);
				if (Map.Info.IsPointOnMap(adjacent_pos, z)) {
					CMapField &adjacent_mf = *this->Field(adjacent_pos, z);
					
					if (adjacent_mf.OverlayTerrain != mf.OverlayTerrain) {
						continue;
					}
					
					this->CalculateTileTransitions(adjacent_pos, true, z);
					
					if (adjacent_mf.playerInfo.IsTeamVisible(*CPlayer::GetThisPlayer())) {
						MarkSeenTile(adjacent_mf, z);
					}
					UI.Minimap.UpdateXY(adjacent_pos, z);
				}
			}
		}
	}
	
	if (mf.OverlayTerrain->Flags & MapFieldUnpassable) {
		Map.CalculateTileOwnership(pos, z);
			
		for (int x_offset = -16; x_offset <= 16; ++x_offset) {
			for (int y_offset = -16; y_offset <= 16; ++y_offset) {
				if (x_offset != 0 || y_offset != 0) {
					Vec2i adjacent_pos(pos.x + x_offset, pos.y + y_offset);
					if (Map.Info.IsPointOnMap(adjacent_pos, z)) {
						Map.CalculateTileOwnership(adjacent_pos, z);
					}
				}
			}
		}
	}
}

void CMap::SetOverlayTerrainDamaged(const Vec2i &pos, bool damaged, int z)
{
	CMapField &mf = *this->Field(pos, z);
	
	if (!mf.OverlayTerrain || mf.OverlayTerrainDamaged == damaged) {
		return;
	}
	
	mf.SetOverlayTerrainDamaged(damaged);
	
	this->CalculateTileTransitions(pos, true, z);
	
	if (mf.playerInfo.IsTeamVisible(*CPlayer::GetThisPlayer())) {
		MarkSeenTile(mf, z);
	}
	UI.Minimap.UpdateXY(pos, z);
}

static int GetTransitionType(std::vector<int> &adjacent_directions, bool allow_single = false)
{
	if (adjacent_directions.size() == 0) {
		return -1;
	}
	
	int transition_type = -1;

	if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) != adjacent_directions.end()) {
		transition_type = SingleTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) != adjacent_directions.end()) {
		transition_type = NorthSingleTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) != adjacent_directions.end()) {
		transition_type = SouthSingleTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end()) {
		transition_type = WestSingleTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) != adjacent_directions.end()) {
		transition_type = EastSingleTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end()) {
		transition_type = NorthSouthTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end()) {
		transition_type = WestEastTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southwest) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southeast) != adjacent_directions.end()) {
		transition_type = NorthSouthwestInnerSoutheastInnerTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southwest) != adjacent_directions.end()) {
		transition_type = NorthSouthwestInnerTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southeast) != adjacent_directions.end()) {
		transition_type = NorthSoutheastInnerTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northwest) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northeast) != adjacent_directions.end()) {
		transition_type = SouthNorthwestInnerNortheastInnerTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northwest) != adjacent_directions.end()) {
		transition_type = SouthNorthwestInnerTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northeast) != adjacent_directions.end()) {
		transition_type = SouthNortheastInnerTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northeast) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southeast) != adjacent_directions.end()) {
		transition_type = WestNortheastInnerSoutheastInnerTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northeast) != adjacent_directions.end()) {
		transition_type = WestNortheastInnerTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southeast) != adjacent_directions.end()) {
		transition_type = WestSoutheastInnerTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northwest) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southwest) != adjacent_directions.end()) {
		transition_type = EastNorthwestInnerSouthwestInnerTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northwest) != adjacent_directions.end()) {
		transition_type = EastNorthwestInnerTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southwest) != adjacent_directions.end()) {
		transition_type = EastSouthwestInnerTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southeast) != adjacent_directions.end()) {
		transition_type = NorthwestOuterSoutheastInnerTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southwest) != adjacent_directions.end()) {
		transition_type = NortheastOuterSouthwestInnerTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northeast) != adjacent_directions.end()) {
		transition_type = SouthwestOuterNortheastInnerTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northwest) != adjacent_directions.end()) {
		transition_type = SoutheastOuterNorthwestInnerTransitionType;
	} else if (std::find(adjacent_directions.begin(), adjacent_directions.end(), North) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southwest) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southeast) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end()) {
		transition_type = NorthTransitionType;
	} else if (std::find(adjacent_directions.begin(), adjacent_directions.end(), South) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northwest) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northeast) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end()) {
		transition_type = SouthTransitionType;
	} else if (std::find(adjacent_directions.begin(), adjacent_directions.end(), West) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northeast) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southeast) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end()) {
		transition_type = WestTransitionType;
	} else if (std::find(adjacent_directions.begin(), adjacent_directions.end(), East) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northwest) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southwest) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end()) {
		transition_type = EastTransitionType;
	} else if ((std::find(adjacent_directions.begin(), adjacent_directions.end(), North) != adjacent_directions.end() || std::find(adjacent_directions.begin(), adjacent_directions.end(), West) != adjacent_directions.end()) && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southeast) == adjacent_directions.end()) {
		transition_type = NorthwestOuterTransitionType;
	} else if ((std::find(adjacent_directions.begin(), adjacent_directions.end(), North) != adjacent_directions.end() || std::find(adjacent_directions.begin(), adjacent_directions.end(), East) != adjacent_directions.end()) && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southwest) == adjacent_directions.end()) {
		transition_type = NortheastOuterTransitionType;
	} else if ((std::find(adjacent_directions.begin(), adjacent_directions.end(), South) != adjacent_directions.end() || std::find(adjacent_directions.begin(), adjacent_directions.end(), West) != adjacent_directions.end()) && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northeast) == adjacent_directions.end()) {
		transition_type = SouthwestOuterTransitionType;
	} else if ((std::find(adjacent_directions.begin(), adjacent_directions.end(), South) != adjacent_directions.end() || std::find(adjacent_directions.begin(), adjacent_directions.end(), East) != adjacent_directions.end()) && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northwest) == adjacent_directions.end()) {
		transition_type = SoutheastOuterTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northwest) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southeast) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northeast) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southwest) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end()) {
		transition_type = NorthwestNortheastSouthwestSoutheastInnerTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northwest) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southeast) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northeast) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southwest) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end()) {
		transition_type = NorthwestNortheastSouthwestInnerTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northwest) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southeast) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northeast) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southwest) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end()) {
		transition_type = NorthwestNortheastSoutheastInnerTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northwest) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southeast) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northeast) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southwest) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end()) {
		transition_type = NorthwestSouthwestSoutheastInnerTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northwest) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southeast) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northeast) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southwest) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end()) {
		transition_type = NortheastSouthwestSoutheastInnerTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northwest) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southeast) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northeast) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southwest) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end()) {
		transition_type = NorthwestNortheastInnerTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northwest) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southeast) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northeast) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southwest) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end()) {
		transition_type = SouthwestSoutheastInnerTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northwest) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southeast) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northeast) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southwest) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end()) {
		transition_type = NorthwestSouthwestInnerTransitionType;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northwest) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southeast) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northeast) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southwest) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end()) {
		transition_type = NortheastSoutheastInnerTransitionType;
	} else if (std::find(adjacent_directions.begin(), adjacent_directions.end(), Northwest) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southeast) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northeast) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southwest) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end()) {
		transition_type = NorthwestSoutheastInnerTransitionType;
	} else if (std::find(adjacent_directions.begin(), adjacent_directions.end(), Northeast) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southwest) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northwest) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southeast) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end()) {
		transition_type = NortheastSouthwestInnerTransitionType;
	} else if (std::find(adjacent_directions.begin(), adjacent_directions.end(), Northwest) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end()) {
		transition_type = NorthwestInnerTransitionType;
	} else if (std::find(adjacent_directions.begin(), adjacent_directions.end(), Northeast) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end()) {
		transition_type = NortheastInnerTransitionType;
	} else if (std::find(adjacent_directions.begin(), adjacent_directions.end(), Southwest) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end()) {
		transition_type = SouthwestInnerTransitionType;
	} else if (std::find(adjacent_directions.begin(), adjacent_directions.end(), Southeast) != adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end()) {
		transition_type = SoutheastInnerTransitionType;
	}

	return transition_type;
}

void CMap::CalculateTileTransitions(const Vec2i &pos, bool overlay, int z)
{
	CMapField &mf = *this->Field(pos, z);
	CTerrainType *terrain = nullptr;
	if (overlay) {
		terrain = mf.OverlayTerrain;
		mf.OverlayTransitionTiles.clear();
	} else {
		terrain = mf.Terrain;
		mf.TransitionTiles.clear();
	}
	
	if (!terrain || (overlay && mf.OverlayTerrainDestroyed)) {
		return;
	}
	
	int terrain_id = terrain->ID;
	
	std::map<int, std::vector<int>> adjacent_terrain_directions;
	
	for (int x_offset = -1; x_offset <= 1; ++x_offset) {
		for (int y_offset = -1; y_offset <= 1; ++y_offset) {
			if (x_offset != 0 || y_offset != 0) {
				Vec2i adjacent_pos(pos.x + x_offset, pos.y + y_offset);
				if (Map.Info.IsPointOnMap(adjacent_pos, z)) {
					CTerrainType *adjacent_terrain = this->GetTileTerrain(adjacent_pos, overlay, z);
					if (overlay && adjacent_terrain && this->Field(adjacent_pos, z)->OverlayTerrainDestroyed) {
						adjacent_terrain = nullptr;
					}
					if (adjacent_terrain && terrain != adjacent_terrain) {
						if (std::find(terrain->InnerBorderTerrains.begin(), terrain->InnerBorderTerrains.end(), adjacent_terrain) != terrain->InnerBorderTerrains.end()) {
							adjacent_terrain_directions[adjacent_terrain->ID].push_back(GetDirectionFromOffset(x_offset, y_offset));
						} else if (std::find(terrain->BorderTerrains.begin(), terrain->BorderTerrains.end(), adjacent_terrain) == terrain->BorderTerrains.end()) { //if the two terrain types can't border, look for a third terrain type which can border both, and which treats both as outer border terrains, and then use for transitions between both tiles
							for (const CTerrainType *border_terrain : terrain->BorderTerrains) {
								if (std::find(terrain->InnerBorderTerrains.begin(), terrain->InnerBorderTerrains.end(), border_terrain) != terrain->InnerBorderTerrains.end() && std::find(adjacent_terrain->InnerBorderTerrains.begin(), adjacent_terrain->InnerBorderTerrains.end(), border_terrain) != adjacent_terrain->InnerBorderTerrains.end()) {
									adjacent_terrain_directions[border_terrain->ID].push_back(GetDirectionFromOffset(x_offset, y_offset));
									break;
								}
							}
						}
					}
					if (!adjacent_terrain || (overlay && terrain != adjacent_terrain && std::find(terrain->BorderTerrains.begin(), terrain->BorderTerrains.end(), adjacent_terrain) == terrain->BorderTerrains.end())) { // happens if terrain is null or if it is an overlay tile which doesn't have a border with this one, so that i.e. tree transitions display correctly when adjacent to tiles without overlays
						adjacent_terrain_directions[CTerrainType::TerrainTypes.size()].push_back(GetDirectionFromOffset(x_offset, y_offset));
					}
				}
			}
		}
	}
	
	for (std::map<int, std::vector<int>>::iterator iterator = adjacent_terrain_directions.begin(); iterator != adjacent_terrain_directions.end(); ++iterator) {
		int adjacent_terrain_id = iterator->first;
		CTerrainType *adjacent_terrain = adjacent_terrain_id < (int) CTerrainType::TerrainTypes.size() ? CTerrainType::TerrainTypes[adjacent_terrain_id] : nullptr;
		int transition_type = GetTransitionType(iterator->second, terrain->AllowSingle);
		
		if (transition_type != -1) {
			bool found_transition = false;
			
			if (!overlay) {
				if (adjacent_terrain != nullptr) {
					auto transition_tiles_find_iterator = terrain->TransitionTiles.find(std::tuple<int, int>(adjacent_terrain_id, transition_type));
					if (transition_tiles_find_iterator != terrain->TransitionTiles.end() && transition_tiles_find_iterator->second.size() > 0) {
						mf.TransitionTiles.push_back(std::pair<CTerrainType *, int>(terrain, transition_tiles_find_iterator->second[SyncRand(transition_tiles_find_iterator->second.size())]));
						found_transition = true;
					} else {
						auto adjacent_transition_tiles_find_iterator = adjacent_terrain->AdjacentTransitionTiles.find(std::tuple<int, int>(terrain_id, transition_type));
						if (adjacent_transition_tiles_find_iterator != adjacent_terrain->AdjacentTransitionTiles.end() && adjacent_transition_tiles_find_iterator->second.size() > 0) {
							mf.TransitionTiles.push_back(std::pair<CTerrainType *, int>(adjacent_terrain, adjacent_transition_tiles_find_iterator->second[SyncRand(adjacent_transition_tiles_find_iterator->second.size())]));
							found_transition = true;
						} else {
							adjacent_transition_tiles_find_iterator = adjacent_terrain->AdjacentTransitionTiles.find(std::tuple<int, int>(-1, transition_type));
							if (adjacent_transition_tiles_find_iterator != adjacent_terrain->AdjacentTransitionTiles.end() && adjacent_transition_tiles_find_iterator->second.size() > 0) {
								mf.TransitionTiles.push_back(std::pair<CTerrainType *, int>(adjacent_terrain, adjacent_transition_tiles_find_iterator->second[SyncRand(adjacent_transition_tiles_find_iterator->second.size())]));
								found_transition = true;
							}
						}
					}
				} else {
					auto transition_tiles_find_iterator = terrain->TransitionTiles.find(std::tuple<int, int>(-1, transition_type));
					if (transition_tiles_find_iterator != terrain->TransitionTiles.end() && transition_tiles_find_iterator->second.size() > 0) {
						mf.TransitionTiles.push_back(std::pair<CTerrainType *, int>(terrain, transition_tiles_find_iterator->second[SyncRand(transition_tiles_find_iterator->second.size())]));
					}
				}
			} else {
				if (adjacent_terrain != nullptr) {
					auto transition_tiles_find_iterator = terrain->TransitionTiles.find(std::tuple<int, int>(adjacent_terrain_id, transition_type));
					if (transition_tiles_find_iterator != terrain->TransitionTiles.end() && transition_tiles_find_iterator->second.size() > 0) {
						mf.OverlayTransitionTiles.push_back(std::pair<CTerrainType *, int>(terrain, transition_tiles_find_iterator->second[SyncRand(transition_tiles_find_iterator->second.size())]));
						found_transition = true;
					} else {
						auto adjacent_transition_tiles_find_iterator = adjacent_terrain->AdjacentTransitionTiles.find(std::tuple<int, int>(terrain_id, transition_type));
						if (adjacent_transition_tiles_find_iterator != adjacent_terrain->AdjacentTransitionTiles.end() && adjacent_transition_tiles_find_iterator->second.size() > 0) {
							mf.OverlayTransitionTiles.push_back(std::pair<CTerrainType *, int>(adjacent_terrain, adjacent_transition_tiles_find_iterator->second[SyncRand(adjacent_transition_tiles_find_iterator->second.size())]));
							found_transition = true;
						} else {
							adjacent_transition_tiles_find_iterator = adjacent_terrain->AdjacentTransitionTiles.find(std::tuple<int, int>(-1, transition_type));
							if (adjacent_transition_tiles_find_iterator != adjacent_terrain->AdjacentTransitionTiles.end() && adjacent_transition_tiles_find_iterator->second.size() > 0) {
								mf.OverlayTransitionTiles.push_back(std::pair<CTerrainType *, int>(adjacent_terrain, adjacent_transition_tiles_find_iterator->second[SyncRand(adjacent_transition_tiles_find_iterator->second.size())]));
								found_transition = true;
							}
						}
					}
				} else {
					auto transition_tiles_find_iterator = terrain->TransitionTiles.find(std::tuple<int, int>(-1, transition_type));
					if (transition_tiles_find_iterator != terrain->TransitionTiles.end() && transition_tiles_find_iterator->second.size() > 0) {
						mf.OverlayTransitionTiles.push_back(std::pair<CTerrainType *, int>(terrain, transition_tiles_find_iterator->second[SyncRand(transition_tiles_find_iterator->second.size())]));
					}
				}
				
				if ((mf.Flags & MapFieldWaterAllowed) && (!adjacent_terrain || !(adjacent_terrain->Flags & MapFieldWaterAllowed))) { //if this is a water tile adjacent to a non-water tile, replace the water flag with a coast one
					mf.Flags &= ~(MapFieldWaterAllowed);
					mf.Flags |= MapFieldCoastAllowed;
				}
			}
			
			if (adjacent_terrain && found_transition) {
				for (size_t i = 0; i != iterator->second.size(); ++i) {
					adjacent_terrain_directions[CTerrainType::TerrainTypes.size()].erase(std::remove(adjacent_terrain_directions[CTerrainType::TerrainTypes.size()].begin(), adjacent_terrain_directions[CTerrainType::TerrainTypes.size()].end(), iterator->second[i]), adjacent_terrain_directions[CTerrainType::TerrainTypes.size()].end());
				}
			}
		}
	}
	
	//sort the transitions so that they will be displayed in the correct order
	if (overlay) {
		bool swapped = true;
		for (int passes = 0; passes < (int) mf.OverlayTransitionTiles.size() && swapped; ++passes) {
			swapped = false;
			for (int i = 0; i < ((int) mf.OverlayTransitionTiles.size()) - 1; ++i) {
				bool change_order = false;
				if (std::find(mf.OverlayTransitionTiles[i + 1].first->InnerBorderTerrains.begin(), mf.OverlayTransitionTiles[i + 1].first->InnerBorderTerrains.end(), mf.OverlayTransitionTiles[i].first) != mf.OverlayTransitionTiles[i + 1].first->InnerBorderTerrains.end()) {
					std::pair<CTerrainType *, int> temp_transition = mf.OverlayTransitionTiles[i];
					mf.OverlayTransitionTiles[i] = mf.OverlayTransitionTiles[i + 1];
					mf.OverlayTransitionTiles[i + 1] = temp_transition;
					swapped = true;
				}
			}
		}
	} else {
		bool swapped = true;
		for (int passes = 0; passes < (int) mf.TransitionTiles.size() && swapped; ++passes) {
			swapped = false;
			for (int i = 0; i < ((int) mf.TransitionTiles.size()) - 1; ++i) {
				bool change_order = false;
				if (std::find(mf.TransitionTiles[i + 1].first->InnerBorderTerrains.begin(), mf.TransitionTiles[i + 1].first->InnerBorderTerrains.end(), mf.TransitionTiles[i].first) != mf.TransitionTiles[i + 1].first->InnerBorderTerrains.end()) {
					std::pair<CTerrainType *, int> temp_transition = mf.TransitionTiles[i];
					mf.TransitionTiles[i] = mf.TransitionTiles[i + 1];
					mf.TransitionTiles[i + 1] = temp_transition;
					swapped = true;
				}
			}
		}
	}
}

void CMap::CalculateTileLandmass(const Vec2i &pos, int z)
{
	if (!this->Info.IsPointOnMap(pos, z)) {
		return;
	}
	
	if (Editor.Running != EditorNotRunning) { //no need to assign landmasses while in the editor
		return;
	}
	
	CMapField &mf = *this->Field(pos, z);

	if (mf.Landmass != 0) {
		return; //already calculated
	}
	
	bool is_water = (mf.Flags & MapFieldWaterAllowed) || (mf.Flags & MapFieldCoastAllowed);

	//doesn't have a landmass ID, and hasn't inherited one from another tile yet, so add a new one
	mf.Landmass = this->Landmasses + 1;
	this->Landmasses += 1;
	this->BorderLandmasses.resize(this->Landmasses + 1);
	//now, spread the new landmass ID to neighboring land tiles
	std::vector<Vec2i> landmass_tiles;
	landmass_tiles.push_back(pos);
	//calculate the landmass of any neighboring land tiles with no set landmass as well
	for (size_t i = 0; i < landmass_tiles.size(); ++i) {
		for (int x_offset = -1; x_offset <= 1; ++x_offset) {
			for (int y_offset = -1; y_offset <= 1; ++y_offset) {
				if (x_offset != 0 || y_offset != 0) {
					Vec2i adjacent_pos(landmass_tiles[i].x + x_offset, landmass_tiles[i].y + y_offset);
					if (this->Info.IsPointOnMap(adjacent_pos, z)) {
						CMapField &adjacent_mf = *this->Field(adjacent_pos, z);
						bool adjacent_is_water = (adjacent_mf.Flags & MapFieldWaterAllowed) || (adjacent_mf.Flags & MapFieldCoastAllowed);
									
						if (adjacent_is_water == is_water && adjacent_mf.Landmass == 0) {
							adjacent_mf.Landmass = mf.Landmass;
							landmass_tiles.push_back(adjacent_pos);
						} else if (adjacent_is_water != is_water && adjacent_mf.Landmass != 0 && std::find(this->BorderLandmasses[mf.Landmass].begin(), this->BorderLandmasses[mf.Landmass].end(), adjacent_mf.Landmass) == this->BorderLandmasses[mf.Landmass].end()) {
							this->BorderLandmasses[mf.Landmass].push_back(adjacent_mf.Landmass);
							this->BorderLandmasses[adjacent_mf.Landmass].push_back(mf.Landmass);
						}
					}
				}
			}
		}
	}
}

void CMap::CalculateTileTerrainFeature(const Vec2i &pos, int z)
{
	if (!this->Info.IsPointOnMap(pos, z)) {
		return;
	}
	
	if (Editor.Running != EditorNotRunning) { //no need to assign terrain features while in the editor
		return;
	}
	
	CMapField &mf = *this->Field(pos, z);

	if (mf.TerrainFeature) {
		return; //already has a terrain feature
	}

	//if any adjacent tile the same top terrain as this one, and has a terrain feature, then use that
	for (int x_offset = -1; x_offset <= 1; ++x_offset) {
		for (int y_offset = -1; y_offset <= 1; ++y_offset) {
			if ((x_offset != 0 || y_offset != 0) && !(x_offset != 0 && y_offset != 0)) { //only directly adjacent tiles (no diagonal ones, and not the same tile)
				Vec2i adjacent_pos(pos.x + x_offset, pos.y + y_offset);
				if (Map.Info.IsPointOnMap(adjacent_pos, z)) {
					CMapField &adjacent_mf = *this->Field(adjacent_pos, z);

					if (adjacent_mf.TerrainFeature && adjacent_mf.TerrainFeature->TerrainType == GetTileTopTerrain(pos, false, z)) {
						mf.TerrainFeature = adjacent_mf.TerrainFeature;
						return;
					}
				}
			}
		}
	}
}

void CMap::CalculateTileOwnership(const Vec2i &pos, int z)
{
	if (!this->Info.IsPointOnMap(pos, z)) {
		return;
	}

	CMapField &mf = *this->Field(pos, z);
	
	int new_owner = -1;
	bool must_have_no_owner = false;
	
	if (mf.Flags & MapFieldBuilding) { //make sure the place a building is located is set to be owned by its player; this is necessary for scenarios, since when they start buildings could be on another player's territory (i.e. if a farm starts next to a town hall)
		const CUnitCache &cache = mf.UnitCache;
		for (size_t i = 0; i != cache.size(); ++i) {
			CUnit *unit = cache[i];
			if (!unit) {
				fprintf(stderr, "Error in CMap::CalculateTileOwnership (pos %d, %d): a unit in the tile's unit cache is null.\n", pos.x, pos.y);
			}
			if (unit->IsAliveOnMap() && unit->Type->BoolFlag[BUILDING_INDEX].value) {
				if (unit->Variable[OWNERSHIPINFLUENCERANGE_INDEX].Value && unit->Player->Index != PlayerNumNeutral) {
					new_owner = unit->Player->Index;
					break;
				} else if (unit->Player->Index == PlayerNumNeutral && (unit->Type == SettlementSiteUnitType || (unit->Type->GivesResource && !unit->Type->BoolFlag[CANHARVEST_INDEX].value))) { //there cannot be an owner for the tile of a (neutral) settlement site or deposit, otherwise players might not be able to build over them
					must_have_no_owner = true;
					break;
				}
			}
		}
	}

	if (new_owner == -1 && !must_have_no_owner) { //if no building is on the tile, set it to the first unit to have influence on it, if that isn't blocked by an obstacle
		std::vector<unsigned long> obstacle_flags;
		obstacle_flags.push_back(MapFieldCoastAllowed);
		obstacle_flags.push_back(MapFieldUnpassable);

		std::vector<CUnit *> table;
		Select(pos - Vec2i(16, 16), pos + Vec2i(16, 16), table, z);
		for (size_t i = 0; i != table.size(); ++i) {
			CUnit *unit = table[i];
			if (!unit) {
				fprintf(stderr, "Error in CMap::CalculateTileOwnership (pos %d, %d): a unit within the tile's range is null.\n", pos.x, pos.y);
			}
			if (unit->IsAliveOnMap() && unit->Variable[OWNERSHIPINFLUENCERANGE_INDEX].Value > 0 && unit->MapDistanceTo(pos, z) <= unit->Variable[OWNERSHIPINFLUENCERANGE_INDEX].Value) {
				bool obstacle_check = true;
				for (size_t j = 0; j < obstacle_flags.size(); ++j) {
					bool obstacle_subcheck = false;
					for (int x = 0; x < unit->Type->TileSize.x; ++x) {
						for (int y = 0; y < unit->Type->TileSize.y; ++y) {
							if (CheckObstaclesBetweenTiles(unit->tilePos + Vec2i(x, y), pos, obstacle_flags[j], z, 0, nullptr, unit->Player->Index)) { //the obstacle must be avoidable from at least one of the unit's tiles
								obstacle_subcheck = true;
								break;
							}
						}
						if (obstacle_subcheck) {
							break;
						}
					}
					if (!obstacle_subcheck) {
						obstacle_check = false;
						break;
					}
				}
				if (!obstacle_check) {
					continue;
				}
				new_owner = unit->Player->Index;
				break;
			}
		}
	}
	
	if (new_owner != mf.Owner) {
		mf.Owner = new_owner;
		
		this->CalculateTileOwnershipTransition(pos, z);
		
		for (int x_offset = -1; x_offset <= 1; ++x_offset) {
			for (int y_offset = -1; y_offset <= 1; ++y_offset) {
				if (x_offset != 0 || y_offset != 0) {
					Vec2i adjacent_pos(pos.x + x_offset, pos.y + y_offset);
					if (Map.Info.IsPointOnMap(adjacent_pos, z)) {
						CMapField &adjacent_mf = *this->Field(adjacent_pos, z);
							
						this->CalculateTileOwnershipTransition(adjacent_pos, z);
					}
				}
			}
		}
	}
}

void CMap::CalculateTileOwnershipTransition(const Vec2i &pos, int z)
{
	if (!this->Info.IsPointOnMap(pos, z)) {
		return;
	}
	
	if (Editor.Running != EditorNotRunning) { //no need to assign ownership transitions while in the editor
		return;
	}
	
	CMapField &mf = *this->Field(pos, z);
	
	mf.OwnershipBorderTile = -1;

	if (mf.Owner == -1) {
		return;
	}
	
	std::vector<int> adjacent_directions;
	
	for (int x_offset = -1; x_offset <= 1; ++x_offset) {
		for (int y_offset = -1; y_offset <= 1; ++y_offset) {
			if (x_offset != 0 || y_offset != 0) {
				Vec2i adjacent_pos(pos.x + x_offset, pos.y + y_offset);
				if (Map.Info.IsPointOnMap(adjacent_pos, z)) {
					CMapField &adjacent_mf = *this->Field(adjacent_pos, z);
					if (adjacent_mf.Owner != mf.Owner) {
						adjacent_directions.push_back(GetDirectionFromOffset(x_offset, y_offset));
					}
				}
			}
		}
	}
	
	int transition_type = GetTransitionType(adjacent_directions, true);
	
	if (transition_type != -1) {
		if (Map.BorderTerrain->TransitionTiles[std::tuple<int, int>(-1, transition_type)].size() > 0) {
			mf.OwnershipBorderTile = Map.BorderTerrain->TransitionTiles[std::tuple<int, int>(-1, transition_type)][SyncRand(Map.BorderTerrain->TransitionTiles[std::tuple<int, int>(-1, transition_type)].size())];
		}
	}
}

void CMap::AdjustMap()
{
	for (size_t z = 0; z < this->MapLayers.size(); ++z) {
		Vec2i map_start_pos(0, 0);
		Vec2i map_end(this->Info.MapWidths[z], this->Info.MapHeights[z]);
		
		this->AdjustTileMapIrregularities(false, map_start_pos, map_end, z);
		this->AdjustTileMapIrregularities(true, map_start_pos, map_end, z);
		this->AdjustTileMapTransitions(map_start_pos, map_end, z);
		this->AdjustTileMapIrregularities(false, map_start_pos, map_end, z);
		this->AdjustTileMapIrregularities(true, map_start_pos, map_end, z);
	}
}

void CMap::AdjustTileMapIrregularities(bool overlay, const Vec2i &min_pos, const Vec2i &max_pos, int z)
{
	bool no_irregularities_found = false;
	while (!no_irregularities_found) {
		no_irregularities_found = true;
		for (int x = min_pos.x; x < max_pos.x; ++x) {
			for (int y = min_pos.y; y < max_pos.y; ++y) {
				CMapField &mf = *this->Field(x, y, z);
				CTerrainType *terrain = overlay ? mf.OverlayTerrain : mf.Terrain;
				if (!terrain || terrain->AllowSingle) {
					continue;
				}
				std::vector<CTerrainType *> acceptable_adjacent_tile_types;
				acceptable_adjacent_tile_types.push_back(terrain);
				for (size_t i = 0; i < terrain->OuterBorderTerrains.size(); ++i) {
					acceptable_adjacent_tile_types.push_back(terrain->OuterBorderTerrains[i]);
				}
				
				int horizontal_adjacent_tiles = 0;
				int vertical_adjacent_tiles = 0;
				int nw_quadrant_adjacent_tiles = 0; //should be 4 if the wrong tile types are present in X-1,Y; X-1,Y-1; X,Y-1; and X+1,Y+1
				int ne_quadrant_adjacent_tiles = 0;
				int sw_quadrant_adjacent_tiles = 0;
				int se_quadrant_adjacent_tiles = 0;
				
				if ((x - 1) >= 0 && std::find(acceptable_adjacent_tile_types.begin(), acceptable_adjacent_tile_types.end(), this->GetTileTerrain(Vec2i(x - 1, y), overlay, z)) == acceptable_adjacent_tile_types.end()) {
					horizontal_adjacent_tiles += 1;
					nw_quadrant_adjacent_tiles += 1;
					sw_quadrant_adjacent_tiles += 1;
				}
				if ((x + 1) < this->Info.MapWidths[z] && std::find(acceptable_adjacent_tile_types.begin(), acceptable_adjacent_tile_types.end(), this->GetTileTerrain(Vec2i(x + 1, y), overlay, z)) == acceptable_adjacent_tile_types.end()) {
					horizontal_adjacent_tiles += 1;
					ne_quadrant_adjacent_tiles += 1;
					se_quadrant_adjacent_tiles += 1;
				}
				
				if ((y - 1) >= 0 && std::find(acceptable_adjacent_tile_types.begin(), acceptable_adjacent_tile_types.end(), this->GetTileTerrain(Vec2i(x, y - 1), overlay, z)) == acceptable_adjacent_tile_types.end()) {
					vertical_adjacent_tiles += 1;
					nw_quadrant_adjacent_tiles += 1;
					ne_quadrant_adjacent_tiles += 1;
				}
				if ((y + 1) < this->Info.MapHeights[z] && std::find(acceptable_adjacent_tile_types.begin(), acceptable_adjacent_tile_types.end(), this->GetTileTerrain(Vec2i(x, y + 1), overlay, z)) == acceptable_adjacent_tile_types.end()) {
					vertical_adjacent_tiles += 1;
					sw_quadrant_adjacent_tiles += 1;
					se_quadrant_adjacent_tiles += 1;
				}

				if ((x - 1) >= 0 && (y - 1) >= 0 && std::find(acceptable_adjacent_tile_types.begin(), acceptable_adjacent_tile_types.end(), this->GetTileTerrain(Vec2i(x - 1, y - 1), overlay, z)) == acceptable_adjacent_tile_types.end()) {
					nw_quadrant_adjacent_tiles += 1;
					se_quadrant_adjacent_tiles += 1;
				}
				if ((x - 1) >= 0 && (y + 1) < this->Info.MapHeights[z] && std::find(acceptable_adjacent_tile_types.begin(), acceptable_adjacent_tile_types.end(), GetTileTerrain(Vec2i(x - 1, y + 1), overlay, z)) == acceptable_adjacent_tile_types.end()) {
					sw_quadrant_adjacent_tiles += 1;
					ne_quadrant_adjacent_tiles += 1;
				}
				if ((x + 1) < this->Info.MapWidths[z] && (y - 1) >= 0 && std::find(acceptable_adjacent_tile_types.begin(), acceptable_adjacent_tile_types.end(), GetTileTerrain(Vec2i(x + 1, y - 1), overlay, z)) == acceptable_adjacent_tile_types.end()) {
					ne_quadrant_adjacent_tiles += 1;
					sw_quadrant_adjacent_tiles += 1;
				}
				if ((x + 1) < this->Info.MapWidths[z] && (y + 1) < this->Info.MapHeights[z] && std::find(acceptable_adjacent_tile_types.begin(), acceptable_adjacent_tile_types.end(), GetTileTerrain(Vec2i(x + 1, y + 1), overlay, z)) == acceptable_adjacent_tile_types.end()) {
					se_quadrant_adjacent_tiles += 1;
					nw_quadrant_adjacent_tiles += 1;
				}
				
				
				if (horizontal_adjacent_tiles >= 2 || vertical_adjacent_tiles >= 2 || nw_quadrant_adjacent_tiles >= 4 || ne_quadrant_adjacent_tiles >= 4 || sw_quadrant_adjacent_tiles >= 4 || se_quadrant_adjacent_tiles >= 4) {
					if (overlay) {
						mf.RemoveOverlayTerrain();
					} else {
						bool changed_terrain = false;
						for (int sub_x = -1; sub_x <= 1; ++sub_x) {
							for (int sub_y = -1; sub_y <= 1; ++sub_y) {
								if ((x + sub_x) < min_pos.x || (x + sub_x) >= max_pos.x || (y + sub_y) < min_pos.y || (y + sub_y) >= max_pos.y || (sub_x == 0 && sub_y == 0)) {
									continue;
								}
								CTerrainType *tile_terrain = GetTileTerrain(Vec2i(x + sub_x, y + sub_y), false, z);
								if (mf.Terrain != tile_terrain) {
									if (std::find(mf.Terrain->InnerBorderTerrains.begin(), mf.Terrain->InnerBorderTerrains.end(), tile_terrain) != mf.Terrain->InnerBorderTerrains.end()) {
										mf.SetTerrain(tile_terrain);
										changed_terrain = true;
										break;
									}
								}
							}
							if (changed_terrain) {
								break;
							}
						}
						if (!changed_terrain && terrain->InnerBorderTerrains.size() > 0) {
							mf.SetTerrain(terrain->InnerBorderTerrains[0]);
						}
					}
					no_irregularities_found = false;
				}
			}
		}
	}
}

void CMap::AdjustTileMapTransitions(const Vec2i &min_pos, const Vec2i &max_pos, int z)
{
	for (int x = min_pos.x; x < max_pos.x; ++x) {
		for (int y = min_pos.y; y < max_pos.y; ++y) {
			CMapField &mf = *this->Field(x, y, z);

			for (int sub_x = -1; sub_x <= 1; ++sub_x) {
				for (int sub_y = -1; sub_y <= 1; ++sub_y) {
					if ((x + sub_x) < min_pos.x || (x + sub_x) >= max_pos.x || (y + sub_y) < min_pos.y || (y + sub_y) >= max_pos.y || (sub_x == 0 && sub_y == 0)) {
						continue;
					}
					CTerrainType *tile_terrain = GetTileTerrain(Vec2i(x + sub_x, y + sub_y), false, z);
					CTerrainType *tile_top_terrain = GetTileTopTerrain(Vec2i(x + sub_x, y + sub_y), false, z);
					if (
						mf.Terrain != tile_terrain
						&& tile_top_terrain->Overlay
						&& tile_top_terrain != mf.OverlayTerrain
						&& std::find(tile_terrain->OuterBorderTerrains.begin(), tile_terrain->OuterBorderTerrains.end(), mf.Terrain) == tile_terrain->OuterBorderTerrains.end()
						&& std::find(tile_top_terrain->BaseTerrainTypes.begin(), tile_top_terrain->BaseTerrainTypes.end(), mf.Terrain) == tile_top_terrain->BaseTerrainTypes.end()
					) {
						mf.SetTerrain(tile_terrain);
					}
				}
			}
		}
	}

	for (int x = min_pos.x; x < max_pos.x; ++x) {
		for (int y = min_pos.y; y < max_pos.y; ++y) {
			CMapField &mf = *this->Field(x, y, z);

			for (int sub_x = -1; sub_x <= 1; ++sub_x) {
				for (int sub_y = -1; sub_y <= 1; ++sub_y) {
					if ((x + sub_x) < min_pos.x || (x + sub_x) >= max_pos.x || (y + sub_y) < min_pos.y || (y + sub_y) >= max_pos.y || (sub_x == 0 && sub_y == 0)) {
						continue;
					}
					CTerrainType *tile_terrain = GetTileTerrain(Vec2i(x + sub_x, y + sub_y), false, z);
					if (mf.Terrain != tile_terrain && std::find(mf.Terrain->BorderTerrains.begin(), mf.Terrain->BorderTerrains.end(), tile_terrain) == mf.Terrain->BorderTerrains.end()) {
						for (size_t i = 0; i < mf.Terrain->BorderTerrains.size(); ++i) {
							CTerrainType *border_terrain = mf.Terrain->BorderTerrains[i];
							if (std::find(border_terrain->BorderTerrains.begin(), border_terrain->BorderTerrains.end(), mf.Terrain) != border_terrain->BorderTerrains.end() && std::find(border_terrain->BorderTerrains.begin(), border_terrain->BorderTerrains.end(), tile_terrain) != border_terrain->BorderTerrains.end()) {
								mf.SetTerrain(border_terrain);
								break;
							}
						}
					}
				}
			}
		}
	}
}

/**
**	@brief	Generate a given terrain on the map
**
**	@param	generated_terrain	The terrain generation characteristics
**	@param	min_pos				The minimum position in the map to generate the terrain on
**	@param	max_pos				The maximum position in the map to generate the terrain on
**	@param	preserve_coastline	Whether to avoid changing the coastline during terrain generation
**	@param	z					The map layer to generate the terrain on
*/
void CMap::GenerateTerrain(const CGeneratedTerrain *generated_terrain, const Vec2i &min_pos, const Vec2i &max_pos, const bool preserve_coastline, const int z)
{
	if (SaveGameLoading) {
		return;
	}
	
	CTerrainType *terrain_type = generated_terrain->TerrainType;
	const int seed_count = generated_terrain->SeedCount;
	const int max_tile_quantity = (max_pos.x + 1 - min_pos.x) * (max_pos.y + 1 - min_pos.y) * generated_terrain->MaxPercent / 100;
	int tile_quantity = 0;
	
	Vec2i random_pos(0, 0);
	int count = seed_count;
	
	std::vector<Vec2i> seeds;
	
	if (generated_terrain->UseExistingAsSeeds) { //use existing tiles of the given terrain as seeds for the terrain generation
		for (int x = min_pos.x; x <= max_pos.x; ++x) {
			for (int y = min_pos.y; y <= max_pos.y; ++y) {
				const Vec2i tile_pos(x, y);
				const CMapField *tile = this->Field(x, y, z);
				
				if (max_tile_quantity != 0 && tile->GetTopTerrain() == terrain_type) {
					tile_quantity++;
				}
				
				if (!generated_terrain->CanUseTileAsSeed(tile)) {
					continue;
				}
				
				if (this->is_point_in_a_subtemplate_area(tile_pos, z)) {
					continue;
				}
				
				seeds.push_back(tile_pos);
			}
		}
	}
	
	if (generated_terrain->UseSubtemplateBordersAsSeeds) {
		for (size_t i = 0; i < this->MapLayers[z]->subtemplate_areas.size(); ++i) {
			const Vec2i subtemplate_min_pos = std::get<0>(this->MapLayers[z]->subtemplate_areas[i]);
			const Vec2i subtemplate_max_pos = std::get<1>(this->MapLayers[z]->subtemplate_areas[i]);
			
			for (int x = subtemplate_min_pos.x; x <= subtemplate_max_pos.x; ++x) {
				for (int y = subtemplate_min_pos.y; y <= subtemplate_max_pos.y; ++y) {
					const Vec2i tile_pos(x, y);
					const CMapField *tile = this->Field(x, y, z);
					
					if (!generated_terrain->CanUseTileAsSeed(tile)) {
						continue;
					}
					
					if (!this->is_point_adjacent_to_non_subtemplate_area(tile_pos, z)) {
						continue;
					}
					
					seeds.push_back(tile_pos);
				}
			}
		}
	}
	
	std::vector<Vec2i> potential_positions;
	for (int x = min_pos.x; x <= max_pos.x; ++x) {
		for (int y = min_pos.y; y <= max_pos.y; ++y) {
			potential_positions.push_back(Vec2i(x, y));
		}
	}
	
	// create initial seeds
	while (count > 0 && !potential_positions.empty()) {
		if (max_tile_quantity != 0 && tile_quantity >= max_tile_quantity) {
			break;
		}
		
		random_pos = potential_positions[SyncRand(potential_positions.size())];
		potential_positions.erase(std::remove(potential_positions.begin(), potential_positions.end(), random_pos), potential_positions.end());
		
		if (!this->Info.IsPointOnMap(random_pos, z) || this->is_point_in_a_subtemplate_area(random_pos, z)) {
			continue;
		}
		
		CTerrainType *tile_terrain = this->GetTileTerrain(random_pos, false, z);
		
		if (!generated_terrain->CanGenerateOnTile(this->Field(random_pos, z))) {
			continue;
		}
		
		if (
			(
				(
					!terrain_type->Overlay
					&& ((tile_terrain == terrain_type && GetTileTopTerrain(random_pos, false, z)->Overlay) || (std::find(terrain_type->BorderTerrains.begin(), terrain_type->BorderTerrains.end(), tile_terrain) != terrain_type->BorderTerrains.end() && this->TileBordersOnlySameTerrain(random_pos, terrain_type, z)))
				)
				|| (
					terrain_type->Overlay
					&& std::find(terrain_type->BaseTerrainTypes.begin(), terrain_type->BaseTerrainTypes.end(), tile_terrain) != terrain_type->BaseTerrainTypes.end() && this->TileBordersOnlySameTerrain(random_pos, terrain_type, z)
					&& (!GetTileTopTerrain(random_pos, false, z)->Overlay || GetTileTopTerrain(random_pos, false, z) == terrain_type)
				)
			)
			&& (!preserve_coastline || (terrain_type->Flags & MapFieldWaterAllowed) == (tile_terrain->Flags & MapFieldWaterAllowed))
			&& !this->TileHasUnitsIncompatibleWithTerrain(random_pos, terrain_type, z)
			&& (!(terrain_type->Flags & MapFieldUnpassable) || !this->TileBordersUnit(random_pos, z)) // if the terrain is unpassable, don't expand to spots adjacent to units
		) {
			std::vector<Vec2i> adjacent_positions;
			for (int sub_x = -1; sub_x <= 1; sub_x += 2) { // +2 so that only diagonals are used
				for (int sub_y = -1; sub_y <= 1; sub_y += 2) {
					Vec2i diagonal_pos(random_pos.x + sub_x, random_pos.y + sub_y);
					Vec2i vertical_pos(random_pos.x, random_pos.y + sub_y);
					Vec2i horizontal_pos(random_pos.x + sub_x, random_pos.y);
					if (!this->Info.IsPointOnMap(diagonal_pos, z)) {
						continue;
					}
					
					CTerrainType *diagonal_tile_terrain = this->GetTileTerrain(diagonal_pos, false, z);
					CTerrainType *vertical_tile_terrain = this->GetTileTerrain(vertical_pos, false, z);
					CTerrainType *horizontal_tile_terrain = this->GetTileTerrain(horizontal_pos, false, z);
					
					if (
						!generated_terrain->CanGenerateOnTile(this->Field(diagonal_pos, z))
						|| !generated_terrain->CanGenerateOnTile(this->Field(vertical_pos, z))
						|| !generated_terrain->CanGenerateOnTile(this->Field(horizontal_pos, z))
					) {
						continue;
					}
		
					if (
						(
							(
								!terrain_type->Overlay
								&& ((diagonal_tile_terrain == terrain_type && GetTileTopTerrain(diagonal_pos, false, z)->Overlay) || (std::find(terrain_type->BorderTerrains.begin(), terrain_type->BorderTerrains.end(), diagonal_tile_terrain) != terrain_type->BorderTerrains.end() && this->TileBordersOnlySameTerrain(diagonal_pos, terrain_type, z)))
								&& ((vertical_tile_terrain == terrain_type && GetTileTopTerrain(vertical_pos, false, z)->Overlay) || (std::find(terrain_type->BorderTerrains.begin(), terrain_type->BorderTerrains.end(), vertical_tile_terrain) != terrain_type->BorderTerrains.end() && this->TileBordersOnlySameTerrain(vertical_pos, terrain_type, z)))
								&& ((horizontal_tile_terrain == terrain_type && GetTileTopTerrain(horizontal_pos, false, z)->Overlay) || (std::find(terrain_type->BorderTerrains.begin(), terrain_type->BorderTerrains.end(), horizontal_tile_terrain) != terrain_type->BorderTerrains.end() && this->TileBordersOnlySameTerrain(horizontal_pos, terrain_type, z)))
							)
							|| (
								terrain_type->Overlay
								&& std::find(terrain_type->BaseTerrainTypes.begin(), terrain_type->BaseTerrainTypes.end(), diagonal_tile_terrain) != terrain_type->BaseTerrainTypes.end() && this->TileBordersOnlySameTerrain(diagonal_pos, terrain_type, z)
								&& std::find(terrain_type->BaseTerrainTypes.begin(), terrain_type->BaseTerrainTypes.end(), vertical_tile_terrain) != terrain_type->BaseTerrainTypes.end() && this->TileBordersOnlySameTerrain(vertical_pos, terrain_type, z)
								&& std::find(terrain_type->BaseTerrainTypes.begin(), terrain_type->BaseTerrainTypes.end(), horizontal_tile_terrain) != terrain_type->BaseTerrainTypes.end() && this->TileBordersOnlySameTerrain(horizontal_pos, terrain_type, z)
								&& (!GetTileTopTerrain(diagonal_pos, false, z)->Overlay || GetTileTopTerrain(diagonal_pos, false, z) == terrain_type) && (!GetTileTopTerrain(vertical_pos, false, z)->Overlay || GetTileTopTerrain(vertical_pos, false, z) == terrain_type) && (!GetTileTopTerrain(horizontal_pos, false, z)->Overlay || GetTileTopTerrain(horizontal_pos, false, z) == terrain_type)
							)
						)
						&& (!preserve_coastline || ((terrain_type->Flags & MapFieldWaterAllowed) == (diagonal_tile_terrain->Flags & MapFieldWaterAllowed) && (terrain_type->Flags & MapFieldWaterAllowed) == (vertical_tile_terrain->Flags & MapFieldWaterAllowed) && (terrain_type->Flags & MapFieldWaterAllowed) == (horizontal_tile_terrain->Flags & MapFieldWaterAllowed)))
						&& !this->TileHasUnitsIncompatibleWithTerrain(diagonal_pos, terrain_type, z) && !this->TileHasUnitsIncompatibleWithTerrain(vertical_pos, terrain_type, z) && !this->TileHasUnitsIncompatibleWithTerrain(horizontal_pos, terrain_type, z)
						&& (!(terrain_type->Flags & MapFieldUnpassable) || (!this->TileBordersUnit(diagonal_pos, z) && !this->TileBordersUnit(vertical_pos, z) && !this->TileBordersUnit(horizontal_pos, z))) // if the terrain is unpassable, don't expand to spots adjacent to buildings
						&& !this->is_point_in_a_subtemplate_area(diagonal_pos, z) && !this->is_point_in_a_subtemplate_area(vertical_pos, z) && !this->is_point_in_a_subtemplate_area(horizontal_pos, z)
					) {
						adjacent_positions.push_back(diagonal_pos);
					}
				}
			}
			
			if (adjacent_positions.size() > 0) {
				Vec2i adjacent_pos = adjacent_positions[SyncRand(adjacent_positions.size())];
				if (!terrain_type->Overlay) {
					this->Field(random_pos, z)->RemoveOverlayTerrain();
					this->Field(adjacent_pos, z)->RemoveOverlayTerrain();
					this->Field(Vec2i(random_pos.x, adjacent_pos.y), z)->RemoveOverlayTerrain();
					this->Field(Vec2i(adjacent_pos.x, random_pos.y), z)->RemoveOverlayTerrain();
				}
				this->Field(random_pos, z)->SetTerrain(terrain_type);
				this->Field(adjacent_pos, z)->SetTerrain(terrain_type);
				this->Field(Vec2i(random_pos.x, adjacent_pos.y), z)->SetTerrain(terrain_type);
				this->Field(Vec2i(adjacent_pos.x, random_pos.y), z)->SetTerrain(terrain_type);
				count -= 1;
				seeds.push_back(random_pos);
				seeds.push_back(adjacent_pos);
				seeds.push_back(Vec2i(random_pos.x, adjacent_pos.y));
				seeds.push_back(Vec2i(adjacent_pos.x, random_pos.y));
				
				tile_quantity += 4;
			}
		}
	}
	
	//expand seeds
	for (size_t i = 0; i < seeds.size(); ++i) {
		Vec2i seed_pos = seeds[i];
		
		if (max_tile_quantity != 0 && tile_quantity >= max_tile_quantity) {
			break;
		}
		
		const int random_number = SyncRand(100);
		if (random_number >= generated_terrain->ExpansionChance) {
			continue;
		}
		
		std::vector<Vec2i> adjacent_positions;
		for (int sub_x = -1; sub_x <= 1; sub_x += 2) { // +2 so that only diagonals are used
			for (int sub_y = -1; sub_y <= 1; sub_y += 2) {
				Vec2i diagonal_pos(seed_pos.x + sub_x, seed_pos.y + sub_y);
				Vec2i vertical_pos(seed_pos.x, seed_pos.y + sub_y);
				Vec2i horizontal_pos(seed_pos.x + sub_x, seed_pos.y);
				if (!this->Info.IsPointOnMap(diagonal_pos, z)) {
					continue;
				}
				
				if ( //must either be able to generate on the tiles, or they must already have the generated terrain type
					!generated_terrain->CanTileBePartOfExpansion(this->Field(diagonal_pos, z))
					|| !generated_terrain->CanTileBePartOfExpansion(this->Field(vertical_pos, z))
					|| !generated_terrain->CanTileBePartOfExpansion(this->Field(horizontal_pos, z))
				) {
					continue;
				}
		
				CTerrainType *diagonal_tile_terrain = this->GetTileTerrain(diagonal_pos, false, z);
				CTerrainType *vertical_tile_terrain = this->GetTileTerrain(vertical_pos, false, z);
				CTerrainType *horizontal_tile_terrain = this->GetTileTerrain(horizontal_pos, false, z);
				CTerrainType *diagonal_tile_top_terrain = this->GetTileTopTerrain(diagonal_pos, false, z);
				CTerrainType *vertical_tile_top_terrain = this->GetTileTopTerrain(vertical_pos, false, z);
				CTerrainType *horizontal_tile_top_terrain = this->GetTileTopTerrain(horizontal_pos, false, z);
				
				if (!terrain_type->Overlay) {
					if (diagonal_tile_terrain != terrain_type && (std::find(terrain_type->BorderTerrains.begin(), terrain_type->BorderTerrains.end(), diagonal_tile_terrain) == terrain_type->BorderTerrains.end() || this->TileBordersTerrainIncompatibleWithTerrain(diagonal_pos, terrain_type, z))) {
						continue;
					}
					if (vertical_tile_terrain != terrain_type && (std::find(terrain_type->BorderTerrains.begin(), terrain_type->BorderTerrains.end(), vertical_tile_terrain) == terrain_type->BorderTerrains.end() || this->TileBordersTerrainIncompatibleWithTerrain(vertical_pos, terrain_type, z))) {
						continue;
					}
					if (horizontal_tile_terrain != terrain_type && (std::find(terrain_type->BorderTerrains.begin(), terrain_type->BorderTerrains.end(), horizontal_tile_terrain) == terrain_type->BorderTerrains.end() || this->TileBordersTerrainIncompatibleWithTerrain(horizontal_pos, terrain_type, z))) {
						continue;
					}
				} else {
					if ((std::find(terrain_type->BaseTerrainTypes.begin(), terrain_type->BaseTerrainTypes.end(), diagonal_tile_terrain) == terrain_type->BaseTerrainTypes.end() || this->TileBordersTerrainIncompatibleWithTerrain(diagonal_pos, terrain_type, z)) && GetTileTerrain(diagonal_pos, terrain_type->Overlay, z) != terrain_type) {
						continue;
					}
					if ((std::find(terrain_type->BaseTerrainTypes.begin(), terrain_type->BaseTerrainTypes.end(), vertical_tile_terrain) == terrain_type->BaseTerrainTypes.end() || this->TileBordersTerrainIncompatibleWithTerrain(vertical_pos, terrain_type, z)) && GetTileTerrain(vertical_pos, terrain_type->Overlay, z) != terrain_type) {
						continue;
					}
					if ((std::find(terrain_type->BaseTerrainTypes.begin(), terrain_type->BaseTerrainTypes.end(), horizontal_tile_terrain) == terrain_type->BaseTerrainTypes.end() || this->TileBordersTerrainIncompatibleWithTerrain(horizontal_pos, terrain_type, z)) && GetTileTerrain(horizontal_pos, terrain_type->Overlay, z) != terrain_type) {
						continue;
					}
				}
				
				if (diagonal_tile_top_terrain == terrain_type && vertical_tile_top_terrain == terrain_type && horizontal_tile_top_terrain == terrain_type) { //at least one of the tiles being expanded to must be different from the terrain type
					continue;
				}
				
				//tiles within a subtemplate area can only be used as seeds, they cannot be modified themselves
				if (
					(this->is_point_in_a_subtemplate_area(diagonal_pos, z) && !generated_terrain->CanUseTileAsSeed(this->Field(diagonal_pos, z)))
					|| (this->is_point_in_a_subtemplate_area(vertical_pos, z) && !generated_terrain->CanUseTileAsSeed(this->Field(vertical_pos, z)))
					|| (this->is_point_in_a_subtemplate_area(horizontal_pos, z) && !generated_terrain->CanUseTileAsSeed(this->Field(horizontal_pos, z)))
				) {
					continue;
				}
				
				if (
					preserve_coastline
					&& (
						(terrain_type->Flags & MapFieldWaterAllowed) != (diagonal_tile_terrain->Flags & MapFieldWaterAllowed)
						|| (terrain_type->Flags & MapFieldWaterAllowed) != (vertical_tile_terrain->Flags & MapFieldWaterAllowed)
						|| (terrain_type->Flags & MapFieldWaterAllowed) != (horizontal_tile_terrain->Flags & MapFieldWaterAllowed)
					)
				) {
					continue;
				}
				
				if (this->TileHasUnitsIncompatibleWithTerrain(diagonal_pos, terrain_type, z) || this->TileHasUnitsIncompatibleWithTerrain(vertical_pos, terrain_type, z) || this->TileHasUnitsIncompatibleWithTerrain(horizontal_pos, terrain_type, z)) {
					continue;
				}
				
				if ( // if the terrain is unpassable, don't expand to spots adjacent to buildings
					(terrain_type->Flags & MapFieldUnpassable) && (this->TileBordersUnit(diagonal_pos, z) || this->TileBordersUnit(vertical_pos, z) || this->TileBordersUnit(horizontal_pos, z))
				) {
					continue;
				}
				
				//tiles with no terrain could nevertheless have units that were placed there already, e.g. due to units in a subtemplate being placed in a location where something is already present (e.g. units with a settlement set as their location, or resource units generated near the player's starting location); as such, we need to check if the terrain is compatible with those units
				if (this->TileHasUnitsIncompatibleWithTerrain(diagonal_pos, terrain_type, z) || this->TileHasUnitsIncompatibleWithTerrain(vertical_pos, terrain_type, z) || this->TileHasUnitsIncompatibleWithTerrain(horizontal_pos, terrain_type, z)) {
					continue;
				}

				adjacent_positions.push_back(diagonal_pos);
			}
		}
		
		if (adjacent_positions.size() > 0) {
			Vec2i adjacent_pos = adjacent_positions[SyncRand(adjacent_positions.size())];
			Vec2i adjacent_pos_horizontal(adjacent_pos.x, seed_pos.y);
			Vec2i adjacent_pos_vertical(seed_pos.x, adjacent_pos.y);
			
			if (!this->is_point_in_a_subtemplate_area(adjacent_pos, z) && this->GetTileTopTerrain(adjacent_pos, false, z) != terrain_type && (this->GetTileTerrain(adjacent_pos, terrain_type->Overlay, z) != terrain_type || generated_terrain->CanRemoveTileOverlayTerrain(this->Field(adjacent_pos, z)))) {
				if (!terrain_type->Overlay && generated_terrain->CanRemoveTileOverlayTerrain(this->Field(adjacent_pos, z))) {
					this->Field(adjacent_pos, z)->RemoveOverlayTerrain();
				}

				if (this->GetTileTerrain(adjacent_pos, terrain_type->Overlay, z) != terrain_type) {
					this->Field(adjacent_pos, z)->SetTerrain(terrain_type);
				}
				
				seeds.push_back(adjacent_pos);
				
				if (this->GetTileTopTerrain(adjacent_pos, false, z) == terrain_type) {
					tile_quantity++;
				}
			}
			
			if (!this->is_point_in_a_subtemplate_area(adjacent_pos_horizontal, z) && this->GetTileTopTerrain(adjacent_pos_horizontal, false, z) != terrain_type && (this->GetTileTerrain(adjacent_pos_horizontal, terrain_type->Overlay, z) != terrain_type || generated_terrain->CanRemoveTileOverlayTerrain(this->Field(adjacent_pos_horizontal, z)))) {
				if (!terrain_type->Overlay && generated_terrain->CanRemoveTileOverlayTerrain(this->Field(adjacent_pos_horizontal, z))) {
					this->Field(adjacent_pos_horizontal, z)->RemoveOverlayTerrain();
				}
				
				if (this->GetTileTerrain(adjacent_pos_horizontal, terrain_type->Overlay, z) != terrain_type) {
					this->Field(adjacent_pos_horizontal, z)->SetTerrain(terrain_type);
				}
				
				seeds.push_back(adjacent_pos_horizontal);
				
				if (this->GetTileTopTerrain(adjacent_pos_horizontal, false, z) == terrain_type) {
					tile_quantity++;
				}
			}
			
			if (!this->is_point_in_a_subtemplate_area(adjacent_pos_vertical, z) && this->GetTileTopTerrain(adjacent_pos_vertical, false, z) != terrain_type && (this->GetTileTerrain(adjacent_pos_vertical, terrain_type->Overlay, z) != terrain_type || generated_terrain->CanRemoveTileOverlayTerrain(this->Field(adjacent_pos_vertical, z)))) {
				if (!terrain_type->Overlay && generated_terrain->CanRemoveTileOverlayTerrain(this->Field(adjacent_pos_vertical, z))) {
					this->Field(adjacent_pos_vertical, z)->RemoveOverlayTerrain();
				}
				
				if (this->GetTileTerrain(adjacent_pos_vertical, terrain_type->Overlay, z) != terrain_type) {
					this->Field(adjacent_pos_vertical, z)->SetTerrain(terrain_type);
				}
				
				seeds.push_back(adjacent_pos_vertical);
				
				if (this->GetTileTopTerrain(adjacent_pos_vertical, false, z) == terrain_type) {
					tile_quantity++;
				}
			}
		}
	}
}

bool CMap::CanTileBePartOfMissingTerrainGeneration(const CMapField *tile, const CTerrainType *terrain_type, const CTerrainType *overlay_terrain_type) const
{
	if (tile->GetTopTerrain() == nullptr) {
		return true;
	}

	if (tile->Terrain == terrain_type && (tile->OverlayTerrain == overlay_terrain_type || overlay_terrain_type == nullptr)) {
		return true;
	}

	return false;
}

void CMap::GenerateMissingTerrain(const Vec2i &min_pos, const Vec2i &max_pos, const int z)
{
	if (SaveGameLoading) {
		return;
	}

	Vec2i random_pos(0, 0);

	std::vector<Vec2i> seeds;

	//use tiles that have a terrain as seeds for the terrain generation
	bool has_tile_with_missing_terrain = false;
	for (int x = min_pos.x; x <= max_pos.x; ++x) {
		for (int y = min_pos.y; y <= max_pos.y; ++y) {
			const Vec2i tile_pos(x, y);

			if (!this->is_point_adjacent_to_non_subtemplate_area(tile_pos, z)) {
				continue;
			}

			const CMapField *tile = this->Field(x, y, z);

			if (tile->GetTopTerrain() == nullptr) {
				has_tile_with_missing_terrain = true;
				continue;
			}

			if (!this->TileBordersTerrain(tile_pos, nullptr, z)) {
				continue; //the seed must border a tile with null terrain
			}

			seeds.push_back(tile_pos);
		}
	}

	if (!has_tile_with_missing_terrain) {
		return;
	}

	//expand seeds
	while (!seeds.empty()) {
		Vec2i seed_pos = seeds[SyncRand(seeds.size())];
		stratagus::vector::remove(seeds, seed_pos);

		CTerrainType *terrain_type = this->Field(seed_pos, z)->Terrain;
		CTerrainType *overlay_terrain_type = this->Field(seed_pos, z)->OverlayTerrain;

		if (overlay_terrain_type != nullptr) {
			if (
				(overlay_terrain_type->Flags & MapFieldWall)
				|| (overlay_terrain_type->Flags & MapFieldRoad)
				|| (overlay_terrain_type->Flags & MapFieldRailroad)
			) {
				overlay_terrain_type = nullptr; //don't expand overlay terrain to tiles with empty terrain if the overlay is a wall or pathway
			}
		}

		std::vector<Vec2i> adjacent_positions;
		for (int sub_x = -1; sub_x <= 1; sub_x += 2) { // +2 so that only diagonals are used
			for (int sub_y = -1; sub_y <= 1; sub_y += 2) {
				Vec2i diagonal_pos(seed_pos.x + sub_x, seed_pos.y + sub_y);
				Vec2i vertical_pos(seed_pos.x, seed_pos.y + sub_y);
				Vec2i horizontal_pos(seed_pos.x + sub_x, seed_pos.y);
				if (!this->Info.IsPointOnMap(diagonal_pos, z)) {
					continue;
				}

				if ( //must either be able to generate on the tiles, or they must already have the generated terrain type
					!this->CanTileBePartOfMissingTerrainGeneration(this->Field(diagonal_pos, z), terrain_type, overlay_terrain_type)
					|| !this->CanTileBePartOfMissingTerrainGeneration(this->Field(vertical_pos, z), terrain_type, overlay_terrain_type)
					|| !this->CanTileBePartOfMissingTerrainGeneration(this->Field(horizontal_pos, z), terrain_type, overlay_terrain_type)
					) {
					continue;
				}

				CTerrainType *diagonal_tile_top_terrain = this->GetTileTopTerrain(diagonal_pos, false, z);
				CTerrainType *vertical_tile_top_terrain = this->GetTileTopTerrain(vertical_pos, false, z);
				CTerrainType *horizontal_tile_top_terrain = this->GetTileTopTerrain(horizontal_pos, false, z);

				if (diagonal_tile_top_terrain == nullptr && this->TileBordersTerrainIncompatibleWithTerrainPair(diagonal_pos, terrain_type, overlay_terrain_type, z)) {
					continue;
				}
				if (vertical_tile_top_terrain == nullptr && this->TileBordersTerrainIncompatibleWithTerrainPair(vertical_pos, terrain_type, overlay_terrain_type, z)) {
					continue;
				}
				if (horizontal_tile_top_terrain == nullptr && this->TileBordersTerrainIncompatibleWithTerrainPair(horizontal_pos, terrain_type, overlay_terrain_type, z)) {
					continue;
				}

				if (diagonal_tile_top_terrain != nullptr && vertical_tile_top_terrain != nullptr && horizontal_tile_top_terrain != nullptr) { //at least one of the tiles being expanded to must have null terrain
					continue;
				}

				//tiles within a subtemplate area can only be used as seeds, they cannot be modified themselves
				if (
					(this->is_point_in_a_subtemplate_area(diagonal_pos, z) && diagonal_tile_top_terrain == nullptr)
					|| (this->is_point_in_a_subtemplate_area(vertical_pos, z) && vertical_tile_top_terrain == nullptr)
					|| (this->is_point_in_a_subtemplate_area(horizontal_pos, z) && horizontal_tile_top_terrain == nullptr)
				) {
					continue;
				}

				if (overlay_terrain_type != nullptr) {
					if (this->TileHasUnitsIncompatibleWithTerrain(diagonal_pos, overlay_terrain_type, z) || this->TileHasUnitsIncompatibleWithTerrain(vertical_pos, overlay_terrain_type, z) || this->TileHasUnitsIncompatibleWithTerrain(horizontal_pos, overlay_terrain_type, z)) {
						continue;
					}
				}

				adjacent_positions.push_back(diagonal_pos);
			}
		}

		if (adjacent_positions.size() > 0) {
			if (adjacent_positions.size() > 1) {
				seeds.push_back(seed_pos); //push the seed back again for another try, since it may be able to generate further terrain in the future
			}

			Vec2i adjacent_pos = adjacent_positions[SyncRand(adjacent_positions.size())];
			Vec2i adjacent_pos_horizontal(adjacent_pos.x, seed_pos.y);
			Vec2i adjacent_pos_vertical(seed_pos.x, adjacent_pos.y);

			if (this->GetTileTopTerrain(adjacent_pos, false, z) == nullptr) {
				this->Field(adjacent_pos, z)->SetTerrain(terrain_type);
				this->Field(adjacent_pos, z)->SetTerrain(overlay_terrain_type);
				seeds.push_back(adjacent_pos);
			}

			if (this->GetTileTopTerrain(adjacent_pos_horizontal, false, z) == nullptr) {
				this->Field(adjacent_pos_horizontal, z)->SetTerrain(terrain_type);
				this->Field(adjacent_pos_horizontal, z)->SetTerrain(overlay_terrain_type);
				seeds.push_back(adjacent_pos_horizontal);
			}

			if (this->GetTileTopTerrain(adjacent_pos_vertical, false, z) == nullptr) {
				this->Field(adjacent_pos_vertical, z)->SetTerrain(terrain_type);
				this->Field(adjacent_pos_vertical, z)->SetTerrain(overlay_terrain_type);
				seeds.push_back(adjacent_pos_vertical);
			}
		}
	}

	//set the terrain of the remaining tiles without any to their most-neighbored terrain/overlay terrain pair
	for (int x = min_pos.x; x <= max_pos.x; ++x) {
		for (int y = min_pos.y; y <= max_pos.y; ++y) {
			const Vec2i tile_pos(x, y);

			if (this->is_point_in_a_subtemplate_area(tile_pos, z)) {
				continue;
			}

			CMapField *tile = this->Field(x, y, z);

			if (tile->GetTopTerrain() != nullptr) {
				continue;
			}

			std::map<std::pair<CTerrainType *, CTerrainType *>, int> terrain_type_pair_neighbor_count;

			for (int x_offset = -1; x_offset <= 1; ++x_offset) {
				for (int y_offset = -1; y_offset <= 1; ++y_offset) {
					if (x_offset == 0 && y_offset == 0) {
						continue;
					}

					Vec2i adjacent_pos(tile_pos.x + x_offset, tile_pos.y + y_offset);

					if (!this->Info.IsPointOnMap(adjacent_pos, z)) {
						continue;
					}

					const CMapField *adjacent_tile = this->Field(adjacent_pos, z);
					CTerrainType *adjacent_terrain_type = adjacent_tile->GetTerrain(false);
					CTerrainType *adjacent_overlay_terrain_type = adjacent_tile->GetTerrain(true);

					if (adjacent_terrain_type == nullptr) {
						continue;
					}

					std::pair<CTerrainType *, CTerrainType *> terrain_type_pair(adjacent_terrain_type, adjacent_overlay_terrain_type);

					auto find_iterator = terrain_type_pair_neighbor_count.find(terrain_type_pair);
					if (find_iterator == terrain_type_pair_neighbor_count.end()) {
						terrain_type_pair_neighbor_count[terrain_type_pair] = 1;
					} else {
						find_iterator->second++;
					}
				}
			}

			std::pair<CTerrainType *, CTerrainType *> best_terrain_type_pair(nullptr, nullptr);
			int best_terrain_type_neighbor_count = 0;
			for (const auto &element : terrain_type_pair_neighbor_count) {
				if (element.second > best_terrain_type_neighbor_count) {
					best_terrain_type_pair = element.first;
					best_terrain_type_neighbor_count = element.second;
				}
			}

			//set the terrain and overlay terrain to the same as the most-neighbored one
			tile->SetTerrain(best_terrain_type_pair.first);
			tile->SetTerrain(best_terrain_type_pair.second);
		}
	}
}

void CMap::GenerateNeutralUnits(CUnitType *unit_type, int quantity, const Vec2i &min_pos, const Vec2i &max_pos, bool grouped, int z)
{
	if (SaveGameLoading) {
		return;
	}
	
	Vec2i unit_pos(-1, -1);
	
	for (int i = 0; i < quantity; ++i) {
		if (i == 0 || !grouped) {
			unit_pos = GenerateUnitLocation(unit_type, nullptr, min_pos, max_pos, z);
		}
		if (!this->Info.IsPointOnMap(unit_pos, z)) {
			continue;
		}
		if (unit_type->GivesResource) {
			CUnit *unit = CreateResourceUnit(unit_pos, *unit_type, z);
		} else {
			CUnit *unit = CreateUnit(unit_pos, *unit_type, CPlayer::Players[PlayerNumNeutral], z, unit_type->BoolFlag[BUILDING_INDEX].value && unit_type->TileSize.x > 1 && unit_type->TileSize.y > 1);
		}
	}
}
//Wyrmgus end

//Wyrmgus start
void CMap::ClearOverlayTile(const Vec2i &pos, int z)
{
	CMapField &mf = *this->Field(pos, z);

	if (!mf.OverlayTerrain) {
		return;
	}
	
	this->SetOverlayTerrainDestroyed(pos, true, z);

	//remove decorations if a wall, tree or rock was removed from the tile
	std::vector<CUnit *> table;
	Select(pos, pos, table, z);
	for (size_t i = 0; i != table.size(); ++i) {
		if (table[i]->Type->UnitType == UnitTypeType::Land && table[i]->Type->BoolFlag[DECORATION_INDEX].value) {
			if (Editor.Running == EditorNotRunning) {
				LetUnitDie(*table[i]);			
			} else {
				EditorActionRemoveUnit(*table[i], false);
			}
		}
	}

	//check if any further tile should be removed with the clearing of this one
	if (!mf.OverlayTerrain->AllowSingle) {
		for (int x_offset = -1; x_offset <= 1; ++x_offset) {
			for (int y_offset = -1; y_offset <= 1; ++y_offset) {
				if (x_offset != 0 || y_offset != 0) {
					Vec2i adjacent_pos(pos.x + x_offset, pos.y + y_offset);
					if (Map.Info.IsPointOnMap(adjacent_pos, z)) {
						CMapField &adjacent_mf = *this->Field(adjacent_pos, z);
						
						if (adjacent_mf.OverlayTerrain == mf.OverlayTerrain && !adjacent_mf.OverlayTerrainDestroyed && !this->CurrentTerrainCanBeAt(adjacent_pos, true, z)) {
							this->ClearOverlayTile(adjacent_pos, z);
						}
					}
				}
			}
		}
	}
}
//Wyrmgus end

//Wyrmgus start
/*
/// Remove wood from the map.
void CMap::ClearWoodTile(const Vec2i &pos)
{
	CMapField &mf = *this->Field(pos);

	mf.setGraphicTile(this->Tileset->getRemovedTreeTile());
	mf.Flags &= ~(MapFieldForest | MapFieldUnpassable);
	mf.Value = 0;

	UI.Minimap.UpdateXY(pos);
	FixNeighbors(MapFieldForest, 0, pos);

	//maybe isExplored
	if (mf.playerInfo.IsExplored(*ThisPlayer)) {
		UI.Minimap.UpdateSeenXY(pos);
		MarkSeenTile(mf);
	}
}

/// Remove rock from the map.
void CMap::ClearRockTile(const Vec2i &pos)
{
	CMapField &mf = *this->Field(pos);

	mf.setGraphicTile(this->Tileset->getRemovedRockTile());
	mf.Flags &= ~(MapFieldRocks | MapFieldUnpassable);
	mf.Value = 0;
	
	UI.Minimap.UpdateXY(pos);
	FixNeighbors(MapFieldRocks, 0, pos);

	//maybe isExplored
	if (mf.playerInfo.IsExplored(*ThisPlayer)) {
		UI.Minimap.UpdateSeenXY(pos);
		MarkSeenTile(mf);
	}
}
*/
//Wyrmgus end

/**
**	@brief	Regenerate forest.
*/
void CMap::RegenerateForest()
{
	if (!ForestRegeneration) {
		return;
	}

	for (CMapLayer *map_layer : this->MapLayers) {
		map_layer->RegenerateForest();
	}
}


/**
**  Load the map presentation
**
**  @param mapname  map filename
*/
void LoadStratagusMapInfo(const std::string &mapname)
{
	// Set the default map setup by replacing .smp with .sms
	size_t loc = mapname.find(".smp");
	if (loc != std::string::npos) {
		CMap::Map.Info.Filename = mapname;
		CMap::Map.Info.Filename.replace(loc, 4, ".sms");
	}

	const std::string filename = LibraryFileName(mapname.c_str());
	LuaLoadFile(filename);
}
