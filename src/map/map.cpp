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
//      (c) Copyright 1998-2021 by Lutz Sammer, Vladi Shabanski,
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

#include "stratagus.h"

#include "map/map.h"

#include "ai/ai_local.h"
#include "database/defines.h"
#include "database/sml_parser.h"
//Wyrmgus start
#include "editor.h"
#include "game/game.h" // for the SaveGameLoading variable
//Wyrmgus end
#include "iolib.h"
#include "map/landmass.h"
#include "map/map_info.h"
#include "map/map_layer.h"
#include "map/map_template.h"
#include "map/minimap.h"
#include "map/site.h"
#include "map/site_container.h"
#include "map/site_game_data.h"
#include "map/terrain_feature.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "map/tile_flag.h"
#include "map/tileset.h"
#include "map/world.h"
#include "map/world_game_data.h"
#include "player/player.h"
#include "player/player_type.h"
//Wyrmgus start
#include "province.h"
//Wyrmgus end
#include "script.h"
//Wyrmgus start
#include "settings.h"
#include "sound/sound_server.h"
//Wyrmgus end
#include "species/species.h"
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
#include "util/container_util.h"
#include "util/point_util.h"
#include "util/rect_util.h"
#include "util/set_util.h"
#include "util/size_util.h"
#include "util/string_util.h"
#include "util/util.h"
#include "util/vector_random_util.h"
#include "util/vector_util.h"
#include "video/video.h"

int FlagRevealMap; //flag must reveal the map
int ReplayRevealMap; //reveal Map is replay
std::filesystem::path CurrentMapPath; //path of the current map

/*----------------------------------------------------------------------------
--  Visible and explored handling
----------------------------------------------------------------------------*/

/**
**  Marks seen tile -- used mainly for the Fog Of War
**
**  @param mf  MapField-position.
*/
void CMap::MarkSeenTile(wyrmgus::tile &mf)
{
	//Wyrmgus start
//	const unsigned int tile = mf.getGraphicTile();
//	const unsigned int seentile = mf.player_info->SeenTile;
	//Wyrmgus end

	//  Nothing changed? Seeing already the correct tile.
	//Wyrmgus start
//	if (tile == seentile) {
	if (mf.IsSeenTileCorrect()) {
	//Wyrmgus end
		return;
	}
	mf.UpdateSeenTile();

#ifdef MINIMAP_UPDATE
	//rb - GRRRRRRRRRRRR
	//Wyrmgus start
//	const unsigned int index = &mf - this->Fields;
//	const int y = index / Info.MapWidth;
//	const int x = index - (y * Info.MapWidth);
	const CMapLayer *map_layer = this->MapLayers[z];
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
		const unsigned int index = &mf - this->Fields;
		const int y = index / Info.MapWidth;
		const int x = index - (y * Info.MapWidth);
		const Vec2i pos(x, y);
#endif

		//  Handle wood changes. FIXME: check if for growing wood correct?
		if (tile == this->Tileset->getRemovedTreeTile()) {
			FixNeighbors(tile_flag::tree, 1, pos);
		} else if (seentile == this->Tileset->getRemovedTreeTile()) {
			FixTile(tile_flag::tree, 1, pos);
		} else if (mf.ForestOnMap()) {
			FixTile(tile_flag::tree, 1, pos);
			FixNeighbors(tile_flag::tree, 1, pos);

			// Handle rock changes.
		} else if (tile == Tileset->getRemovedRockTile()) {
			FixNeighbors(tile_flag::rock, 1, pos);
		} else if (seentile == Tileset->getRemovedRockTile()) {
			FixTile(tile_flag::rock, 1, pos);
		} else if (mf.RockOnMap()) {
			FixTile(tile_flag::rock, 1, pos);
			FixNeighbors(tile_flag::rock, 1, pos);

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
//	UI.get_minimap()->UpdateXY(pos);
	UI.get_minimap()->UpdateXY(pos, z);
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
	for (int i = 0; i != this->Info->MapWidth * this->Info->MapHeight; ++i) {
		wyrmgus::tile &mf = *this->Field(i);
		wyrmgus::tile_player_info &playerInfo = mf.playerInfo;
		for (int p = 0; p < PlayerMax; ++p) {
			//Wyrmgus start
//			playerInfo.Visible[p] = std::max<unsigned short>(1, playerInfo.Visible[p]);
			if (Players[p].Type == player_type::person || !only_person_players) {
				playerInfo.Visible[p] = std::max<unsigned short>(1, playerInfo.Visible[p]);
			}
			//Wyrmgus end
		}
		MarkSeenTile(mf);
	}
	*/
	for (size_t z = 0; z < this->MapLayers.size(); ++z) {
		for (int i = 0; i != this->Info->MapWidths[z] * this->Info->MapHeights[z]; ++i) {
			wyrmgus::tile &mf = *this->Field(i, z);
			const std::unique_ptr<wyrmgus::tile_player_info> &player_info = mf.player_info;
			for (int p = 0; p < PlayerMax; ++p) {
				if (CPlayer::Players[p]->get_type() == player_type::person || !only_person_players) {
					player_info->Visible[p] = std::max<unsigned short>(1, player_info->Visible[p]);
				}
			}
			MarkSeenTile(mf);
		}
	}
	//Wyrmgus end

	//  Global seen recount. Simple and effective.
	for (CUnit *unit : wyrmgus::unit_manager::get()->get_units()) {
		//  Reveal neutral buildings. Gold mines:)
		if (unit->Player->get_type() == player_type::neutral) {
			for (int p = 0; p < PlayerMax; ++p) {
				const CPlayer *player = CPlayer::Players[p].get();
				if (player->get_type() != player_type::nobody && (player->get_type() == player_type::person || !only_person_players) && !unit->is_seen_by_player(p)) {
					UnitGoesOutOfFog(*unit, *player);
					UnitGoesUnderFog(*unit, *player);
				}
			}
		}
		UnitCountSeen(*unit);
	}
}

/*----------------------------------------------------------------------------
--  Map queries
----------------------------------------------------------------------------*/

Vec2i CMap::map_pixel_pos_to_tile_pos(const PixelPos &mapPos) const
{
	const Vec2i tilePos(mapPos.x / wyrmgus::defines::get()->get_tile_width(), mapPos.y / wyrmgus::defines::get()->get_tile_height());

	return tilePos;
}

Vec2i CMap::scaled_map_pixel_pos_to_tile_pos(const PixelPos &mapPos) const
{
	return this->map_pixel_pos_to_tile_pos(mapPos / wyrmgus::defines::get()->get_scale_factor());
}

PixelPos CMap::tile_pos_to_map_pixel_pos_top_left(const Vec2i &tilePos) const
{
	PixelPos mapPixelPos(tilePos.x * wyrmgus::defines::get()->get_tile_width(), tilePos.y * wyrmgus::defines::get()->get_tile_height());

	return mapPixelPos;
}

PixelPos CMap::tile_pos_to_scaled_map_pixel_pos_top_left(const Vec2i &tilePos) const
{
	return this->tile_pos_to_map_pixel_pos_top_left(tilePos) * wyrmgus::defines::get()->get_scale_factor();
}

PixelPos CMap::tile_pos_to_map_pixel_pos_center(const Vec2i &tilePos) const
{
	return this->tile_pos_to_map_pixel_pos_top_left(tilePos) + wyrmgus::size::to_point(wyrmgus::defines::get()->get_tile_size()) / 2;
}

PixelPos CMap::tile_pos_to_scaled_map_pixel_pos_center(const Vec2i &tilePos) const
{
	return this->tile_pos_to_scaled_map_pixel_pos_top_left(tilePos) + wyrmgus::size::to_point(wyrmgus::defines::get()->get_scaled_tile_size()) / 2;
}

//Wyrmgus start
const wyrmgus::terrain_type *CMap::GetTileTerrain(const Vec2i &pos, const bool overlay, const int z) const
{
	if (!this->Info->IsPointOnMap(pos, z)) {
		return nullptr;
	}
	
	const wyrmgus::tile &mf = *this->Field(pos, z);
	
	return overlay ? mf.get_overlay_terrain() : mf.get_terrain();
}

const wyrmgus::terrain_type *CMap::GetTileTopTerrain(const Vec2i &pos, const bool seen, const int z, const bool ignore_destroyed) const
{
	if (!this->Info->IsPointOnMap(pos, z)) {
		return nullptr;
	}
	
	wyrmgus::tile &mf = *this->Field(pos, z);
	
	return mf.get_top_terrain(seen, ignore_destroyed);
}

const landmass *CMap::get_tile_landmass(const QPoint &pos, const int z) const
{
	if (!this->Info->IsPointOnMap(pos, z)) {
		return nullptr;
	}
	
	const wyrmgus::tile &mf = *this->Field(pos, z);
	
	return mf.get_landmass();
}

const CUnitCache &CMap::get_tile_unit_cache(const QPoint &pos, int z)
{
	return this->Field(pos, z)->UnitCache;
}

QPoint CMap::generate_unit_location(const wyrmgus::unit_type *unit_type, const wyrmgus::faction *faction, const QPoint &min_pos, const QPoint &max_pos, const int z) const
{
	if (SaveGameLoading) {
		return QPoint(-1, -1);
	}
	
	CPlayer *player = GetFactionPlayer(faction);
	
	QPoint random_pos(-1, -1);
	
	std::vector<const terrain_type *> allowed_terrains;
	if (unit_type->BoolFlag[FAUNA_INDEX].value && unit_type->get_species() != nullptr) { //if the unit is a fauna one, it has to start on terrain it is native to
		allowed_terrains = unit_type->get_species()->get_native_terrain_types();
	}
	
	for (const wyrmgus::unit_type *spawned_type : unit_type->SpawnUnits) {
		if (spawned_type->BoolFlag[FAUNA_INDEX].value && spawned_type->get_species()) {
			vector::merge(allowed_terrains, spawned_type->get_species()->get_native_terrain_types());
		}
	}

	const CUnitStats &stats = player != nullptr ? unit_type->Stats[player->get_index()] : unit_type->MapDefaultStat;

	std::vector<QPoint> potential_positions;

	const int max_x_offset = (max_pos.x() - (unit_type->get_tile_width() - 1)) - min_pos.x();
	const int max_y_offset = (max_pos.y() - (unit_type->get_tile_height() - 1)) - min_pos.y();
	potential_positions.reserve(max_x_offset * max_y_offset);

	for (int x_offset = 0; x_offset <= max_x_offset; ++x_offset) {
		for (int y_offset = 0; y_offset <= max_y_offset; ++y_offset) {
			potential_positions.emplace_back(min_pos.x() + x_offset, min_pos.y() + y_offset);
		}
	}
	
	while (!potential_positions.empty()) {
		random_pos = vector::take_random(potential_positions);
		
		if (!this->Info->IsPointOnMap(random_pos, z) || (this->is_point_in_a_subtemplate_area(random_pos, z) && GameCycle == 0)) {
			continue;
		}
		
		const wyrmgus::tile *tile = this->Field(random_pos, z);

		if (!allowed_terrains.empty() && !vector::contains(allowed_terrains, tile->get_top_terrain())) {
			//if the unit is a fauna one, it has to start on terrain it is native to
			continue;
		}

		//do not generate organic units on deserts if they would die from that
		if (tile->has_flag(tile_flag::desert) && unit_type->BoolFlag[ORGANIC_INDEX].value && stats.Variables[DEHYDRATIONIMMUNITY_INDEX].Value <= 0) {
			continue;
		}
		
		std::vector<CUnit *> table;
		if (player != nullptr) {
			Select(random_pos - QPoint(32, 32), random_pos + QPoint(unit_type->get_tile_width() - 1, unit_type->get_tile_height() - 1) + QPoint(32, 32), table, z, MakeAndPredicate(HasNotSamePlayerAs(*player), HasNotSamePlayerAs(*CPlayer::get_neutral_player())));
		} else if (unit_type->get_given_resource() == nullptr) {
			if (unit_type->BoolFlag[NEUTRAL_HOSTILE_INDEX].value || unit_type->BoolFlag[PREDATOR_INDEX].value || (unit_type->BoolFlag[PEOPLEAVERSION_INDEX].value && (unit_type->UnitType == UnitTypeType::Fly || unit_type->UnitType == UnitTypeType::Space))) {
				Select(random_pos - QPoint(16, 16), random_pos + QPoint(unit_type->get_tile_width() - 1, unit_type->get_tile_height() - 1) + QPoint(16, 16), table, z, HasNotSamePlayerAs(*CPlayer::get_neutral_player()));
			} else {
				Select(random_pos - QPoint(8, 8), random_pos + QPoint(unit_type->get_tile_width() - 1, unit_type->get_tile_height() - 1) + QPoint(8, 8), table, z, HasNotSamePlayerAs(*CPlayer::get_neutral_player()));
			}
		} else if (unit_type->get_given_resource() != nullptr && !unit_type->BoolFlag[BUILDING_INDEX].value) {
			//for non-building resources (i.e. wood piles), place them within a certain distance of player units, to prevent them from blocking the way
			Select(random_pos - QPoint(4, 4), random_pos + QPoint(unit_type->get_tile_width() - 1, unit_type->get_tile_height() - 1) + QPoint(4, 4), table, z, HasNotSamePlayerAs(*CPlayer::get_neutral_player()));
		}
		
		if (!table.empty()) {
			continue;
		}

		//check if the unit won't be placed next to unpassable terrain
		bool passable_surroundings = true;
		for (int x = random_pos.x() - 1; x < random_pos.x() + unit_type->get_tile_width() + 1; ++x) {
			for (int y = random_pos.y() - 1; y < random_pos.y() + unit_type->get_tile_height() + 1; ++y) {
				if (this->Info->IsPointOnMap(x, y, z) && this->Field(x, y, z)->CheckMask(tile_flag::impassable)) {
					passable_surroundings = false;
					break;
				}
			}
			if (!passable_surroundings) {
				break;
			}
		}
		if (!passable_surroundings) {
			continue;
		}

		if (UnitTypeCanBeAt(*unit_type, random_pos, z) && (!unit_type->BoolFlag[BUILDING_INDEX].value || CanBuildUnitType(nullptr, *unit_type, random_pos, 0, true, z))) {
			return random_pos;
		}
	}
	
	return QPoint(-1, -1);
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
	Assert(this->Info->IsPointOnMap(pos, z));
	return Field(pos, z)->isAWall();
}

//Wyrmgus start
bool CMap::CurrentTerrainCanBeAt(const Vec2i &pos, const bool overlay, const int z) const
{
	wyrmgus::tile &mf = *this->Field(pos, z);
	const wyrmgus::terrain_type *terrain = nullptr;
	
	if (overlay) {
		terrain = mf.get_overlay_terrain();
	} else {
		terrain = mf.get_terrain();
	}
	
	if (!terrain) {
		return true;
	}
	
	if (terrain->allows_single()) {
		return true;
	}

	std::vector<int> transition_directions;
	
	for (int x_offset = -1; x_offset <= 1; ++x_offset) {
		for (int y_offset = -1; y_offset <= 1; ++y_offset) {
			if (x_offset != 0 || y_offset != 0) {
				Vec2i adjacent_pos(pos.x + x_offset, pos.y + y_offset);
				if (this->Info->IsPointOnMap(adjacent_pos, z)) {
					const wyrmgus::terrain_type *adjacent_terrain = this->GetTileTerrain(adjacent_pos, overlay, z);
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
//Wyrmgus end

bool CMap::is_tile_on_map_borders(const QPoint &tile_pos, const int z) const
{
	const CMapLayer *map_layer = this->MapLayers[z].get();
	return tile_pos.x() == 0 || tile_pos.y() == 0 || tile_pos.x() == (map_layer->get_width() - 1) || tile_pos.y() == (map_layer->get_height() - 1);
}

//Wyrmgus start
bool CMap::TileBordersTerrain(const Vec2i &pos, const wyrmgus::terrain_type *terrain_type, const int z) const
{
	bool overlay = terrain_type != nullptr ? terrain_type->is_overlay() : false;

	for (int sub_x = -1; sub_x <= 1; ++sub_x) {
		for (int sub_y = -1; sub_y <= 1; ++sub_y) {
			Vec2i adjacent_pos(pos.x + sub_x, pos.y + sub_y);
			if (!this->Info->IsPointOnMap(adjacent_pos, z) || (sub_x == 0 && sub_y == 0)) {
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
bool CMap::TileBordersOnlySameTerrain(const Vec2i &pos, const wyrmgus::terrain_type *new_terrain_type, const int z) const
{
	for (int sub_x = -1; sub_x <= 1; ++sub_x) {
		for (int sub_y = -1; sub_y <= 1; ++sub_y) {
			Vec2i adjacent_pos(pos.x + sub_x, pos.y + sub_y);
			if (!this->Info->IsPointOnMap(adjacent_pos, z) || (sub_x == 0 && sub_y == 0)) {
				continue;
			}
			if (this->is_point_in_a_subtemplate_area(pos, z) && !this->is_point_in_a_subtemplate_area(adjacent_pos, z)) {
				continue;
			}
			const wyrmgus::terrain_type *top_terrain = GetTileTopTerrain(pos, false, z);
			const wyrmgus::terrain_type *adjacent_top_terrain = GetTileTopTerrain(adjacent_pos, false, z);
			if (!new_terrain_type->is_overlay()) {
				if (
					adjacent_top_terrain
					&& adjacent_top_terrain != top_terrain
					&& (!wyrmgus::vector::contains(top_terrain->get_inner_border_terrain_types(), adjacent_top_terrain) || !wyrmgus::vector::contains(new_terrain_type->get_inner_border_terrain_types(), adjacent_top_terrain))
					&& adjacent_top_terrain != new_terrain_type
				) {
					return false;
				}
			} else {
				if (
					adjacent_top_terrain
					&& adjacent_top_terrain != top_terrain
					&& !wyrmgus::vector::contains(top_terrain->get_base_terrain_types(), adjacent_top_terrain) && !wyrmgus::vector::contains(adjacent_top_terrain->get_base_terrain_types(), top_terrain)
					&& adjacent_top_terrain != new_terrain_type
				) {
					return false;
				}
			}
		}
	}
		
	return true;
}

bool CMap::TileBordersFlag(const Vec2i &pos, const int z, const tile_flag flag, const bool reverse) const
{
	for (int sub_x = -1; sub_x <= 1; ++sub_x) {
		for (int sub_y = -1; sub_y <= 1; ++sub_y) {
			Vec2i adjacent_pos(pos.x + sub_x, pos.y + sub_y);
			if (!this->Info->IsPointOnMap(adjacent_pos, z) || (sub_x == 0 && sub_y == 0)) {
				continue;
			}
			const wyrmgus::tile &mf = *this->Field(adjacent_pos, z);
			
			if ((!reverse && mf.CheckMask(flag)) || (reverse && !mf.CheckMask(flag))) {
				return true;
			}
		}
	}
		
	return false;
}

bool CMap::tile_borders_other_terrain_feature(const QPoint &pos, const int z) const
{
	const wyrmgus::terrain_feature *tile_terrain_feature = this->Field(pos, z)->get_terrain_feature();

	std::optional<QPoint> result = wyrmgus::point::find_adjacent_if(pos, [&](const QPoint &adjacent_pos) {
		if (!this->Info->IsPointOnMap(adjacent_pos, z)) {
			return false;
		}

		const wyrmgus::terrain_feature *adjacent_terrain_feature = this->Field(adjacent_pos, z)->get_terrain_feature();
		return tile_terrain_feature != adjacent_terrain_feature;
	});

	return result.has_value();
}

bool CMap::tile_borders_same_settlement_territory(const QPoint &pos, const int z, const bool diagonal_allowed) const
{
	const wyrmgus::site *tile_settlement = this->Field(pos, z)->get_settlement();

	for (int sub_x = -1; sub_x <= 1; ++sub_x) {
		for (int sub_y = -1; sub_y <= 1; ++sub_y) {
			const QPoint adjacent_pos(pos.x() + sub_x, pos.y() + sub_y);
			if (!this->Info->IsPointOnMap(adjacent_pos, z) || (sub_x == 0 && sub_y == 0)) {
				continue;
			}

			if (!diagonal_allowed && sub_x != 0 && sub_y != 0) {
				continue;
			}

			const wyrmgus::site *adjacent_tile_settlement = this->Field(adjacent_pos, z)->get_settlement();
			if (tile_settlement == adjacent_tile_settlement) {
				return true;
			}
		}
	}

	return false;
}

bool CMap::tile_borders_other_settlement_territory(const QPoint &pos, const int z) const
{
	const wyrmgus::site *tile_settlement = this->Field(pos, z)->get_settlement();

	std::optional<QPoint> result = wyrmgus::point::find_adjacent_if(pos, [&](const QPoint &adjacent_pos) {
		if (!this->Info->IsPointOnMap(adjacent_pos, z)) {
			return false;
		}

		const wyrmgus::site *adjacent_tile_settlement = this->Field(adjacent_pos, z)->get_settlement();
		return tile_settlement != adjacent_tile_settlement;
	});

	return result.has_value();
}

bool CMap::tile_borders_other_player_territory(const QPoint &pos, const int z, const int range) const
{
	const CPlayer *tile_owner = this->Field(pos, z)->get_owner();

	for (int sub_x = -1; sub_x <= 1; ++sub_x) {
		for (int sub_y = -1; sub_y <= 1; ++sub_y) {
			const QPoint adjacent_pos(pos.x() + sub_x, pos.y() + sub_y);
			if (!this->Info->IsPointOnMap(adjacent_pos, z) || (sub_x == 0 && sub_y == 0)) {
				continue;
			}

			const CPlayer *adjacent_tile_owner = this->Field(adjacent_pos, z)->get_owner();
			if (tile_owner != adjacent_tile_owner) {
				return true;
			}

			if (range >= 1 && this->tile_borders_other_player_territory(adjacent_pos, z, range - 1)) {
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
			if (!this->Info->IsPointOnMap(adjacent_pos, z) || (sub_x == 0 && sub_y == 0)) {
				continue;
			}
			const wyrmgus::tile &mf = *this->Field(adjacent_pos, z);
			
			if (mf.CheckMask(tile_flag::building)) {
				return true;
			}
		}
	}
		
	return false;
}

bool CMap::tile_borders_pathway(const QPoint &pos, const int z, const bool only_railroad)
{
	for (int sub_x = -1; sub_x <= 1; ++sub_x) {
		for (int sub_y = -1; sub_y <= 1; ++sub_y) {
			const QPoint adjacent_pos(pos.x() + sub_x, pos.y() + sub_y);
			if (!this->Info->IsPointOnMap(adjacent_pos, z) || (sub_x == 0 && sub_y == 0)) {
				continue;
			}
			const wyrmgus::tile &mf = *this->Field(adjacent_pos, z);

			if (mf.get_overlay_terrain() == nullptr || !mf.get_overlay_terrain()->is_pathway()) {
				continue;
			}
			
			if (!only_railroad || mf.CheckMask(tile_flag::railroad)) {
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
			if (!this->Info->IsPointOnMap(adjacent_pos, z) || (sub_x == 0 && sub_y == 0)) {
				continue;
			}
			const wyrmgus::tile &mf = *this->Field(adjacent_pos, z);
			
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
bool CMap::TileBordersTerrainIncompatibleWithTerrain(const Vec2i &pos, const wyrmgus::terrain_type *terrain_type, const int z) const
{
	if (!terrain_type || !terrain_type->is_overlay()) {
		return false;
	}
	
	const wyrmgus::terrain_type *tile_terrain = this->GetTileTerrain(pos, false, z);
	
	for (int sub_x = -1; sub_x <= 1; ++sub_x) {
		for (int sub_y = -1; sub_y <= 1; ++sub_y) {
			Vec2i adjacent_pos(pos.x + sub_x, pos.y + sub_y);
			
			if (!this->Info->IsPointOnMap(adjacent_pos, z) || (sub_x == 0 && sub_y == 0)) {
				continue;
			}
			
			const wyrmgus::terrain_type *adjacent_terrain = this->GetTileTerrain(adjacent_pos, false, z);
			
			if (adjacent_terrain == nullptr) {
				continue;
			}

			if (tile_terrain == adjacent_terrain) {
				continue;
			}
			
			if (terrain_type->is_overlay()) {
				if ( //if the terrain type is an overlay one, the adjacent tile terrain is incompatible with it if it both cannot be a base terrain for the overlay terrain type, and it "expands into" the tile (that is, the tile has the adjacent terrain as an inner border terrain)
					vector::contains(tile_terrain->get_inner_border_terrain_types(), adjacent_terrain)
					&& !vector::contains(terrain_type->get_base_terrain_types(), adjacent_terrain)
				) {
					return true;
				}
			} else {
				//if the terrain type is not an overlay one, the adjacent tile terrain is incompatible with it if it cannot border the terrain type
				if (!terrain_type->is_border_terrain_type(adjacent_terrain)) {
					return true;
				}
			}
		}
	}
		
	return false;
}

bool CMap::TileBordersTerrainIncompatibleWithTerrainPair(const Vec2i &pos, const wyrmgus::terrain_type *terrain_type, const wyrmgus::terrain_type *overlay_terrain_type, const int z) const
{
	if (!terrain_type) {
		return false;
	}

	for (int sub_x = -1; sub_x <= 1; ++sub_x) {
		for (int sub_y = -1; sub_y <= 1; ++sub_y) {
			Vec2i adjacent_pos(pos.x + sub_x, pos.y + sub_y);

			if (!this->Info->IsPointOnMap(adjacent_pos, z) || (sub_x == 0 && sub_y == 0)) {
				continue;
			}

			const wyrmgus::terrain_type *adjacent_terrain = this->GetTileTerrain(adjacent_pos, false, z);

			if (adjacent_terrain == nullptr) {
				continue;
			}

			if (terrain_type == adjacent_terrain) {
				continue;
			}

			//the adjacent tile terrain is incompatible with the non-overlay terrain type if it cannot border the terrain type
			if (!terrain_type->is_border_terrain_type(adjacent_terrain)) {
				return true;
			}

			if (overlay_terrain_type != nullptr) {
				if ( //if the terrain type is an overlay one, the adjacent tile terrain is incompatible with it if it both cannot be a base terrain for the overlay terrain type, and it "expands into" the tile (that is, the tile has the adjacent terrain as an inner border terrain)
					wyrmgus::vector::contains(terrain_type->get_inner_border_terrain_types(), adjacent_terrain)
					&& !wyrmgus::vector::contains(overlay_terrain_type->get_base_terrain_types(), adjacent_terrain)
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
bool CMap::TileHasUnitsIncompatibleWithTerrain(const Vec2i &pos, const wyrmgus::terrain_type *terrain_type, const int z)
{
	const wyrmgus::tile &mf = *this->Field(pos, z);
	
	const CUnitCache &cache = mf.UnitCache;
	for (size_t i = 0; i != cache.size(); ++i) {
		const CUnit &unit = *cache[i];
		if (unit.IsAliveOnMap() && (terrain_type->Flags & unit.Type->MovementMask) != tile_flag::none) {
			return true;
		}
	}

	return false;
}

bool CMap::is_point_in_a_subtemplate_area(const QPoint &pos, const int z) const
{
	for (const auto &kv_pair : this->MapLayers[z]->subtemplate_areas) {
		const map_template *subtemplate = kv_pair.first;
		const QRect &subtemplate_rect = kv_pair.second;
		if (subtemplate_rect.contains(pos) && subtemplate->is_map_pos_usable(pos)) {
			return true;
		}
	}

	return false;
}

bool CMap::is_rect_in_a_subtemplate_area(const QRect &rect, const int z) const
{
	for (const auto &kv_pair : this->MapLayers[z]->subtemplate_areas) {
		const map_template *subtemplate = kv_pair.first;
		const QRect &subtemplate_rect = kv_pair.second;

		if (!rect.intersects(subtemplate_rect)) {
			continue;
		}

		for (int x = rect.x(); x <= rect.right(); ++x) {
			for (int y = rect.y(); y <= rect.bottom(); ++y) {
				const QPoint tile_pos(x, y);

				if (subtemplate->is_map_pos_usable(tile_pos)) {
					return true;
				}
			}
		}
	}

	return false;
}

bool CMap::is_point_in_subtemplate_area(const QPoint &pos, const int z, const wyrmgus::map_template *subtemplate) const
{
	const QRect &subtemplate_rect = this->MapLayers[z]->get_subtemplate_rect(subtemplate);

	if (!subtemplate_rect.isValid()) {
		return false;
	}

	return subtemplate_rect.contains(pos);
}

bool CMap::is_subtemplate_on_map(const wyrmgus::map_template *subtemplate) const
{
	const QPoint subtemplate_pos = this->get_subtemplate_pos(subtemplate);
	return subtemplate_pos.x() != -1 && subtemplate_pos.y() != -1;
}

const QRect &CMap::get_subtemplate_rect(const wyrmgus::map_template *subtemplate) const
{
	static QRect empty_rect;

	if (subtemplate == nullptr) {
		return empty_rect;
	}

	const wyrmgus::map_template *main_template = subtemplate->GetTopMapTemplate();
	if (main_template && subtemplate != main_template) {
		const int z = GetMapLayer(main_template->get_world() ? main_template->get_world()->get_identifier() : "");
		if (z != -1) {
			return this->MapLayers[z]->get_subtemplate_rect(subtemplate);
		}
	}

	return empty_rect;
}

QPoint CMap::get_subtemplate_pos(const wyrmgus::map_template *subtemplate) const
{
	const QRect &subtemplate_rect = this->get_subtemplate_rect(subtemplate);

	if (!subtemplate_rect.isValid()) {
		return QPoint(-1, -1);
	}

	return subtemplate_rect.topLeft();
}

QPoint CMap::get_subtemplate_center_pos(const wyrmgus::map_template *subtemplate) const
{
	const QRect &subtemplate_rect = this->get_subtemplate_rect(subtemplate);

	if (!subtemplate_rect.isValid()) {
		return QPoint(-1, -1);
	}

	const QPoint start_pos = subtemplate_rect.topLeft();
	const QPoint end_pos = subtemplate_rect.bottomRight();

	return start_pos + ((end_pos - start_pos) / 2);
}

QPoint CMap::get_subtemplate_end_pos(const wyrmgus::map_template *subtemplate) const
{
	const QRect &subtemplate_rect = this->get_subtemplate_rect(subtemplate);

	if (!subtemplate_rect.isValid()) {
		return QPoint(-1, -1);
	}

	return subtemplate_rect.bottomRight();
}

/**
**	@brief	Get the applied map layer of a given subtemplate
**
**	@param	subtemplate		The subtemplate
**
**	@return	The subtemplate's map layer if found, or null otherwise
*/
CMapLayer *CMap::get_subtemplate_map_layer(const wyrmgus::map_template *subtemplate) const
{
	if (!subtemplate) {
		return nullptr;
	}
	
	const wyrmgus::map_template *main_template = subtemplate->GetTopMapTemplate();
	if (main_template && subtemplate != main_template) {
		const int z = GetMapLayer(main_template->get_world() ? main_template->get_world()->get_identifier() : "");
		if (z != -1) {
			if (this->MapLayers[z]->has_subtemplate_area(subtemplate)) {
				return this->MapLayers[z].get();
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
std::vector<CUnit *> CMap::get_map_template_layer_connectors(const wyrmgus::map_template *map_template) const
{
	std::vector<CUnit *> layer_connectors;
	
	if (!map_template) {
		return layer_connectors;
	}
	
	const wyrmgus::map_template *main_template = map_template->GetTopMapTemplate();
	if (main_template) {
		const bool is_main_template = main_template == map_template;
		const int z = GetMapLayer(main_template->get_world() ? main_template->get_world()->get_identifier() : "");
		if (z != -1) {
			for (size_t i = 0; i < this->MapLayers[z]->LayerConnectors.size(); ++i) {
				CUnit *connector_unit = this->MapLayers[z]->LayerConnectors[i];
				const Vec2i unit_pos = connector_unit->get_center_tile_pos();
				
				if (is_main_template && this->is_point_in_a_subtemplate_area(unit_pos, z)) {
					continue;
				} else if (!is_main_template && !this->is_point_in_subtemplate_area(unit_pos, z, map_template)) {
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
			
			if (this->Info->IsPointOnMap(adjacent_pos, z) && !this->is_point_in_a_subtemplate_area(adjacent_pos, z)) {
				return true;
			}
		}
	}
	
	return false;
}

std::vector<const map_template *> CMap::get_pos_subtemplates(const QPoint &pos, const int z) const
{
	std::vector<const map_template *> subtemplates;

	for (const auto &kv_pair : this->MapLayers[z]->subtemplate_areas) {
		const map_template *subtemplate = kv_pair.first;
		const QRect &subtemplate_rect = kv_pair.second;
		if (subtemplate_rect.contains(pos)) {
			subtemplates.push_back(subtemplate);
		}
	}

	return subtemplates;
}

std::vector<const map_template *> CMap::get_rect_subtemplates(const QRect &rect, const int z) const
{
	std::vector<const map_template *> subtemplates;

	for (const auto &kv_pair : this->MapLayers[z]->subtemplate_areas) {
		const map_template *subtemplate = kv_pair.first;
		const QRect &subtemplate_rect = kv_pair.second;

		if (subtemplate_rect.intersects(rect)) {
			subtemplates.push_back(subtemplate);
		}
	}

	return subtemplates;
}

bool CMap::is_rect_in_settlement(const QRect &rect, const int z, const wyrmgus::site *settlement)
{
	for (int x = rect.x(); x <= rect.right(); ++x) {
		for (int y = rect.y(); y <= rect.bottom(); ++y) {
			const QPoint tile_pos(x, y);

			if (!this->Info->IsPointOnMap(tile_pos, z)) {
				return false;
			}

			const wyrmgus::tile *tile = this->Field(tile_pos, z);

			//doesn't return false for tiles with no settlement
			if (tile->get_settlement() == nullptr) {
				continue;
			}

			if (tile->get_settlement() != settlement) {
				return false;
			}
		}
	}

	return true;
}

const world *CMap::calculate_pos_world(const QPoint &pos, const int z, const bool include_adjacent) const
{
	std::vector<const map_template *> pos_subtemplates;

	if (include_adjacent) {
		pos_subtemplates = this->get_rect_subtemplates(QRect(pos - QPoint(1, 1), pos + QPoint(1, 1)), z);
	} else {
		pos_subtemplates = this->get_pos_subtemplates(pos, z);
	}

	for (const map_template *subtemplate : pos_subtemplates) {
		if (subtemplate->get_world() == nullptr) {
			continue;
		}

		if (include_adjacent) {
			//check if any adjacent point is in a usable part of the subtemplate
			const std::optional<QPoint> find_pos = point::find_adjacent_if(pos, [&](const QPoint &adjacent_point) {
				return subtemplate->is_map_pos_usable(adjacent_point);
			});

			if (!find_pos.has_value()) {
				continue;
			}
		} else {
			if (!subtemplate->is_map_pos_usable(pos)) {
				continue;
			}
		}

		return subtemplate->get_world();
	}

	const CMapLayer *map_layer = this->MapLayers[z].get();

	if (map_layer->world != nullptr) {
		return map_layer->world;
	}

	return nullptr;
}

void CMap::SetCurrentWorld(wyrmgus::world *world)
{
	if (UI.CurrentMapLayer->world == world) {
		return;
	}
	
	int map_layer = -1;
	
	for (size_t z = 0; z < this->MapLayers.size(); ++z) {
		if (this->MapLayers[z]->world == world) {
			map_layer = z;
			break;
		}
	}
	
	if (map_layer == -1) {
		for (size_t z = 0; z < this->MapLayers.size(); ++z) {
			if (this->MapLayers[z]->world == world) {
				map_layer = z;
				break;
			}
		}
	}
	
	if (map_layer != -1) {
		ChangeCurrentMapLayer(map_layer);
	}
}

const wyrmgus::world *CMap::GetCurrentWorld() const
{
	if (UI.CurrentMapLayer) {
		return UI.CurrentMapLayer->world;
	} else {
		return nullptr;
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
bool CheckedCanMoveToMask(const Vec2i &pos, const tile_flag mask, const int z)
{
	return CMap::get()->Info->IsPointOnMap(pos, z) && CanMoveToMask(pos, mask, z);
}

/**
**  Can a unit of unit-type be placed at this point.
**
**  @param type  unit-type to be checked.
**  @param pos   map tile position.
**
**  @return      True if could be entered, false otherwise.
*/
bool UnitTypeCanBeAt(const wyrmgus::unit_type &type, const Vec2i &pos, int z)
{
	const tile_flag mask = type.MovementMask;
	unsigned int index = pos.y * CMap::get()->Info->MapWidths[z];

	for (int addy = 0; addy < type.get_tile_height(); ++addy) {
		for (int addx = 0; addx < type.get_tile_width(); ++addx) {
			if (CMap::get()->Info->IsPointOnMap(pos.x + addx, pos.y + addy, z) == false) {
				return false;
			}

			const wyrmgus::tile *tile = CMap::get()->Field(pos.x + addx + index, z);
			if (tile->CheckMask(mask) == true || (tile->get_terrain() == nullptr && wyrmgus::game::get()->get_current_campaign() != nullptr)) {
				return false;
			}
			
		}
		index += CMap::get()->Info->MapWidths[z];
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
	try {
		ShowLoadProgress("%s", _("Initializing Map..."));

		//Wyrmgus start
		/*
		for (int ix = 0; ix < this->Info->MapWidth; ++ix) {
			for (int iy = 0; iy < this->Info->MapHeight; ++iy) {
				wyrmgus::tile &mf = *this->Field(ix, iy);
				mf.player_info->SeenTile = mf.getGraphicTile();
			}
		}
		*/

		for (size_t z = 0; z < CMap::get()->MapLayers.size(); ++z) {
			for (int ix = 0; ix < CMap::get()->Info->MapWidths[z]; ++ix) {
				for (int iy = 0; iy < CMap::get()->Info->MapHeights[z]; ++iy) {
					const QPoint tile_pos(ix, iy);
					wyrmgus::tile &mf = *CMap::get()->Field(tile_pos, z);
					CMap::get()->calculate_tile_solid_tile(tile_pos, false, z);
					if (mf.get_overlay_terrain() != nullptr) {
						CMap::get()->calculate_tile_solid_tile(tile_pos, true, z);
					}
					CMap::get()->CalculateTileTransitions(tile_pos, false, z);
					CMap::get()->CalculateTileTransitions(tile_pos, true, z);
				}
			}

			CMap::get()->expand_terrain_features_to_same_terrain(z);

			//settlement territories need to be generated after tile transitions are calculated, so that the coast map field has been set
			CMap::get()->generate_settlement_territories(z);

			for (int ix = 0; ix < CMap::get()->Info->MapWidths[z]; ++ix) {
				for (int iy = 0; iy < CMap::get()->Info->MapHeights[z]; ++iy) {
					const QPoint tile_pos(ix, iy);
					wyrmgus::tile &mf = *CMap::get()->Field(tile_pos, z);
					CMap::get()->CalculateTileLandmass(tile_pos, z);
					CMap::get()->CalculateTileOwnershipTransition(tile_pos, z);
					mf.bump_incompatible_units();
					mf.UpdateSeenTile();
					UI.get_minimap()->UpdateXY(tile_pos, z);
					UI.get_minimap()->update_territory_xy(tile_pos, z);
					if (mf.player_info->IsTeamVisible(*CPlayer::GetThisPlayer())) {
						CMap::get()->MarkSeenTile(mf);
					}
				}
			}
		}
		//Wyrmgus end

		//set the season and time of day for worlds
		const world *main_world = nullptr;
		int largest_world_area = 0;

		for (const world *world : world::get_all()) {
			const world_game_data *world_game_data = world->get_game_data();
			if (!world_game_data->is_on_map()) {
				continue;
			}

			const int map_area = world_game_data->get_map_rect().width() * world_game_data->get_map_rect().height();
			if (map_area > largest_world_area) {
				main_world = world;
				largest_world_area = map_area;
			}
		}

		for (const world *world : world::get_all()) {
			world_game_data *world_game_data = world->get_game_data();
			if (!world_game_data->is_on_map()) {
				continue;
			}

			uint64_t hours = game::get()->get_current_total_hours();
			if (world != main_world) {
				//for the main world, just use the current total hours, but for other worlds, use a random offset for their schedules, so that they have different times of day and seasons than the main world
				const unsigned total_schedule_hours = std::max(world->get_season_schedule()->get_total_hours(), world->get_time_of_day_schedule()->get_total_hours());
				hours += random::get()->generate(total_schedule_hours);
			}

			world_game_data->set_season_by_hours(hours);
			world_game_data->set_time_of_day_by_hours(hours);
		}

		CMap::get()->calculate_settlement_resource_units();

		//Wyrmgus start
		/*
		// it is required for fixing the wood that all tiles are marked as seen!
		if (this->Tileset->TileTypeTable.empty() == false) {
			Vec2i pos;
			for (pos.x = 0; pos.x < this->Info->MapWidth; ++pos.x) {
				for (pos.y = 0; pos.y < this->Info->MapHeight; ++pos.y) {
					MapFixWallTile(pos);
					MapFixSeenWallTile(pos);
				}
			}
		}
		*/
		//Wyrmgus end
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Error preprocessing the map."));
	}
}

//Wyrmgus start
int GetMapLayer(const std::string &world_ident)
{
	wyrmgus::world *world = wyrmgus::world::try_get(world_ident);

	for (size_t z = 0; z < CMap::get()->MapLayers.size(); ++z) {
		if (CMap::get()->MapLayers[z]->world == world) {
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
	if (z < 0 || z >= static_cast<int>(CMap::get()->MapLayers.size()) || UI.CurrentMapLayer->ID == z) {
		return;
	}
	
	Vec2i new_viewport_map_pos(UI.SelectedViewport->MapPos.x * CMap::get()->Info->MapWidths[z] / UI.CurrentMapLayer->get_width(), UI.SelectedViewport->MapPos.y * CMap::get()->Info->MapHeights[z] / UI.CurrentMapLayer->get_height());
	
	UI.PreviousMapLayer = UI.CurrentMapLayer;
	UI.CurrentMapLayer = CMap::get()->MapLayers[z].get();
	UI.get_minimap()->UpdateCache = true;
	UI.SelectedViewport->Set(new_viewport_map_pos, wyrmgus::size::to_point(wyrmgus::defines::get()->get_scaled_tile_size()) / 2);
}

/**
**	@brief	Set the current time of day for a particular map layer
**
**	@param	time_of_day_ident	The time of day's string identifier
**	@param	z					The map layer
*/
void SetTimeOfDay(const std::string &time_of_day_ident, int z)
{
	const std::unique_ptr<CMapLayer> &map_layer = CMap::get()->MapLayers[z];

	if (time_of_day_ident.empty()) {
		map_layer->SetTimeOfDay(nullptr);
		map_layer->RemainingTimeOfDayHours = 0;

		for (const world *world : world::get_all()) {
			world_game_data *world_game_data = world->get_game_data();

			if (!world_game_data->is_on_map()) {
				continue;
			}

			world_game_data->set_time_of_day(nullptr);
			world_game_data->set_remaining_time_of_day_hours(0);
		}

		return;
	}

	const time_of_day *time_of_day = time_of_day::try_get(time_of_day_ident);
	if (time_of_day == nullptr) {
		return;
	}

	const time_of_day_schedule *schedule = map_layer->get_time_of_day_schedule();
	if (schedule != nullptr) {
		for (const std::unique_ptr<scheduled_time_of_day> &scheduled_time_of_day : schedule->get_scheduled_times_of_day()) {
			if (scheduled_time_of_day->get_time_of_day() == time_of_day) {
				map_layer->SetTimeOfDay(scheduled_time_of_day.get());
				map_layer->RemainingTimeOfDayHours = scheduled_time_of_day->get_hours(map_layer->GetSeason());
				break;
			}
		}
	}

	for (const world *world : world::get_all()) {
		world_game_data *world_game_data = world->get_game_data();

		if (!world_game_data->is_on_map()) {
			continue;
		}

		for (const std::unique_ptr<scheduled_time_of_day> &scheduled_time_of_day : world->get_time_of_day_schedule()->get_scheduled_times_of_day()) {
			if (scheduled_time_of_day->get_time_of_day() == time_of_day) {
				world_game_data->set_time_of_day(scheduled_time_of_day.get());
				world_game_data->set_remaining_time_of_day_hours(scheduled_time_of_day->get_hours(world_game_data->get_season()));
				break;
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
	if (z >= static_cast<int>(CMap::get()->MapLayers.size())) {
		fprintf(stderr, "Error in CMap::SetTimeOfDaySchedule: the given map layer index (%d) is not valid given the map layer quantity (%ld).\n", z, CMap::get()->MapLayers.size());
		return;
	}

	if (time_of_day_schedule_ident.empty()) {
		CMap::get()->MapLayers[z]->set_time_of_day_schedule(nullptr);
		CMap::get()->MapLayers[z]->SetTimeOfDay(nullptr);
		CMap::get()->MapLayers[z]->RemainingTimeOfDayHours = 0;
	} else {
		const time_of_day_schedule *schedule = time_of_day_schedule::try_get(time_of_day_schedule_ident);
		if (schedule != nullptr) {
			CMap::get()->MapLayers[z]->set_time_of_day_schedule(schedule);
			CMap::get()->MapLayers[z]->SetTimeOfDay(schedule->get_scheduled_times_of_day().front().get());
			CMap::get()->MapLayers[z]->RemainingTimeOfDayHours = CMap::get()->MapLayers[z]->get_scheduled_time_of_day()->get_hours(CMap::get()->MapLayers[z]->GetSeason());
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
	const std::unique_ptr<CMapLayer> &map_layer = CMap::get()->MapLayers[z];

	if (season_ident.empty()) {
		map_layer->SetSeason(nullptr);
		map_layer->RemainingSeasonHours = 0;

		for (const world *world : world::get_all()) {
			world_game_data *world_game_data = world->get_game_data();

			if (!world_game_data->is_on_map()) {
				continue;
			}

			world_game_data->set_season(nullptr);
			world_game_data->set_remaining_season_hours(0);
		}

		return;
	}

	const season *season = season::try_get(season_ident);
	if (season == nullptr) {
		return;
	}

	const season_schedule *schedule = map_layer->get_season_schedule();
	if (schedule != nullptr) {
		for (const std::unique_ptr<scheduled_season> &scheduled_season : schedule->get_scheduled_seasons()) {
			if (scheduled_season->get_season() == season) {
				map_layer->SetSeason(scheduled_season.get());
				map_layer->RemainingSeasonHours = scheduled_season->get_hours();
				break;
			}
		}
	}

	for (const world *world : world::get_all()) {
		world_game_data *world_game_data = world->get_game_data();

		if (!world_game_data->is_on_map()) {
			continue;
		}

		for (const std::unique_ptr<scheduled_season> &scheduled_season : world->get_season_schedule()->get_scheduled_seasons()) {
			if (scheduled_season->get_season() == season) {
				world_game_data->set_season(scheduled_season.get());
				world_game_data->set_remaining_season_hours(scheduled_season->get_hours());
				break;
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
		CMap::get()->MapLayers[z]->set_season_schedule(nullptr);
		CMap::get()->MapLayers[z]->SetSeason(nullptr);
		CMap::get()->MapLayers[z]->RemainingSeasonHours = 0;
	} else {
		const season_schedule *schedule = season_schedule::try_get(season_schedule_ident);
		if (schedule != nullptr) {
			CMap::get()->MapLayers[z]->set_season_schedule(schedule);
			CMap::get()->MapLayers[z]->SetSeason(schedule->get_scheduled_seasons().front().get());
			CMap::get()->MapLayers[z]->RemainingSeasonHours = CMap::get()->MapLayers[z]->get_scheduled_season()->get_hours();
		}
	}
}
//Wyrmgus end

bool CanMoveToMask(const Vec2i &pos, const tile_flag mask, const int z)
{
	return !CMap::get()->Field(pos, z)->CheckMask(mask);
}

CMap::CMap()
{
	this->Tileset = std::make_unique<CTileset>();

	this->Info = make_qunique<map_info>();
	if (QApplication::instance()->thread() != QThread::currentThread()) {
		this->Info->moveToThread(QApplication::instance()->thread());
	}
}

CMap::~CMap()
{
}

int CMap::get_pos_index(const int x, const int y, const int z) const
{
	return point::to_index(x, y, this->Info->MapWidths[z]);
}

int CMap::get_pos_index(const QPoint &pos, const int z) const
{
	return point::to_index(pos, this->Info->MapWidths[z]);
}

/**
**	@brief	Get the map field at a given location
**
**	@param	index	The index of the map field
**	@param	z		The map layer of the map field
**
**	@return	The map field
*/
wyrmgus::tile *CMap::Field(const unsigned int index, const int z) const
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
wyrmgus::tile *CMap::Field(const int x, const int y, const int z) const
{
	return this->MapLayers[z]->Field(x, y);
}

/**
**	@brief	Allocate and initialize map table
*/
void CMap::Create()
{
	Assert(this->MapLayers.size() == 0);

	auto map_layer = std::make_unique<CMapLayer>(this->Info->MapWidth, this->Info->MapHeight);

	if (QApplication::instance()->thread() != QThread::currentThread()) {
		map_layer->moveToThread(QApplication::instance()->thread());
	}

	map_layer->ID = this->MapLayers.size();
	this->Info->MapWidths.push_back(this->Info->MapWidth);
	this->Info->MapHeights.push_back(this->Info->MapHeight);
	
	if (!CEditor::get()->is_running()) {
		map_layer->set_season_schedule(defines::get()->get_default_season_schedule());
		map_layer->SetSeasonByHours(game::get()->get_current_total_hours());
		
		if (!GameSettings.Inside) {
			map_layer->set_time_of_day_schedule(defines::get()->get_default_time_of_day_schedule());
			map_layer->SetTimeOfDayByHours(game::get()->get_current_total_hours());
		} else {
			map_layer->set_time_of_day_schedule(nullptr);
			map_layer->SetTimeOfDay(nullptr); // make indoors have no time of day setting until it is possible to make light sources change their surrounding "time of day" // indoors it is always dark (maybe would be better to allow a special setting to have bright indoor places?
		}
	}

	this->MapLayers.push_back(std::move(map_layer));
}

/**
**  Initialize the fog of war.
**  Build tables, setup functions.
*/
void CMap::Init()
{
	this->InitFogOfWar();
}

/**
**  Cleanup the map module.
*/
void CMap::Clean()
{
	UI.CurrentMapLayer = nullptr;
	UI.PreviousMapLayer = nullptr;
	this->landmasses.clear();

	for (world *world : world::get_all()) {
		world->reset_game_data();
	}

	for (site *site : site::get_all()) {
		site->reset_game_data();
	}

	for (map_template *map_template : map_template::get_all()) {
		map_template->reset_game_data();
	}

	//Wyrmgus start
	this->ClearMapLayers();
	this->settlement_units.clear();
	//Wyrmgus end

	// Tileset freed by Tileset?

	this->Info->Clear();
	this->NoFogOfWar = false;
	this->Tileset->clear();
	this->TileModelsFileName.clear();
	this->TileGraphic.reset();

	FlagRevealMap = 0;
	ReplayRevealMap = 0;

	UI.get_minimap()->Destroy();
}

void CMap::ClearMapLayers()
{
	this->MapLayers.clear();
}

QRect CMap::get_rect(const int z) const
{
	return QRect(QPoint(0, 0), QPoint(this->Info->MapWidths[z] - 1, this->Info->MapHeights[z] - 1));
}

void CMap::process_sml_property(const sml_property &property)
{
	const std::string &key = property.get_key();

	throw std::runtime_error("Invalid map data property: \"" + key + "\".");
}

void CMap::process_sml_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "size") {
		const QSize map_size = scope.to_size();
		this->Info->MapWidth = map_size.width();
		this->Info->MapHeight = map_size.height();

		this->ClearMapLayers();
		auto map_layer = std::make_unique<CMapLayer>(this->Info->MapWidth, this->Info->MapHeight);

		if (QApplication::instance()->thread() != QThread::currentThread()) {
			map_layer->moveToThread(QApplication::instance()->thread());
		}

		map_layer->ID = this->MapLayers.size();
		this->Info->MapWidths.clear();
		this->Info->MapWidths.push_back(this->Info->MapWidth);
		this->Info->MapHeights.clear();
		this->Info->MapHeights.push_back(this->Info->MapHeight);
		this->MapLayers.push_back(std::move(map_layer));
	} else if (tag == "extra_map_layers") {
		scope.for_each_child([&](const sml_data &map_layer_data) {
			//must process the size here already, as it is required for the map layer's constructor
			const QSize size = map_layer_data.get_child("size").to_size();

			auto map_layer = std::make_unique<CMapLayer>(size);

			if (QApplication::instance()->thread() != QThread::currentThread()) {
				map_layer->moveToThread(QApplication::instance()->thread());
			}

			database::process_sml_data(map_layer, map_layer_data);

			this->Info->MapWidths.push_back(map_layer->get_width());
			this->Info->MapHeights.push_back(map_layer->get_height());
			map_layer->ID = this->MapLayers.size();
			this->MapLayers.push_back(std::move(map_layer));
		});
	} else if (tag == "landmasses") {
		//first, create all landmasses, as when they are processed they refer to each other
		for (int i = 0; i < scope.get_children_count(); ++i) {
			this->add_landmass(std::make_unique<landmass>(static_cast<size_t>(i)));
		}

		//now, process the data for each landmass
		size_t current_index = 0;
		scope.for_each_child([&](const sml_data &landmass_data) {
			const std::unique_ptr<landmass> &landmass = this->get_landmasses()[current_index];
			database::process_sml_data(landmass, landmass_data);
			++current_index;
		});
	} else if (tag == "world_data") {
		scope.for_each_child([&](const sml_data &child_scope) {
			const world *world = world::get(child_scope.get_tag());
			database::process_sml_data(world->get_game_data(), child_scope);
		});
	} else {
		throw std::runtime_error("Invalid map data scope: \"" + scope.get_tag() + "\".");
	}
}

void CMap::save(CFile &file) const
{
	file.printf("\n--- -----------------------------------------\n");
	file.printf("--- MODULE: map\n");
	file.printf("LoadTileModels(\"%s\")\n\n", this->TileModelsFileName.c_str());

	sml_data map_data;

	map_data.add_child(sml_data::from_size(this->MapLayers.front()->get_size(), "size"));

	if (this->MapLayers.size() > 1) {
		sml_data extra_map_layers_data("extra_map_layers");
		for (size_t i = 1; i < this->MapLayers.size(); ++i) {
			const CMapLayer *map_layer = this->MapLayers.at(i).get();
			extra_map_layers_data.add_child(map_layer->to_sml_data());
		}
		map_data.add_child(std::move(extra_map_layers_data));
	}

	if (!this->get_landmasses().empty()) {
		sml_data landmasses_data("landmasses");
		for (const auto &landmass : this->get_landmasses()) {
			landmasses_data.add_child(landmass->to_sml_data());
		}
		map_data.add_child(std::move(landmasses_data));
	}

	sml_data world_game_data("world_data");
	for (const world *world : world::get_all()) {
		if (world->get_game_data() == nullptr) {
			continue;
		}

		if (!world->get_game_data()->is_on_map()) {
			continue;
		}

		sml_data world_data = world->get_game_data()->to_sml_data();

		if (world_data.is_empty()) {
			continue;
		}

		world_game_data.add_child(std::move(world_data));
	}
	if (!world_game_data.is_empty()) {
		map_data.add_child(std::move(world_game_data));
	}

	const std::string str = "load_map_data(\"" + string::escaped(map_data.print_to_string()) + "\")\n\n";
	file.printf("%s", str.c_str());

	file.printf("StratagusMap(\n");
	file.printf("  \"version\", \"%s\",\n", QApplication::applicationVersion().toStdString().c_str());
	file.printf("  \"description\", \"%s\",\n", this->Info->get_name().c_str());
	file.printf("  \"the-map\", {\n");
	file.printf("  \"%s\",\n", this->NoFogOfWar ? "no-fog-of-war" : "fog-of-war");
	file.printf("  \"filename\", \"%s\",\n", this->Info->get_presentation_filepath().string().c_str());
	//Wyrmgus start
	file.printf("  \"time-of-day\", {\n");
	for (size_t z = 0; z < this->MapLayers.size(); ++z) {
		file.printf("  {\"%s\", %zu, %d},\n", this->MapLayers[z]->get_time_of_day_schedule() ? this->MapLayers[z]->get_time_of_day_schedule()->get_identifier().c_str() : "", this->MapLayers[z]->get_scheduled_time_of_day() ? this->MapLayers[z]->get_scheduled_time_of_day()->get_index() : 0, this->MapLayers[z]->RemainingTimeOfDayHours);
	}
	file.printf("  },\n");
	file.printf("  \"season\", {\n");
	for (size_t z = 0; z < this->MapLayers.size(); ++z) {
		file.printf("  {\"%s\", %zu, %d},\n", this->MapLayers[z]->get_season_schedule() ? this->MapLayers[z]->get_season_schedule()->get_identifier().c_str() : "", this->MapLayers[z]->get_scheduled_season() ? this->MapLayers[z]->get_scheduled_season()->get_index() : 0, this->MapLayers[z]->RemainingSeasonHours);
	}
	file.printf("  },\n");
	file.printf("  \"layer-references\", {\n");
	for (size_t z = 0; z < this->MapLayers.size(); ++z) {
		file.printf("  {\"%s\"},\n", this->MapLayers[z]->world ? this->MapLayers[z]->world->get_identifier().c_str() : "");
	}
	file.printf("  },\n");
	//Wyrmgus end

	file.printf("  \"map-fields\", {\n");
	//Wyrmgus start
	/*
	for (int h = 0; h < this->Info->MapHeight; ++h) {
		file.printf("  -- %d\n", h);
		for (int w = 0; w < this->Info->MapWidth; ++w) {
			const wyrmgus::tile &mf = *this->Field(w, h);

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
		for (int h = 0; h < this->Info->MapHeights[z]; ++h) {
			file.printf("  -- %d\n", h);
			for (int w = 0; w < this->Info->MapWidths[z]; ++w) {
				const wyrmgus::tile &mf = *this->Field(w, h, z);

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
	Assert(type == tile_flag::tree || type == tile_flag::rock);

	//  Outside of map or no wood.
	if (!Info.IsPointOnMap(pos)) {
		return;
	}
	unsigned int index = getIndex(pos);
	wyrmgus::tile &mf = *this->Field(index);

	if (!((type == tile_flag::tree && Tileset->isAWoodTile(mf.player_info->SeenTile))
		  || (type == tile_flag::rock && Tileset->isARockTile(mf.player_info->SeenTile)))) {
		if (seen) {
			return;
		}
	}

	if (!seen && !(mf.get_flags() & type)) {
		return;
	}

	// Select Table to lookup
	int removedtile;
	int flags;
	if (type == tile_flag::tree) {
		removedtile = this->Tileset->getRemovedTreeTile();
		flags = (tile_flag::tree | tile_flag::impassable);
	} else { // (type == tile_flag::rock)
		removedtile = this->Tileset->getRemovedRockTile();
		flags = (tile_flag::rock | tile_flag::impassable);
	}
	//  Find out what each tile has with respect to wood, or grass.
	int ttup;
	int ttdown;
	int ttleft;
	int ttright;

	if (pos.y - 1 < 0) {
		ttup = -1; //Assign trees in all directions
	} else {
		const wyrmgus::tile &new_mf = *(&mf - this->Info->MapWidth);
		ttup = seen ? new_mf.player_info->SeenTile : new_mf.getGraphicTile();
	}
	if (pos.x + 1 >= this->Info->MapWidth) {
		ttright = -1; //Assign trees in all directions
	} else {
		const wyrmgus::tile &new_mf = *(&mf + 1);
		ttright = seen ? new_mf.player_info->SeenTile : new_mf.getGraphicTile();
	}
	if (pos.y + 1 >= this->Info->MapHeight) {
		ttdown = -1; //Assign trees in all directions
	} else {
		const wyrmgus::tile &new_mf = *(&mf + this->Info->MapWidth);
		ttdown = seen ? new_mf.player_info->SeenTile : new_mf.getGraphicTile();
	}
	if (pos.x - 1 < 0) {
		ttleft = -1; //Assign trees in all directions
	} else {
		const wyrmgus::tile &new_mf = *(&mf - 1);
		ttleft = seen ? new_mf.player_info->SeenTile : new_mf.getGraphicTile();
	}
	int tile = this->Tileset->getTileBySurrounding(type, ttup, ttright, ttdown, ttleft);

	//Update seen tile.
	if (tile == -1) { // No valid wood remove it.
		if (seen) {
			mf.player_info->SeenTile = removedtile;
			this->FixNeighbors(type, seen, pos);
		} else {
			mf.setGraphicTile(removedtile);
			mf.Flags &= ~flags;
			mf.set_value(0);
			UI.get_minimap()->UpdateXY(pos);
		}
	} else if (seen && this->Tileset->isEquivalentTile(tile, mf.player_info->SeenTile)) { //Same Type
		return;
	} else {
		if (seen) {
			mf.player_info->SeenTile = tile;
		} else {
			mf.setGraphicTile(tile);
		}
	}

	//maybe isExplored
	if (mf.player_info->IsExplored(*ThisPlayer)) {
		UI.get_minimap()->UpdateSeenXY(pos);
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
void CMap::SetTileTerrain(const QPoint &pos, const terrain_type *terrain, const int z)
{
	if (!terrain) {
		return;
	}
	
	try {
		const CMapLayer *map_layer = this->MapLayers[z].get();

		tile *tile = map_layer->Field(pos);

		const terrain_type *old_terrain = this->GetTileTerrain(pos, terrain->is_overlay(), z);

		if (terrain == old_terrain) {
			return;
		}

		const terrain_type *old_base_terrain = tile->get_terrain();
		const terrain_type *old_overlay_terrain = tile->get_overlay_terrain();
		const short old_base_solid_tile = tile->SolidTile;
		const short old_overlay_solid_tile = tile->OverlaySolidTile;
		const size_t old_base_transition_count = tile->TransitionTiles.size();
		const size_t old_overlay_transition_count = tile->OverlayTransitionTiles.size();

		tile->SetTerrain(terrain);

		if (terrain->is_overlay()) {
			//remove decorations if the overlay terrain has changed
			std::vector<CUnit *> table;
			Select(pos, pos, table, z);
			for (size_t i = 0; i != table.size(); ++i) {
				if (table[i] && table[i]->IsAlive() && table[i]->Type->UnitType == UnitTypeType::Land && table[i]->Type->BoolFlag[DECORATION_INDEX].value) {
					if (!CEditor::get()->is_running()) {
						LetUnitDie(*table[i]);
					} else {
						EditorActionRemoveUnit(*table[i], false);
					}
				}
			}
		}

		//recalculate transitions and solid tiles for both non-overlay and overlay, since setting one may have changed the other
		this->calculate_tile_solid_tile(pos, false, z);
		if (tile->get_overlay_terrain() != nullptr) {
			this->calculate_tile_solid_tile(pos, true, z);
		}
		this->CalculateTileTransitions(pos, false, z);
		this->CalculateTileTransitions(pos, true, z);

		if (tile->player_info->IsTeamVisible(*CPlayer::GetThisPlayer())) {
			MarkSeenTile(*tile);
		}
		UI.get_minimap()->UpdateXY(pos, z);

		const player_color *player_color = tile->get_player_color();

		if (old_base_terrain != tile->get_terrain() || old_base_solid_tile != tile->SolidTile) {
			emit map_layer->tile_image_changed(pos, tile->get_terrain(), tile->SolidTile, player_color);
		}

		if (old_overlay_terrain != tile->get_overlay_terrain() || old_overlay_solid_tile != tile->OverlaySolidTile) {
			emit map_layer->tile_overlay_image_changed(pos, tile->get_overlay_terrain(), tile->OverlaySolidTile, player_color);
		}

		if (old_base_transition_count != 0 || tile->TransitionTiles.size() != 0) {
			emit map_layer->tile_transition_images_changed(pos, tile->TransitionTiles, player_color);
		}

		if (old_overlay_transition_count != 0 || tile->OverlayTransitionTiles.size() != 0) {
			emit map_layer->tile_overlay_transition_images_changed(pos, tile->OverlayTransitionTiles, player_color);
		}

		for (int x_offset = -1; x_offset <= 1; ++x_offset) {
			for (int y_offset = -1; y_offset <= 1; ++y_offset) {
				if (x_offset != 0 || y_offset != 0) {
					const QPoint adjacent_pos(pos.x() + x_offset, pos.y() + y_offset);

					if (!this->Info->IsPointOnMap(adjacent_pos, z)) {
						continue;
					}

					wyrmgus::tile *adjacent_tile = map_layer->Field(adjacent_pos);

					if (terrain->is_overlay() && adjacent_tile->get_overlay_terrain() != terrain && adjacent_tile->get_overlay_terrain() != old_terrain && !CEditor::get()->is_running()) {
						continue;
					}

					const size_t old_adjacent_base_transition_count = adjacent_tile->TransitionTiles.size();
					const size_t old_adjacent_overlay_transition_count = adjacent_tile->OverlayTransitionTiles.size();

					this->CalculateTileTransitions(adjacent_pos, false, z);
					this->CalculateTileTransitions(adjacent_pos, true, z);

					if (adjacent_tile->player_info->IsTeamVisible(*CPlayer::GetThisPlayer())) {
						MarkSeenTile(*adjacent_tile);
					}
					UI.get_minimap()->UpdateXY(adjacent_pos, z);

					const wyrmgus::player_color *adjacent_player_color = adjacent_tile->get_player_color();

					if (old_adjacent_base_transition_count != 0 || adjacent_tile->TransitionTiles.size() != 0) {
						emit map_layer->tile_transition_images_changed(adjacent_pos, adjacent_tile->TransitionTiles, adjacent_player_color);
					}

					if (old_adjacent_overlay_transition_count != 0 || adjacent_tile->OverlayTransitionTiles.size() != 0) {
						emit map_layer->tile_overlay_transition_images_changed(adjacent_pos, adjacent_tile->OverlayTransitionTiles, adjacent_player_color);
					}
				}
			}
		}
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Error setting terrain \"" + terrain->get_identifier() + "\" for tile " + point::to_string(pos) + ", map layer " + std::to_string(z) + "."));
	}
}

void CMap::RemoveTileOverlayTerrain(const QPoint &pos, const int z)
{
	const CMapLayer *map_layer = this->MapLayers[z].get();

	tile *tile = map_layer->Field(pos);
	
	if (tile->get_overlay_terrain() == nullptr) {
		return;
	}
	
	const size_t old_overlay_transition_count = tile->OverlayTransitionTiles.size();

	tile->RemoveOverlayTerrain();
	
	this->CalculateTileTransitions(pos, true, z);
	
	if (tile->player_info->IsTeamVisible(*CPlayer::GetThisPlayer())) {
		MarkSeenTile(*tile);
	}
	UI.get_minimap()->UpdateXY(pos, z);

	const player_color *player_color = tile->get_player_color();

	emit map_layer->tile_overlay_image_changed(pos, tile->get_overlay_terrain(), tile->OverlaySolidTile, player_color);

	if (old_overlay_transition_count != 0 || tile->OverlayTransitionTiles.size() != 0) {
		emit map_layer->tile_overlay_transition_images_changed(pos, tile->OverlayTransitionTiles, player_color);
	}

	for (int x_offset = -1; x_offset <= 1; ++x_offset) {
		for (int y_offset = -1; y_offset <= 1; ++y_offset) {
			if (x_offset != 0 || y_offset != 0) {
				const QPoint adjacent_pos(pos.x() + x_offset, pos.y() + y_offset);

				if (!this->Info->IsPointOnMap(adjacent_pos, z)) {
					continue;
				}

				wyrmgus::tile *adjacent_tile = map_layer->Field(adjacent_pos);

				const size_t old_adjacent_overlay_transition_count = adjacent_tile->OverlayTransitionTiles.size();

				this->CalculateTileTransitions(adjacent_pos, true, z);

				if (adjacent_tile->player_info->IsTeamVisible(*CPlayer::GetThisPlayer())) {
					MarkSeenTile(*adjacent_tile);
				}
				UI.get_minimap()->UpdateXY(adjacent_pos, z);

				if (old_adjacent_overlay_transition_count != 0 || adjacent_tile->OverlayTransitionTiles.size() != 0) {
					const wyrmgus::player_color *adjacent_player_color = adjacent_tile->get_player_color();

					emit map_layer->tile_overlay_transition_images_changed(adjacent_pos, adjacent_tile->OverlayTransitionTiles, adjacent_player_color);
				}
			}
		}
	}
}

void CMap::SetOverlayTerrainDestroyed(const QPoint &pos, const bool destroyed, const int z)
{
	try {
		CMapLayer *map_layer = this->MapLayers[z].get();

		if (!map_layer) {
			return;
		}

		tile *tile = map_layer->Field(pos);

		if (tile->get_overlay_terrain() == nullptr || tile->OverlayTerrainDestroyed == destroyed) {
			return;
		}

		const short old_overlay_solid_tile = tile->OverlaySolidTile;
		const size_t old_overlay_transition_count = tile->OverlayTransitionTiles.size();

		tile->SetOverlayTerrainDestroyed(destroyed);

		if (destroyed) {
			if (tile->get_overlay_terrain()->has_flag(tile_flag::tree)) {
				tile->Flags &= ~(tile_flag::tree | tile_flag::impassable);
				tile->Flags |= tile_flag::stumps;
				map_layer->destroyed_tree_tiles.push_back(pos);
			} else {
				if (tile->get_overlay_terrain()->has_flag(tile_flag::rock)) {
					tile->Flags &= ~(tile_flag::rock | tile_flag::impassable);
					tile->Flags |= tile_flag::gravel;
				} else if (tile->get_overlay_terrain()->has_flag(tile_flag::wall)) {
					tile->Flags &= ~(tile_flag::wall | tile_flag::impassable);
					tile->Flags |= tile_flag::gravel;
					if (tile->has_flag(tile_flag::underground)) {
						tile->Flags &= ~(tile_flag::air_impassable);
					}
				}

				map_layer->destroyed_overlay_terrain_tiles.push_back(pos);
			}

			tile->set_value(0);
		} else {
			if (tile->has_flag(tile_flag::stumps)) { //if is a cleared tree tile regrowing trees
				tile->Flags &= ~(tile_flag::stumps);
				tile->Flags |= tile_flag::tree | tile_flag::impassable;
				tile->set_value(tile->get_overlay_terrain()->get_resource()->get_default_amount());
			}
		}

		if (destroyed) {
			if (tile->get_overlay_terrain()->get_destroyed_tiles().size() > 0) {
				tile->OverlaySolidTile = vector::get_random(tile->get_overlay_terrain()->get_destroyed_tiles());
			}
		} else {
			this->calculate_tile_solid_tile(pos, true, z);
		}

		this->CalculateTileTransitions(pos, true, z);

		if (tile->player_info->IsTeamVisible(*CPlayer::GetThisPlayer())) {
			MarkSeenTile(*tile);
		}
		UI.get_minimap()->UpdateXY(pos, z);

		const player_color *player_color = tile->get_player_color();

		if (old_overlay_solid_tile != tile->OverlaySolidTile) {
			emit map_layer->tile_overlay_image_changed(pos, tile->get_overlay_terrain(), tile->OverlaySolidTile, player_color);
		}

		if (old_overlay_transition_count != 0 || tile->OverlayTransitionTiles.size() != 0) {
			emit map_layer->tile_overlay_transition_images_changed(pos, tile->OverlayTransitionTiles, player_color);
		}

		for (int x_offset = -1; x_offset <= 1; ++x_offset) {
			for (int y_offset = -1; y_offset <= 1; ++y_offset) {
				if (x_offset != 0 || y_offset != 0) {
					const QPoint adjacent_pos(pos.x() + x_offset, pos.y() + y_offset);
					if (!this->Info->IsPointOnMap(adjacent_pos, z)) {
						continue;
					}

					wyrmgus::tile *adjacent_tile = map_layer->Field(adjacent_pos);

					if (adjacent_tile->get_overlay_terrain() != adjacent_tile->get_overlay_terrain()) {
						continue;
					}

					const size_t old_adjacent_overlay_transition_count = adjacent_tile->OverlayTransitionTiles.size();

					this->CalculateTileTransitions(adjacent_pos, true, z);

					if (adjacent_tile->player_info->IsTeamVisible(*CPlayer::GetThisPlayer())) {
						MarkSeenTile(*adjacent_tile);
					}
					UI.get_minimap()->UpdateXY(adjacent_pos, z);

					if (old_adjacent_overlay_transition_count != 0 || adjacent_tile->OverlayTransitionTiles.size() != 0) {
						const wyrmgus::player_color *adjacent_player_color = adjacent_tile->get_player_color();

						emit map_layer->tile_overlay_transition_images_changed(adjacent_pos, adjacent_tile->OverlayTransitionTiles, adjacent_player_color);
					}
				}
			}
		}
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Error setting the overlay terrain of tile " + point::to_string(pos) + ", map layer " + std::to_string(z) + " to " + (destroyed ? "" : "not") + " destroyed."));
	}
}

void CMap::SetOverlayTerrainDamaged(const QPoint &pos, const bool damaged, const int z)
{
	try {
		const CMapLayer *map_layer = this->MapLayers[z].get();

		tile *tile = map_layer->Field(pos);

		if (tile->get_overlay_terrain() == nullptr || tile->OverlayTerrainDamaged == damaged) {
			return;
		}

		const short old_overlay_solid_tile = tile->OverlaySolidTile;
		const size_t old_overlay_transition_count = tile->OverlayTransitionTiles.size();

		tile->SetOverlayTerrainDamaged(damaged);

		if (damaged) {
			if (tile->get_overlay_terrain()->get_damaged_tiles().size() > 0) {
				tile->OverlaySolidTile = vector::get_random(tile->get_overlay_terrain()->get_damaged_tiles());
			}
		} else {
			this->calculate_tile_solid_tile(pos, true, z);
		}

		this->CalculateTileTransitions(pos, true, z);

		if (tile->player_info->IsTeamVisible(*CPlayer::GetThisPlayer())) {
			MarkSeenTile(*tile);
		}
		UI.get_minimap()->UpdateXY(pos, z);

		const player_color *player_color = tile->get_player_color();

		if (old_overlay_solid_tile != tile->OverlaySolidTile) {
			emit map_layer->tile_overlay_image_changed(pos, tile->get_overlay_terrain(), tile->OverlaySolidTile, player_color);
		}

		if (old_overlay_transition_count != 0 || tile->OverlayTransitionTiles.size() != 0) {
			emit map_layer->tile_overlay_transition_images_changed(pos, tile->OverlayTransitionTiles, player_color);
		}
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Error setting the overlay terrain of tile " + point::to_string(pos) + ", map layer " + std::to_string(z) + " to " + (damaged ? "" : "not") + " damaged."));
	}
}

static tile_transition_type GetTransitionType(std::vector<int> &adjacent_directions, const bool allow_single = false)
{
	if (adjacent_directions.size() == 0) {
		return tile_transition_type::none;
	}
	
	tile_transition_type transition_type = tile_transition_type::none;

	if (allow_single && vector::contains(adjacent_directions, North) && vector::contains(adjacent_directions, South) && vector::contains(adjacent_directions, West) && vector::contains(adjacent_directions, East)) {
		transition_type = tile_transition_type::single;
	} else if (allow_single && vector::contains(adjacent_directions, North) && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && vector::contains(adjacent_directions, West) && vector::contains(adjacent_directions, East)) {
		transition_type = tile_transition_type::north_single;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && vector::contains(adjacent_directions, South) && vector::contains(adjacent_directions, West) && vector::contains(adjacent_directions, East)) {
		transition_type = tile_transition_type::south_single;
	} else if (allow_single && vector::contains(adjacent_directions, North) && vector::contains(adjacent_directions, South) && vector::contains(adjacent_directions, West) && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end()) {
		transition_type = tile_transition_type::west_single;
	} else if (allow_single && vector::contains(adjacent_directions, North) && vector::contains(adjacent_directions, South) && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && vector::contains(adjacent_directions, East)) {
		transition_type = tile_transition_type::east_single;
	} else if (allow_single && vector::contains(adjacent_directions, North) && vector::contains(adjacent_directions, South) && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end()) {
		transition_type = tile_transition_type::north_south;
	} else if (allow_single && vector::contains(adjacent_directions, West) && vector::contains(adjacent_directions, East) && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end()) {
		transition_type = tile_transition_type::west_east;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end() && vector::contains(adjacent_directions, North) && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && vector::contains(adjacent_directions, Southwest) && vector::contains(adjacent_directions, Southeast)) {
		transition_type = tile_transition_type::north_southwest_inner_southeast_inner;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end() && vector::contains(adjacent_directions, North) && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && vector::contains(adjacent_directions, Southwest)) {
		transition_type = tile_transition_type::north_southwest_inner;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end() && vector::contains(adjacent_directions, North) && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && vector::contains(adjacent_directions, Southeast)) {
		transition_type = tile_transition_type::north_southeast_inner;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && vector::contains(adjacent_directions, South) && vector::contains(adjacent_directions, Northwest) && vector::contains(adjacent_directions, Northeast)) {
		transition_type = tile_transition_type::south_northwest_inner_northeast_inner;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && vector::contains(adjacent_directions, South) && vector::contains(adjacent_directions, Northwest)) {
		transition_type = tile_transition_type::south_northwest_inner;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && vector::contains(adjacent_directions, South) && vector::contains(adjacent_directions, Northeast)) {
		transition_type = tile_transition_type::south_northeast_inner;
	} else if (allow_single && vector::contains(adjacent_directions, West) && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && vector::contains(adjacent_directions, Northeast) && vector::contains(adjacent_directions, Southeast)) {
		transition_type = tile_transition_type::west_northeast_inner_southeast_inner;
	} else if (allow_single && vector::contains(adjacent_directions, West) && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && vector::contains(adjacent_directions, Northeast)) {
		transition_type = tile_transition_type::west_northeast_inner;
	} else if (allow_single && vector::contains(adjacent_directions, West) && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && vector::contains(adjacent_directions, Southeast)) {
		transition_type = tile_transition_type::west_southeast_inner;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && vector::contains(adjacent_directions, East) && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && vector::contains(adjacent_directions, Northwest) && vector::contains(adjacent_directions, Southwest)) {
		transition_type = tile_transition_type::east_northwest_inner_southwest_inner;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && vector::contains(adjacent_directions, East) && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && vector::contains(adjacent_directions, Northwest)) {
		transition_type = tile_transition_type::east_northwest_inner;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && vector::contains(adjacent_directions, East) && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && vector::contains(adjacent_directions, Southwest)) {
		transition_type = tile_transition_type::east_southwest_inner;
	} else if (allow_single && vector::contains(adjacent_directions, West) && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end() && vector::contains(adjacent_directions, North) && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && vector::contains(adjacent_directions, Southeast)) {
		transition_type = tile_transition_type::northwest_outer_southeast_inner;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && vector::contains(adjacent_directions, East) && vector::contains(adjacent_directions, North) && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && vector::contains(adjacent_directions, Southwest)) {
		transition_type = tile_transition_type::northeast_outer_southwest_inner;
	} else if (allow_single && vector::contains(adjacent_directions, West) && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && vector::contains(adjacent_directions, South) && vector::contains(adjacent_directions, Northeast)) {
		transition_type = tile_transition_type::southwest_outer_northeast_inner;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && vector::contains(adjacent_directions, East) && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && vector::contains(adjacent_directions, South) && vector::contains(adjacent_directions, Northwest)) {
		transition_type = tile_transition_type::southeast_outer_northwest_inner;
	} else if (vector::contains(adjacent_directions, North) && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southwest) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southeast) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end()) {
		transition_type = tile_transition_type::north;
	} else if (vector::contains(adjacent_directions, South) && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northwest) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northeast) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end()) {
		transition_type = tile_transition_type::south;
	} else if (vector::contains(adjacent_directions, West) && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northeast) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southeast) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end()) {
		transition_type = tile_transition_type::west;
	} else if (vector::contains(adjacent_directions, East) && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northwest) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southwest) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end()) {
		transition_type = tile_transition_type::east;
	} else if ((vector::contains(adjacent_directions, North) || vector::contains(adjacent_directions, West)) && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southeast) == adjacent_directions.end()) {
		transition_type = tile_transition_type::northwest_outer;
	} else if ((vector::contains(adjacent_directions, North) || vector::contains(adjacent_directions, East)) && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southwest) == adjacent_directions.end()) {
		transition_type = tile_transition_type::northeast_outer;
	} else if ((vector::contains(adjacent_directions, South) || vector::contains(adjacent_directions, West)) && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northeast) == adjacent_directions.end()) {
		transition_type = tile_transition_type::southwest_outer;
	} else if ((vector::contains(adjacent_directions, South) || vector::contains(adjacent_directions, East)) && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northwest) == adjacent_directions.end()) {
		transition_type = tile_transition_type::southeast_outer;
	} else if (allow_single && vector::contains(adjacent_directions, Northwest) && vector::contains(adjacent_directions, Southeast) && vector::contains(adjacent_directions, Northeast) && vector::contains(adjacent_directions, Southwest) && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end()) {
		transition_type = tile_transition_type::northwest_northeast_southwest_southeast_inner;
	} else if (allow_single && vector::contains(adjacent_directions, Northwest) && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southeast) == adjacent_directions.end() && vector::contains(adjacent_directions, Northeast) && vector::contains(adjacent_directions, Southwest) && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end()) {
		transition_type = tile_transition_type::northwest_northeast_southwest_inner;
	} else if (allow_single && vector::contains(adjacent_directions, Northwest) && vector::contains(adjacent_directions, Southeast) && vector::contains(adjacent_directions, Northeast) && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southwest) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end()) {
		transition_type = tile_transition_type::northwest_northeast_southeast_inner;
	} else if (allow_single && vector::contains(adjacent_directions, Northwest) && vector::contains(adjacent_directions, Southeast) && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northeast) == adjacent_directions.end() && vector::contains(adjacent_directions, Southwest) && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end()) {
		transition_type = tile_transition_type::northwest_southwest_southeast_inner;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northwest) == adjacent_directions.end() && vector::contains(adjacent_directions, Southeast) && vector::contains(adjacent_directions, Northeast) && vector::contains(adjacent_directions, Southwest) && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end()) {
		transition_type = tile_transition_type::northeast_southwest_southeast_inner;
	} else if (allow_single && vector::contains(adjacent_directions, Northwest) && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southeast) == adjacent_directions.end() && vector::contains(adjacent_directions, Northeast) && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southwest) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end()) {
		transition_type = tile_transition_type::northwest_northeast_inner;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northwest) == adjacent_directions.end() && vector::contains(adjacent_directions, Southeast) && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northeast) == adjacent_directions.end() && vector::contains(adjacent_directions, Southwest) && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end()) {
		transition_type = tile_transition_type::southwest_southeast_inner;
	} else if (allow_single && vector::contains(adjacent_directions, Northwest) && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southeast) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northeast) == adjacent_directions.end() && vector::contains(adjacent_directions, Southwest) && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end()) {
		transition_type = tile_transition_type::northwest_southwest_inner;
	} else if (allow_single && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northwest) == adjacent_directions.end() && vector::contains(adjacent_directions, Southeast) && vector::contains(adjacent_directions, Northeast) && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southwest) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end()) {
		transition_type = tile_transition_type::northeast_southeast_inner;
	} else if (vector::contains(adjacent_directions, Northwest) && vector::contains(adjacent_directions, Southeast) && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northeast) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southwest) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end()) {
		transition_type = tile_transition_type::northwest_southeast_inner;
	} else if (vector::contains(adjacent_directions, Northeast) && vector::contains(adjacent_directions, Southwest) && std::find(adjacent_directions.begin(), adjacent_directions.end(), Northwest) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), Southeast) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end()) {
		transition_type = tile_transition_type::northeast_southwest_inner;
	} else if (vector::contains(adjacent_directions, Northwest) && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end()) {
		transition_type = tile_transition_type::northwest_inner;
	} else if (vector::contains(adjacent_directions, Northeast) && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end()) {
		transition_type = tile_transition_type::northeast_inner;
	} else if (vector::contains(adjacent_directions, Southwest) && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end()) {
		transition_type = tile_transition_type::southwest_inner;
	} else if (vector::contains(adjacent_directions, Southeast) && std::find(adjacent_directions.begin(), adjacent_directions.end(), North) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), South) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), West) == adjacent_directions.end() && std::find(adjacent_directions.begin(), adjacent_directions.end(), East) == adjacent_directions.end()) {
		transition_type = tile_transition_type::southeast_inner;
	}

	return transition_type;
}

void CMap::calculate_tile_solid_tile(const QPoint &pos, const bool overlay, const int z)
{
	tile *tile = this->Field(pos, z);

	const terrain_type *terrain_type = nullptr;
	int solid_tile = 0;

	if (overlay) {
		terrain_type = tile->get_overlay_terrain();
	} else {
		terrain_type = tile->get_terrain();
	}

	if (terrain_type == nullptr) {
		const std::vector<const map_template *> pos_subtemplates = get_pos_subtemplates(pos, z);

		std::string error_message = "Failed to calculate solid tile for tile " + point::to_string(pos) + ", map layer " + std::to_string(z);

		if (!pos_subtemplates.empty()) {
			error_message += ", subtemplate area \"" + pos_subtemplates.front()->get_identifier() + "\"";
		}

		error_message += ": " + std::string(overlay ? "overlay " : "") + "terrain is null.";

		throw std::runtime_error(error_message);
	}

	if (terrain_type->has_tiled_background()) {
		const std::shared_ptr<CPlayerColorGraphic> &terrain_graphics = terrain_type->get_graphics();
		const int solid_tile_frame_x = pos.x() % terrain_graphics->get_frames_per_row();
		const int solid_tile_frame_y = pos.y() % terrain_graphics->get_frames_per_column();
		solid_tile = terrain_graphics->get_frame_index(QPoint(solid_tile_frame_x, solid_tile_frame_y));
	} else {
		if (!terrain_type->get_solid_tiles().empty()) {
			solid_tile = vector::get_random(terrain_type->get_solid_tiles());
		}
	}

	if (overlay) {
		tile->OverlaySolidTile = solid_tile;
	} else {
		tile->SolidTile = solid_tile;
	}
}

void CMap::CalculateTileTransitions(const Vec2i &pos, bool overlay, int z)
{
	tile &mf = *this->Field(pos, z);
	const terrain_type *terrain = nullptr;
	if (overlay) {
		terrain = mf.get_overlay_terrain();
		mf.OverlayTransitionTiles.clear();
	} else {
		terrain = mf.get_terrain();
		mf.TransitionTiles.clear();
	}
	
	if (!terrain || (overlay && mf.OverlayTerrainDestroyed)) {
		return;
	}
	
	std::map<int, std::vector<int>> adjacent_terrain_directions;
	
	for (int x_offset = -1; x_offset <= 1; ++x_offset) {
		for (int y_offset = -1; y_offset <= 1; ++y_offset) {
			if (x_offset != 0 || y_offset != 0) {
				Vec2i adjacent_pos(pos.x + x_offset, pos.y + y_offset);
				if (this->Info->IsPointOnMap(adjacent_pos, z)) {
					const terrain_type *adjacent_terrain = this->GetTileTerrain(adjacent_pos, overlay, z);
					if (overlay && adjacent_terrain && this->Field(adjacent_pos, z)->OverlayTerrainDestroyed) {
						adjacent_terrain = nullptr;
					}
					if (adjacent_terrain && terrain != adjacent_terrain) {
						if (vector::contains(terrain->get_inner_border_terrain_types(), adjacent_terrain)) {
							adjacent_terrain_directions[adjacent_terrain->ID].push_back(GetDirectionFromOffset(x_offset, y_offset));
						} else if (!terrain->is_border_terrain_type(adjacent_terrain)) {
							//if the two terrain types can't border, look for a third terrain type which can border both, and which treats both as outer border terrains, and then use for transitions between both tiles
							for (const terrain_type *border_terrain : terrain->BorderTerrains) {
								if (vector::contains(terrain->get_inner_border_terrain_types(), border_terrain) && vector::contains(adjacent_terrain->get_inner_border_terrain_types(), border_terrain)) {
									adjacent_terrain_directions[border_terrain->ID].push_back(GetDirectionFromOffset(x_offset, y_offset));
									break;
								}
							}
						}
					}
					if (!adjacent_terrain || (overlay && terrain != adjacent_terrain && !terrain->is_border_terrain_type(adjacent_terrain))) { // happens if terrain is null or if it is an overlay tile which doesn't have a border with this one, so that i.e. tree transitions display correctly when adjacent to tiles without overlays
						adjacent_terrain_directions[terrain_type::get_all().size()].push_back(GetDirectionFromOffset(x_offset, y_offset));
					}
				}
			}
		}
	}
	
	for (std::map<int, std::vector<int>>::iterator iterator = adjacent_terrain_directions.begin(); iterator != adjacent_terrain_directions.end(); ++iterator) {
		int adjacent_terrain_id = iterator->first;
		terrain_type *adjacent_terrain = adjacent_terrain_id < (int) terrain_type::get_all().size() ? terrain_type::get_all()[adjacent_terrain_id] : nullptr;
		const tile_transition_type transition_type = GetTransitionType(iterator->second, terrain->allows_single());
		
		if (transition_type != tile_transition_type::none) {
			bool found_transition = false;
			
			if (!overlay) {
				if (adjacent_terrain != nullptr) {
					const std::vector<int> &transition_tiles = terrain->get_transition_tiles(adjacent_terrain, transition_type);
					if (!transition_tiles.empty()) {
						mf.TransitionTiles.emplace_back(terrain, vector::get_random(transition_tiles));
						found_transition = true;
					} else {
						const std::vector<int> &adjacent_transition_tiles = adjacent_terrain->get_adjacent_transition_tiles(terrain, transition_type);
						if (!adjacent_transition_tiles.empty()) {
							mf.TransitionTiles.emplace_back(adjacent_terrain, vector::get_random(adjacent_transition_tiles));
							found_transition = true;
						} else {
							const std::vector<int> &sub_adjacent_transition_tiles = adjacent_terrain->get_adjacent_transition_tiles(nullptr, transition_type);
							if (!sub_adjacent_transition_tiles.empty()) {
								mf.TransitionTiles.emplace_back(adjacent_terrain, vector::get_random(sub_adjacent_transition_tiles));
								found_transition = true;
							}
						}
					}
				} else {
					const std::vector<int> &transition_tiles = terrain->get_transition_tiles(nullptr, transition_type);
					if (!transition_tiles.empty()) {
						mf.TransitionTiles.emplace_back(terrain, vector::get_random(transition_tiles));
					}
				}
			} else {
				if (adjacent_terrain != nullptr) {
					const std::vector<int> &transition_tiles = terrain->get_transition_tiles(adjacent_terrain, transition_type);
					if (!transition_tiles.empty()) {
						mf.OverlayTransitionTiles.emplace_back(terrain, vector::get_random(transition_tiles));
						found_transition = true;
					} else {
						const std::vector<int> &adjacent_transition_tiles = adjacent_terrain->get_transition_tiles(terrain, transition_type);
						if (!adjacent_transition_tiles.empty()) {
							mf.OverlayTransitionTiles.emplace_back(adjacent_terrain, vector::get_random(adjacent_transition_tiles));
							found_transition = true;
						} else {
							const std::vector<int> &sub_adjacent_transition_tiles = adjacent_terrain->get_transition_tiles(nullptr, transition_type);
							if (!sub_adjacent_transition_tiles.empty()) {
								mf.OverlayTransitionTiles.emplace_back(adjacent_terrain, vector::get_random(sub_adjacent_transition_tiles));
								found_transition = true;
							}
						}
					}
				} else {
					const std::vector<int> &transition_tiles = terrain->get_transition_tiles(nullptr, transition_type);
					if (!transition_tiles.empty()) {
						mf.OverlayTransitionTiles.emplace_back(terrain, vector::get_random(transition_tiles));
					}
				}
				
				if (mf.has_flag(tile_flag::water_allowed) && (!adjacent_terrain || !adjacent_terrain->has_flag(tile_flag::water_allowed))) {
					//if this is a water tile adjacent to a non-water tile, replace the water flag with a coast one
					mf.Flags &= ~(tile_flag::water_allowed);
					mf.Flags |= tile_flag::coast_allowed;
				}
				
				if (mf.has_flag(tile_flag::space) && (!adjacent_terrain || !adjacent_terrain->has_flag(tile_flag::space))) {
					//if this is a space tile adjacent to a non-space tile, replace the space flag with a cliff one
					mf.Flags &= ~(tile_flag::space);
					mf.Flags |= tile_flag::space_cliff;
				}
			}
			
			if (adjacent_terrain && found_transition) {
				for (size_t i = 0; i != iterator->second.size(); ++i) {
					vector::remove(adjacent_terrain_directions[terrain_type::get_all().size()], iterator->second[i]);
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
				if (vector::contains(mf.OverlayTransitionTiles[i + 1].terrain->get_inner_border_terrain_types(), mf.OverlayTransitionTiles[i].terrain)) {
					tile_transition temp_transition = mf.OverlayTransitionTiles[i];
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
				if (vector::contains(mf.TransitionTiles[i + 1].terrain->get_inner_border_terrain_types(), mf.TransitionTiles[i].terrain)) {
					tile_transition temp_transition = mf.TransitionTiles[i];
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
	if (!this->Info->IsPointOnMap(pos, z)) {
		return;
	}
	
	if (CEditor::get()->is_running()) { //no need to assign landmasses while in the editor
		return;
	}
	
	wyrmgus::tile &mf = *this->Field(pos, z);

	if (mf.get_landmass() != nullptr) {
		return; //already calculated
	}
	
	const bool is_space = mf.has_flag(tile_flag::space);

	if (is_space) {
		return; //no landmass for space tiles
	}

	const bool is_water = mf.has_flag(tile_flag::water_allowed) || mf.has_flag(tile_flag::coast_allowed);
	const bool is_space_cliff = mf.has_flag(tile_flag::space_cliff);

	//doesn't have a landmass, and hasn't inherited one from another tile, so add a new one
	const size_t landmass_index = this->landmasses.size();
	const world *landmass_world = this->calculate_pos_world(pos, z, is_space_cliff);
	this->landmasses.push_back(std::make_unique<landmass>(landmass_index, landmass_world));
	mf.set_landmass(this->landmasses.back().get());

	//now, spread the new landmass to neighboring land tiles
	std::vector<Vec2i> landmass_tiles;
	landmass_tiles.push_back(pos);

	for (size_t i = 0; i < landmass_tiles.size(); ++i) {
		for (int x_offset = -1; x_offset <= 1; ++x_offset) {
			for (int y_offset = -1; y_offset <= 1; ++y_offset) {
				if (x_offset != 0 || y_offset != 0) {
					Vec2i adjacent_pos(landmass_tiles[i].x + x_offset, landmass_tiles[i].y + y_offset);

					if (this->Info->IsPointOnMap(adjacent_pos, z)) {
						wyrmgus::tile &adjacent_mf = *this->Field(adjacent_pos, z);

						const bool adjacent_is_space = adjacent_mf.has_flag(tile_flag::space);

						if (adjacent_is_space) {
							continue;
						}

						const bool adjacent_is_water = adjacent_mf.has_flag(tile_flag::water_allowed) || adjacent_mf.has_flag(tile_flag::coast_allowed);
						const bool adjacent_is_space_cliff = adjacent_mf.has_flag(tile_flag::space_cliff);
						const bool adjacent_is_compatible = (adjacent_is_water == is_water) && (adjacent_is_space_cliff == is_space_cliff);
									
						if (adjacent_is_compatible) {
							if (adjacent_mf.get_landmass() == nullptr) {
								adjacent_mf.set_landmass(mf.get_landmass());
								landmass_tiles.push_back(adjacent_pos);
							}
						} else {
							if (adjacent_mf.get_landmass() != nullptr && !mf.get_landmass()->borders_landmass(adjacent_mf.get_landmass())) {
								mf.get_landmass()->add_border_landmass(adjacent_mf.get_landmass());
								adjacent_mf.get_landmass()->add_border_landmass(mf.get_landmass());
							}
						}
					}
				}
			}
		}
	}
}

void CMap::CalculateTileOwnershipTransition(const Vec2i &pos, int z)
{
	if (!this->Info->IsPointOnMap(pos, z)) {
		return;
	}
	
	if (CEditor::get()->is_running()) { //no need to assign ownership transitions while in the editor
		return;
	}
	
	wyrmgus::tile &mf = *this->Field(pos, z);
	
	mf.set_ownership_border_tile(-1);

	if (mf.get_owner() == nullptr) {
		return;
	}
	
	std::vector<int> adjacent_directions;
	
	for (int x_offset = -1; x_offset <= 1; ++x_offset) {
		for (int y_offset = -1; y_offset <= 1; ++y_offset) {
			if (x_offset != 0 || y_offset != 0) {
				Vec2i adjacent_pos(pos.x + x_offset, pos.y + y_offset);
				if (this->Info->IsPointOnMap(adjacent_pos, z)) {
					wyrmgus::tile &adjacent_mf = *this->Field(adjacent_pos, z);
					if (adjacent_mf.get_owner() != mf.get_owner()) {
						adjacent_directions.push_back(GetDirectionFromOffset(x_offset, y_offset));
					}
				}
			}
		}
	}
	
	const wyrmgus::tile_transition_type transition_type = GetTransitionType(adjacent_directions, true);

	if (transition_type != wyrmgus::tile_transition_type::none) {
		const std::vector<int> &transition_tiles = wyrmgus::defines::get()->get_border_terrain_type()->get_transition_tiles(nullptr, transition_type);
		if (!transition_tiles.empty()) {
			mf.set_ownership_border_tile(wyrmgus::vector::get_random(transition_tiles));
		}
	}
}

void CMap::AdjustMap()
{
	for (size_t z = 0; z < this->MapLayers.size(); ++z) {
		Vec2i map_start_pos(0, 0);
		Vec2i map_end(this->Info->MapWidths[z], this->Info->MapHeights[z]);
		
		this->AdjustTileMapIrregularities(false, map_start_pos, map_end, z);
		this->AdjustTileMapIrregularities(true, map_start_pos, map_end, z);
		this->AdjustTileMapTransitions(map_start_pos, map_end, z);
		this->AdjustTileMapIrregularities(true, map_start_pos, map_end, z);
	}
}

void CMap::AdjustTileMapIrregularities(const bool overlay, const Vec2i &min_pos, const Vec2i &max_pos, const int z)
{
	bool no_irregularities_found = false;
	int try_count = 0;
	static constexpr int max_try_count = 100;

	while (!no_irregularities_found && try_count < max_try_count) {
		no_irregularities_found = true;
		++try_count;

		for (int x = min_pos.x; x < max_pos.x; ++x) {
			for (int y = min_pos.y; y < max_pos.y; ++y) {
				tile &mf = *this->Field(x, y, z);
				const terrain_type *terrain = overlay ? mf.get_overlay_terrain() : mf.get_terrain();
				if (!terrain || terrain->allows_single()) {
					continue;
				}

				std::set<const terrain_type *> acceptable_adjacent_tile_types;
				acceptable_adjacent_tile_types.insert(terrain);
				set::merge(acceptable_adjacent_tile_types, terrain->get_outer_border_terrain_types());
				
				int horizontal_adjacent_tiles = 0;
				int vertical_adjacent_tiles = 0;
				int nw_quadrant_adjacent_tiles = 0; //should be 4 if the wrong tile types are present in X-1,Y; X-1,Y-1; X,Y-1; and X+1,Y+1
				int ne_quadrant_adjacent_tiles = 0;
				int sw_quadrant_adjacent_tiles = 0;
				int se_quadrant_adjacent_tiles = 0;
				
				if ((x - 1) >= 0 && !acceptable_adjacent_tile_types.contains(this->GetTileTerrain(Vec2i(x - 1, y), overlay, z))) {
					horizontal_adjacent_tiles += 1;
					nw_quadrant_adjacent_tiles += 1;
					sw_quadrant_adjacent_tiles += 1;
				}
				if ((x + 1) < this->Info->MapWidths[z] && !acceptable_adjacent_tile_types.contains(this->GetTileTerrain(Vec2i(x + 1, y), overlay, z))) {
					horizontal_adjacent_tiles += 1;
					ne_quadrant_adjacent_tiles += 1;
					se_quadrant_adjacent_tiles += 1;
				}
				
				if ((y - 1) >= 0 && !acceptable_adjacent_tile_types.contains(this->GetTileTerrain(Vec2i(x, y - 1), overlay, z))) {
					vertical_adjacent_tiles += 1;
					nw_quadrant_adjacent_tiles += 1;
					ne_quadrant_adjacent_tiles += 1;
				}
				if ((y + 1) < this->Info->MapHeights[z] && !acceptable_adjacent_tile_types.contains(this->GetTileTerrain(Vec2i(x, y + 1), overlay, z))) {
					vertical_adjacent_tiles += 1;
					sw_quadrant_adjacent_tiles += 1;
					se_quadrant_adjacent_tiles += 1;
				}

				if ((x - 1) >= 0 && (y - 1) >= 0 && !acceptable_adjacent_tile_types.contains(this->GetTileTerrain(Vec2i(x - 1, y - 1), overlay, z))) {
					nw_quadrant_adjacent_tiles += 1;
					se_quadrant_adjacent_tiles += 1;
				}

				if ((x - 1) >= 0 && (y + 1) < this->Info->MapHeights[z] && !acceptable_adjacent_tile_types.contains(GetTileTerrain(Vec2i(x - 1, y + 1), overlay, z))) {
					sw_quadrant_adjacent_tiles += 1;
					ne_quadrant_adjacent_tiles += 1;
				}
				if ((x + 1) < this->Info->MapWidths[z] && (y - 1) >= 0 && !acceptable_adjacent_tile_types.contains(GetTileTerrain(Vec2i(x + 1, y - 1), overlay, z))) {
					ne_quadrant_adjacent_tiles += 1;
					sw_quadrant_adjacent_tiles += 1;
				}
				if ((x + 1) < this->Info->MapWidths[z] && (y + 1) < this->Info->MapHeights[z] && !acceptable_adjacent_tile_types.contains(GetTileTerrain(Vec2i(x + 1, y + 1), overlay, z))) {
					se_quadrant_adjacent_tiles += 1;
					nw_quadrant_adjacent_tiles += 1;
				}
				
				if (horizontal_adjacent_tiles >= 2 || vertical_adjacent_tiles >= 2 || nw_quadrant_adjacent_tiles >= 4 || ne_quadrant_adjacent_tiles >= 4 || sw_quadrant_adjacent_tiles >= 4 || se_quadrant_adjacent_tiles >= 4) {
					if (overlay) {
						mf.RemoveOverlayTerrain();
					} else {
						std::map<const wyrmgus::terrain_type *, int> best_terrain_scores;

						for (int sub_x = -1; sub_x <= 1; ++sub_x) {
							for (int sub_y = -1; sub_y <= 1; ++sub_y) {
								if ((x + sub_x) < min_pos.x || (x + sub_x) >= max_pos.x || (y + sub_y) < min_pos.y || (y + sub_y) >= max_pos.y || (sub_x == 0 && sub_y == 0)) {
									continue;
								}
								const wyrmgus::terrain_type *tile_terrain = GetTileTerrain(Vec2i(x + sub_x, y + sub_y), false, z);
								if (mf.get_terrain() != tile_terrain) {
									best_terrain_scores[tile_terrain]++;
								}
							}
						}

						const wyrmgus::terrain_type *best_terrain = nullptr;
						int best_score = 0;
						for (const auto &score_pair : best_terrain_scores) {
							const int score = score_pair.second;
							if (score > best_score) {
								best_score = score;
								best_terrain = score_pair.first;
							}
						}

						mf.SetTerrain(best_terrain);
					}
					no_irregularities_found = false;
				}
			}
		}
	}
}

void CMap::AdjustTileMapTransitions(const Vec2i &min_pos, const Vec2i &max_pos, int z)
{
	bool tile_changed = true;
	int try_count = 0;
	static constexpr int max_try_count = 100;

	while (tile_changed && try_count < max_try_count) {
		tile_changed = false;
		++try_count;

		for (int x = min_pos.x; x < max_pos.x; ++x) {
			for (int y = min_pos.y; y < max_pos.y; ++y) {
				wyrmgus::tile &mf = *this->Field(x, y, z);

				if (mf.get_terrain() == nullptr) {
					continue;
				}

				for (int sub_x = -1; sub_x <= 1; ++sub_x) {
					for (int sub_y = -1; sub_y <= 1; ++sub_y) {
						if ((x + sub_x) < min_pos.x || (x + sub_x) >= max_pos.x || (y + sub_y) < min_pos.y || (y + sub_y) >= max_pos.y || (sub_x == 0 && sub_y == 0)) {
							continue;
						}

						const wyrmgus::terrain_type *tile_terrain = GetTileTerrain(Vec2i(x + sub_x, y + sub_y), false, z);
						const wyrmgus::terrain_type *tile_top_terrain = GetTileTopTerrain(Vec2i(x + sub_x, y + sub_y), false, z);

						if (tile_terrain == nullptr) {
							continue;
						}

						if (
							mf.get_terrain() != tile_terrain
							&& tile_top_terrain->is_overlay()
							&& tile_top_terrain != mf.get_overlay_terrain()
							&& !vector::contains(tile_terrain->get_outer_border_terrain_types(), mf.get_terrain())
							&& !vector::contains(tile_top_terrain->get_base_terrain_types(), mf.get_terrain())
						) {
							mf.SetTerrain(tile_terrain);
							tile_changed = true;
						}
					}
				}
			}
		}

		for (int x = min_pos.x; x < max_pos.x; ++x) {
			for (int y = min_pos.y; y < max_pos.y; ++y) {
				wyrmgus::tile &mf = *this->Field(x, y, z);

				if (mf.get_terrain() == nullptr) {
					continue;
				}

				for (int sub_x = -1; sub_x <= 1; ++sub_x) {
					for (int sub_y = -1; sub_y <= 1; ++sub_y) {
						if ((x + sub_x) < min_pos.x || (x + sub_x) >= max_pos.x || (y + sub_y) < min_pos.y || (y + sub_y) >= max_pos.y || (sub_x == 0 && sub_y == 0)) {
							continue;
						}

						const terrain_type *tile_terrain = GetTileTerrain(Vec2i(x + sub_x, y + sub_y), false, z);

						if (tile_terrain == nullptr) {
							continue;
						}

						if (mf.get_terrain() != tile_terrain && !mf.get_terrain()->is_border_terrain_type(tile_terrain)) {
							const terrain_type *intermediate_terrain = mf.get_terrain()->get_intermediate_terrain_type(tile_terrain);
							if (intermediate_terrain != nullptr) {
								mf.SetTerrain(intermediate_terrain);
								tile_changed = true;
							}
						}
					}
				}
			}
		}
	}
}

void CMap::adjust_territory_irregularities(const QPoint &min_pos, const QPoint &max_pos, const int z)
{
	bool no_irregularities_found = false;
	int try_count = 0;
	static constexpr int max_try_count = 100;

	while (!no_irregularities_found && try_count < max_try_count) {
		no_irregularities_found = true;
		++try_count;

		for (int x = min_pos.x(); x <= max_pos.x(); ++x) {
			for (int y = min_pos.y(); y <= max_pos.y(); ++y) {
				const QPoint tile_pos(x, y);
				wyrmgus::tile *tile = this->Field(tile_pos, z);
				const wyrmgus::site *settlement = tile->get_settlement();

				if (settlement == nullptr) {
					continue;
				}

				const wyrmgus::site_game_data *settlement_game_data = settlement->get_game_data();

				if (settlement_game_data->get_site_unit() == nullptr) {
					tile->set_settlement(nullptr);
					no_irregularities_found = false;
					continue;
				}

				if (!this->tile_borders_same_settlement_territory(tile_pos, z, false)) {
					tile->set_settlement(nullptr);
					no_irregularities_found = false;
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
void CMap::GenerateTerrain(const std::unique_ptr<wyrmgus::generated_terrain> &generated_terrain, const Vec2i &min_pos, const Vec2i &max_pos, const bool preserve_coastline, const int z)
{
	if (SaveGameLoading) {
		return;
	}
	
	wyrmgus::terrain_type *terrain_type = generated_terrain->TerrainType;
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
				const wyrmgus::tile *tile = this->Field(x, y, z);
				
				if (max_tile_quantity != 0 && tile->get_top_terrain() == terrain_type) {
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
		for (const auto &kv_pair : this->MapLayers[z]->subtemplate_areas) {
			const QRect &subtemplate_rect = kv_pair.second;

			const QPoint subtemplate_min_pos = subtemplate_rect.topLeft();
			const QPoint subtemplate_max_pos = subtemplate_rect.bottomRight();
			
			for (int x = subtemplate_min_pos.x(); x <= subtemplate_max_pos.x(); ++x) {
				for (int y = subtemplate_min_pos.y(); y <= subtemplate_max_pos.y(); ++y) {
					const Vec2i tile_pos(x, y);
					const wyrmgus::tile *tile = this->Field(x, y, z);
					
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
		
		random_pos = vector::take_random(potential_positions);
		
		if (!this->Info->IsPointOnMap(random_pos, z) || this->is_point_in_a_subtemplate_area(random_pos, z)) {
			continue;
		}
		
		const wyrmgus::terrain_type *tile_terrain = this->GetTileTerrain(random_pos, false, z);
		
		if (!generated_terrain->CanGenerateOnTile(this->Field(random_pos, z))) {
			continue;
		}
		
		if (
			(
				(
					!terrain_type->is_overlay()
					&& ((tile_terrain == terrain_type && GetTileTopTerrain(random_pos, false, z)->is_overlay()) || (terrain_type->is_border_terrain_type(tile_terrain) && this->TileBordersOnlySameTerrain(random_pos, terrain_type, z)))
				)
				|| (
					terrain_type->is_overlay()
					&& wyrmgus::vector::contains(terrain_type->get_base_terrain_types(), tile_terrain) && this->TileBordersOnlySameTerrain(random_pos, terrain_type, z)
					&& (!GetTileTopTerrain(random_pos, false, z)->is_overlay() || GetTileTopTerrain(random_pos, false, z) == terrain_type)
				)
			)
			&& (!preserve_coastline || terrain_type->has_flag(tile_flag::water_allowed) == tile_terrain->has_flag(tile_flag::water_allowed))
			&& !this->TileHasUnitsIncompatibleWithTerrain(random_pos, terrain_type, z)
			&& (!terrain_type->has_flag(tile_flag::impassable) || !this->TileBordersUnit(random_pos, z)) // if the terrain is unpassable, don't expand to spots adjacent to units
		) {
			std::vector<Vec2i> adjacent_positions;
			for (int sub_x = -1; sub_x <= 1; sub_x += 2) { // +2 so that only diagonals are used
				for (int sub_y = -1; sub_y <= 1; sub_y += 2) {
					Vec2i diagonal_pos(random_pos.x + sub_x, random_pos.y + sub_y);
					Vec2i vertical_pos(random_pos.x, random_pos.y + sub_y);
					Vec2i horizontal_pos(random_pos.x + sub_x, random_pos.y);
					if (!this->Info->IsPointOnMap(diagonal_pos, z)) {
						continue;
					}
					
					const wyrmgus::terrain_type *diagonal_tile_terrain = this->GetTileTerrain(diagonal_pos, false, z);
					const wyrmgus::terrain_type *vertical_tile_terrain = this->GetTileTerrain(vertical_pos, false, z);
					const wyrmgus::terrain_type *horizontal_tile_terrain = this->GetTileTerrain(horizontal_pos, false, z);
					
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
								!terrain_type->is_overlay()
								&& ((diagonal_tile_terrain == terrain_type && GetTileTopTerrain(diagonal_pos, false, z)->is_overlay()) || (terrain_type->is_border_terrain_type(diagonal_tile_terrain) && this->TileBordersOnlySameTerrain(diagonal_pos, terrain_type, z)))
								&& ((vertical_tile_terrain == terrain_type && GetTileTopTerrain(vertical_pos, false, z)->is_overlay()) || (terrain_type->is_border_terrain_type(vertical_tile_terrain) && this->TileBordersOnlySameTerrain(vertical_pos, terrain_type, z)))
								&& ((horizontal_tile_terrain == terrain_type && GetTileTopTerrain(horizontal_pos, false, z)->is_overlay()) || (terrain_type->is_border_terrain_type(horizontal_tile_terrain) && this->TileBordersOnlySameTerrain(horizontal_pos, terrain_type, z)))
							)
							|| (
								terrain_type->is_overlay()
								&& wyrmgus::vector::contains(terrain_type->get_base_terrain_types(), diagonal_tile_terrain) && this->TileBordersOnlySameTerrain(diagonal_pos, terrain_type, z)
								&& wyrmgus::vector::contains(terrain_type->get_base_terrain_types(), vertical_tile_terrain) && this->TileBordersOnlySameTerrain(vertical_pos, terrain_type, z)
								&& wyrmgus::vector::contains(terrain_type->get_base_terrain_types(), horizontal_tile_terrain) && this->TileBordersOnlySameTerrain(horizontal_pos, terrain_type, z)
								&& (!GetTileTopTerrain(diagonal_pos, false, z)->is_overlay() || GetTileTopTerrain(diagonal_pos, false, z) == terrain_type) && (!GetTileTopTerrain(vertical_pos, false, z)->is_overlay() || GetTileTopTerrain(vertical_pos, false, z) == terrain_type) && (!GetTileTopTerrain(horizontal_pos, false, z)->is_overlay() || GetTileTopTerrain(horizontal_pos, false, z) == terrain_type)
							)
						)
						&& (!preserve_coastline || (terrain_type->has_flag(tile_flag::water_allowed) == diagonal_tile_terrain->has_flag(tile_flag::water_allowed) && terrain_type->has_flag(tile_flag::water_allowed) == vertical_tile_terrain->has_flag(tile_flag::water_allowed) && terrain_type->has_flag(tile_flag::water_allowed) == horizontal_tile_terrain->has_flag(tile_flag::water_allowed)))
						&& !this->TileHasUnitsIncompatibleWithTerrain(diagonal_pos, terrain_type, z) && !this->TileHasUnitsIncompatibleWithTerrain(vertical_pos, terrain_type, z) && !this->TileHasUnitsIncompatibleWithTerrain(horizontal_pos, terrain_type, z)
						&& (!terrain_type->has_flag(tile_flag::impassable) || (!this->TileBordersUnit(diagonal_pos, z) && !this->TileBordersUnit(vertical_pos, z) && !this->TileBordersUnit(horizontal_pos, z))) // if the terrain is unpassable, don't expand to spots adjacent to buildings
						&& !this->is_point_in_a_subtemplate_area(diagonal_pos, z) && !this->is_point_in_a_subtemplate_area(vertical_pos, z) && !this->is_point_in_a_subtemplate_area(horizontal_pos, z)
					) {
						adjacent_positions.push_back(diagonal_pos);
					}
				}
			}
			
			if (adjacent_positions.size() > 0) {
				Vec2i adjacent_pos = adjacent_positions[SyncRand(adjacent_positions.size())];
				if (!terrain_type->is_overlay()) {
					if (generated_terrain->CanRemoveTileOverlayTerrain(this->Field(random_pos, z))) {
						this->Field(random_pos, z)->RemoveOverlayTerrain();
					}
					if (generated_terrain->CanRemoveTileOverlayTerrain(this->Field(adjacent_pos, z))) {
						this->Field(adjacent_pos, z)->RemoveOverlayTerrain();
					}
					if (generated_terrain->CanRemoveTileOverlayTerrain(this->Field(Vec2i(random_pos.x, adjacent_pos.y), z))) {
						this->Field(Vec2i(random_pos.x, adjacent_pos.y), z)->RemoveOverlayTerrain();
					}
					if (generated_terrain->CanRemoveTileOverlayTerrain(this->Field(Vec2i(adjacent_pos.x, random_pos.y), z))) {
						this->Field(Vec2i(adjacent_pos.x, random_pos.y), z)->RemoveOverlayTerrain();
					}
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
				if (!this->Info->IsPointOnMap(diagonal_pos, z) || diagonal_pos.x < min_pos.x || diagonal_pos.y < min_pos.y || diagonal_pos.x > max_pos.x || diagonal_pos.y > max_pos.y) {
					continue;
				}

				if ( //must either be able to generate on the tiles, or they must already have the generated terrain type
					!generated_terrain->CanTileBePartOfExpansion(this->Field(diagonal_pos, z))
					|| !generated_terrain->CanTileBePartOfExpansion(this->Field(vertical_pos, z))
					|| !generated_terrain->CanTileBePartOfExpansion(this->Field(horizontal_pos, z))
				) {
					continue;
				}
		
				const wyrmgus::terrain_type *diagonal_tile_terrain = this->GetTileTerrain(diagonal_pos, false, z);
				const wyrmgus::terrain_type *vertical_tile_terrain = this->GetTileTerrain(vertical_pos, false, z);
				const wyrmgus::terrain_type *horizontal_tile_terrain = this->GetTileTerrain(horizontal_pos, false, z);
				const wyrmgus::terrain_type *diagonal_tile_top_terrain = this->GetTileTopTerrain(diagonal_pos, false, z);
				const wyrmgus::terrain_type *vertical_tile_top_terrain = this->GetTileTopTerrain(vertical_pos, false, z);
				const wyrmgus::terrain_type *horizontal_tile_top_terrain = this->GetTileTopTerrain(horizontal_pos, false, z);
				
				if (!terrain_type->is_overlay()) {
					if (diagonal_tile_terrain != terrain_type && (!terrain_type->is_border_terrain_type(diagonal_tile_terrain) || this->TileBordersTerrainIncompatibleWithTerrain(diagonal_pos, terrain_type, z))) {
						continue;
					}
					if (vertical_tile_terrain != terrain_type && (!terrain_type->is_border_terrain_type(vertical_tile_terrain) || this->TileBordersTerrainIncompatibleWithTerrain(vertical_pos, terrain_type, z))) {
						continue;
					}
					if (horizontal_tile_terrain != terrain_type && (!terrain_type->is_border_terrain_type(horizontal_tile_terrain) || this->TileBordersTerrainIncompatibleWithTerrain(horizontal_pos, terrain_type, z))) {
						continue;
					}
				} else {
					if ((!wyrmgus::vector::contains(terrain_type->get_base_terrain_types(), diagonal_tile_terrain) || this->TileBordersTerrainIncompatibleWithTerrain(diagonal_pos, terrain_type, z)) && GetTileTerrain(diagonal_pos, terrain_type->is_overlay(), z) != terrain_type) {
						continue;
					}
					if ((!wyrmgus::vector::contains(terrain_type->get_base_terrain_types(), vertical_tile_terrain) || this->TileBordersTerrainIncompatibleWithTerrain(vertical_pos, terrain_type, z)) && GetTileTerrain(vertical_pos, terrain_type->is_overlay(), z) != terrain_type) {
						continue;
					}
					if ((!wyrmgus::vector::contains(terrain_type->get_base_terrain_types(), horizontal_tile_terrain) || this->TileBordersTerrainIncompatibleWithTerrain(horizontal_pos, terrain_type, z)) && GetTileTerrain(horizontal_pos, terrain_type->is_overlay(), z) != terrain_type) {
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
						terrain_type->has_flag(tile_flag::water_allowed) != diagonal_tile_terrain->has_flag(tile_flag::water_allowed)
						|| terrain_type->has_flag(tile_flag::water_allowed) != vertical_tile_terrain->has_flag(tile_flag::water_allowed)
						|| terrain_type->has_flag(tile_flag::water_allowed) != horizontal_tile_terrain->has_flag(tile_flag::water_allowed)
					)
				) {
					continue;
				}
				
				if (this->TileHasUnitsIncompatibleWithTerrain(diagonal_pos, terrain_type, z) || this->TileHasUnitsIncompatibleWithTerrain(vertical_pos, terrain_type, z) || this->TileHasUnitsIncompatibleWithTerrain(horizontal_pos, terrain_type, z)) {
					continue;
				}
				
				if ( // if the terrain is unpassable, don't expand to spots adjacent to buildings
					terrain_type->has_flag(tile_flag::impassable) && (this->TileBordersUnit(diagonal_pos, z) || this->TileBordersUnit(vertical_pos, z) || this->TileBordersUnit(horizontal_pos, z))
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
			
			if (!this->is_point_in_a_subtemplate_area(adjacent_pos, z) && this->GetTileTopTerrain(adjacent_pos, false, z) != terrain_type && (this->GetTileTerrain(adjacent_pos, terrain_type->is_overlay(), z) != terrain_type || generated_terrain->CanRemoveTileOverlayTerrain(this->Field(adjacent_pos, z)))) {
				if (!terrain_type->is_overlay() && generated_terrain->CanRemoveTileOverlayTerrain(this->Field(adjacent_pos, z))) {
					this->Field(adjacent_pos, z)->RemoveOverlayTerrain();
				}

				if (this->GetTileTerrain(adjacent_pos, terrain_type->is_overlay(), z) != terrain_type) {
					this->Field(adjacent_pos, z)->SetTerrain(terrain_type);
				}
				
				seeds.push_back(adjacent_pos);
				
				if (this->GetTileTopTerrain(adjacent_pos, false, z) == terrain_type) {
					tile_quantity++;
				}
			}
			
			if (!this->is_point_in_a_subtemplate_area(adjacent_pos_horizontal, z) && this->GetTileTopTerrain(adjacent_pos_horizontal, false, z) != terrain_type && (this->GetTileTerrain(adjacent_pos_horizontal, terrain_type->is_overlay(), z) != terrain_type || generated_terrain->CanRemoveTileOverlayTerrain(this->Field(adjacent_pos_horizontal, z)))) {
				if (!terrain_type->is_overlay() && generated_terrain->CanRemoveTileOverlayTerrain(this->Field(adjacent_pos_horizontal, z))) {
					this->Field(adjacent_pos_horizontal, z)->RemoveOverlayTerrain();
				}
				
				if (this->GetTileTerrain(adjacent_pos_horizontal, terrain_type->is_overlay(), z) != terrain_type) {
					this->Field(adjacent_pos_horizontal, z)->SetTerrain(terrain_type);
				}
				
				seeds.push_back(adjacent_pos_horizontal);
				
				if (this->GetTileTopTerrain(adjacent_pos_horizontal, false, z) == terrain_type) {
					tile_quantity++;
				}
			}
			
			if (!this->is_point_in_a_subtemplate_area(adjacent_pos_vertical, z) && this->GetTileTopTerrain(adjacent_pos_vertical, false, z) != terrain_type && (this->GetTileTerrain(adjacent_pos_vertical, terrain_type->is_overlay(), z) != terrain_type || generated_terrain->CanRemoveTileOverlayTerrain(this->Field(adjacent_pos_vertical, z)))) {
				if (!terrain_type->is_overlay() && generated_terrain->CanRemoveTileOverlayTerrain(this->Field(adjacent_pos_vertical, z))) {
					this->Field(adjacent_pos_vertical, z)->RemoveOverlayTerrain();
				}
				
				if (this->GetTileTerrain(adjacent_pos_vertical, terrain_type->is_overlay(), z) != terrain_type) {
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

bool CMap::CanTileBePartOfMissingTerrainGeneration(const wyrmgus::tile *tile, const wyrmgus::terrain_type *terrain_type, const wyrmgus::terrain_type *overlay_terrain_type) const
{
	if (tile->get_top_terrain() == nullptr) {
		return true;
	}

	if (tile->get_terrain() == terrain_type && (tile->get_overlay_terrain() == overlay_terrain_type || overlay_terrain_type == nullptr)) {
		return true;
	}

	return false;
}

void CMap::generate_missing_terrain(const QRect &rect, const int z)
{
	if (SaveGameLoading) {
		return;
	}

	std::vector<QPoint> seeds;

	//use tiles that have a terrain as seeds for the terrain generation
	bool has_tile_with_missing_terrain = false;

	rect::for_each_point(rect, [&](QPoint &&tile_pos) {
		const wyrmgus::tile *tile = this->Field(tile_pos, z);
		const wyrmgus::terrain_type *top_terrain = tile->get_top_terrain();

		if (top_terrain == nullptr) {
			has_tile_with_missing_terrain = true;
			return;
		}

		if (!this->TileBordersTerrain(tile_pos, nullptr, z)) {
			return; //the seed must border a tile with null terrain
		}

		if (top_terrain->has_flag(tile_flag::space)) {
			//space tiles cannot be used as seeds, or else missing terrain generation wouldn't work properly for world terrain circles
			return;
		}

		seeds.push_back(std::move(tile_pos));
	});

	if (!has_tile_with_missing_terrain) {
		return;
	}

	const QPoint min_pos = rect.topLeft();
	const QPoint max_pos = rect.bottomRight();

	//expand seeds
	vector::process_randomly(seeds, [&](const QPoint &seed_pos) {
		const wyrmgus::tile *seed_tile = this->Field(seed_pos, z);

		const wyrmgus::terrain_type *terrain_type = seed_tile->get_terrain();
		const wyrmgus::terrain_type *overlay_terrain_type = seed_tile->get_overlay_terrain();
		const wyrmgus::terrain_feature *terrain_feature = seed_tile->get_terrain_feature();

		if (overlay_terrain_type != nullptr && overlay_terrain_type->is_constructed()) {
			overlay_terrain_type = nullptr; //don't expand overlay terrain to tiles with empty terrain if the overlay is a constructed one
		}

		const std::vector<QPoint> adjacent_positions = point::get_diagonally_adjacent_if(seed_pos, [&](const QPoint &diagonal_pos) {
			const QPoint vertical_pos(seed_pos.x(), diagonal_pos.y());
			const QPoint horizontal_pos(diagonal_pos.x(), seed_pos.y());

			if (!this->Info->IsPointOnMap(diagonal_pos, z) || diagonal_pos.x() < min_pos.x() || diagonal_pos.y() < min_pos.y() || diagonal_pos.x() > max_pos.x() || diagonal_pos.y() > max_pos.y()) {
				return false;
			}

			if ( //must either be able to generate on the tiles, or they must already have the generated terrain type
				!this->CanTileBePartOfMissingTerrainGeneration(this->Field(diagonal_pos, z), terrain_type, overlay_terrain_type)
				|| !this->CanTileBePartOfMissingTerrainGeneration(this->Field(vertical_pos, z), terrain_type, overlay_terrain_type)
				|| !this->CanTileBePartOfMissingTerrainGeneration(this->Field(horizontal_pos, z), terrain_type, overlay_terrain_type)
			) {
				return false;
			}

			const wyrmgus::terrain_type *diagonal_tile_top_terrain = this->GetTileTopTerrain(diagonal_pos, false, z);
			const wyrmgus::terrain_type *vertical_tile_top_terrain = this->GetTileTopTerrain(vertical_pos, false, z);
			const wyrmgus::terrain_type *horizontal_tile_top_terrain = this->GetTileTopTerrain(horizontal_pos, false, z);

			if (diagonal_tile_top_terrain == nullptr && this->TileBordersTerrainIncompatibleWithTerrainPair(diagonal_pos, terrain_type, overlay_terrain_type, z)) {
				return false;
			}
			if (vertical_tile_top_terrain == nullptr && this->TileBordersTerrainIncompatibleWithTerrainPair(vertical_pos, terrain_type, overlay_terrain_type, z)) {
				return false;
			}
			if (horizontal_tile_top_terrain == nullptr && this->TileBordersTerrainIncompatibleWithTerrainPair(horizontal_pos, terrain_type, overlay_terrain_type, z)) {
				return false;
			}

			if (diagonal_tile_top_terrain != nullptr && vertical_tile_top_terrain != nullptr && horizontal_tile_top_terrain != nullptr) { //at least one of the tiles being expanded to must have null terrain
				return false;
			}

			if (overlay_terrain_type != nullptr) {
				if (this->TileHasUnitsIncompatibleWithTerrain(diagonal_pos, overlay_terrain_type, z) || this->TileHasUnitsIncompatibleWithTerrain(vertical_pos, overlay_terrain_type, z) || this->TileHasUnitsIncompatibleWithTerrain(horizontal_pos, overlay_terrain_type, z)) {
					return false;
				}
			}

			return true;
		});

		if (adjacent_positions.size() > 0) {
			if (adjacent_positions.size() > 1) {
				seeds.push_back(seed_pos); //push the seed back again for another try, since it may be able to generate further terrain in the future
			}

			const QPoint &adjacent_pos = vector::get_random(adjacent_positions);
			const QPoint adjacent_pos_horizontal(adjacent_pos.x(), seed_pos.y());
			const QPoint adjacent_pos_vertical(seed_pos.x(), adjacent_pos.y());

			if (this->GetTileTopTerrain(adjacent_pos, false, z) == nullptr) {
				this->Field(adjacent_pos, z)->SetTerrain(terrain_type);
				if (overlay_terrain_type != nullptr) {
					this->Field(adjacent_pos, z)->SetTerrain(overlay_terrain_type);
				}
				if (terrain_feature != nullptr) {
					this->Field(adjacent_pos, z)->set_terrain_feature(terrain_feature);
				}
				seeds.push_back(adjacent_pos);
			}

			if (this->GetTileTopTerrain(adjacent_pos_horizontal, false, z) == nullptr) {
				this->Field(adjacent_pos_horizontal, z)->SetTerrain(terrain_type);
				if (overlay_terrain_type != nullptr) {
					this->Field(adjacent_pos_horizontal, z)->SetTerrain(overlay_terrain_type);
				}
				if (terrain_feature != nullptr) {
					this->Field(adjacent_pos_horizontal, z)->set_terrain_feature(terrain_feature);
				}
				seeds.push_back(adjacent_pos_horizontal);
			}

			if (this->GetTileTopTerrain(adjacent_pos_vertical, false, z) == nullptr) {
				this->Field(adjacent_pos_vertical, z)->SetTerrain(terrain_type);
				if (overlay_terrain_type != nullptr) {
					this->Field(adjacent_pos_vertical, z)->SetTerrain(overlay_terrain_type);
				}
				if (terrain_feature != nullptr) {
					this->Field(adjacent_pos_vertical, z)->set_terrain_feature(terrain_feature);
				}
				seeds.push_back(adjacent_pos_vertical);
			}
		}
	});

	//set the terrain of the remaining tiles without any to their most-neighbored terrain/overlay terrain pair
	rect::for_each_point(rect, [&](const QPoint &tile_pos) {
		wyrmgus::tile *tile = this->Field(tile_pos, z);

		if (tile->get_top_terrain() != nullptr) {
			return;
		}

		std::map<std::pair<const terrain_type *, const terrain_type *>, int> terrain_type_pair_neighbor_count;

		for (int x_offset = -1; x_offset <= 1; ++x_offset) {
			for (int y_offset = -1; y_offset <= 1; ++y_offset) {
				if (x_offset == 0 && y_offset == 0) {
					continue;
				}

				const QPoint adjacent_pos(tile_pos.x() + x_offset, tile_pos.y() + y_offset);

				if (!this->Info->IsPointOnMap(adjacent_pos, z)) {
					continue;
				}

				const wyrmgus::tile *adjacent_tile = this->Field(adjacent_pos, z);
				const wyrmgus::terrain_type *adjacent_terrain_type = adjacent_tile->get_terrain();
				const wyrmgus::terrain_type *adjacent_overlay_terrain_type = adjacent_tile->get_overlay_terrain();

				if (adjacent_terrain_type == nullptr) {
					continue;
				}

				const wyrmgus::terrain_type *adjacent_top_terrain_type = adjacent_tile->get_top_terrain();
				if (adjacent_top_terrain_type->has_flag(tile_flag::space)) {
					//space tiles cannot be used as seeds, or else missing terrain generation wouldn't work properly for world terrain circles
					continue;
				}

				std::pair<const terrain_type *, const terrain_type *> terrain_type_pair(adjacent_terrain_type, adjacent_overlay_terrain_type);

				auto find_iterator = terrain_type_pair_neighbor_count.find(terrain_type_pair);
				if (find_iterator == terrain_type_pair_neighbor_count.end()) {
					terrain_type_pair_neighbor_count[terrain_type_pair] = 1;
				} else {
					find_iterator->second++;
				}
			}
		}

		std::pair<const terrain_type *, const terrain_type *> best_terrain_type_pair(nullptr, nullptr);
		int best_terrain_type_neighbor_count = 0;
		for (const auto &element : terrain_type_pair_neighbor_count) {
			if (element.second > best_terrain_type_neighbor_count) {
				best_terrain_type_pair = element.first;
				best_terrain_type_neighbor_count = element.second;
			}
		}

		//set the terrain and overlay terrain to the same as the most-neighbored one
		tile->SetTerrain(best_terrain_type_pair.first);

		if (best_terrain_type_pair.second != nullptr) {
			tile->SetTerrain(best_terrain_type_pair.second);
		}
	});
}

void CMap::expand_terrain_features_to_same_terrain(const int z)
{
	if (CEditor::get()->is_running()) { //no need to assign terrain features while in the editor
		return;
	}

	//expand terrain features to neighboring tiles with the same terrain
	const QRect rect = this->get_rect(z);

	std::vector<QPoint> seeds = wyrmgus::rect::find_points_if(rect, [&](const QPoint &tile_pos) {
		const wyrmgus::tile *tile = this->Field(tile_pos, z);

		const wyrmgus::terrain_feature *terrain_feature = tile->get_terrain_feature();
		if (terrain_feature == nullptr) {
			return false;
		}

		if (!this->tile_borders_other_terrain_feature(tile_pos, z)) {
			return false;
		}

		return true;
	});

	//expand seeds
	wyrmgus::vector::process_randomly(seeds, [&](const QPoint &seed_pos) {
		const wyrmgus::tile *seed_tile = this->Field(seed_pos, z);

		const wyrmgus::terrain_feature *terrain_feature = seed_tile->get_terrain_feature();

		const std::vector<QPoint> adjacent_positions = wyrmgus::point::get_adjacent_if(seed_pos, [&](const QPoint &adjacent_pos) {
			if (!this->Info->IsPointOnMap(adjacent_pos, z)) {
				return false;
			}

			const wyrmgus::tile *adjacent_tile = this->Field(adjacent_pos, z);

			if (adjacent_tile->get_top_terrain() != terrain_feature->get_terrain_type()) {
				return false;
			}

			if (adjacent_tile->get_terrain_feature() != nullptr) {
				return false;
			}

			return true;
		});

		if (adjacent_positions.size() > 0) {
			if (adjacent_positions.size() > 1) {
				//push the seed back again for another try, since it may be able to further expand the terrain feature in the future
				seeds.push_back(seed_pos);
			}

			QPoint adjacent_pos = wyrmgus::vector::get_random(adjacent_positions);
			this->Field(adjacent_pos, z)->set_terrain_feature(terrain_feature);
			seeds.push_back(std::move(adjacent_pos));
		}
	});
}

void CMap::generate_settlement_territories(const int z)
{
	if (SaveGameLoading) {
		return;
	}

	const QRect rect = this->get_rect(z);

	point_set seeds = rect::find_point_set_if(rect, [&](const QPoint &tile_pos) {
		const wyrmgus::tile *tile = this->Field(tile_pos, z);

		const wyrmgus::site *settlement = tile->get_settlement();
		if (settlement == nullptr) {
			return false;
		}

		if (!this->tile_borders_other_settlement_territory(tile_pos, z)) {
			return false;
		}

		return true;
	});

	seeds = this->expand_settlement_territories(container::to_vector(seeds), z, (tile_flag::impassable | tile_flag::coast_allowed | tile_flag::space | tile_flag::space_cliff), tile_flag::water_allowed | tile_flag::underground);
	seeds = this->expand_settlement_territories(container::to_vector(seeds), z, (tile_flag::coast_allowed | tile_flag::space), tile_flag::water_allowed | tile_flag::underground);
	seeds = this->expand_settlement_territories(container::to_vector(seeds), z, tile_flag::space, tile_flag::underground);
	seeds = this->expand_settlement_territories(container::to_vector(seeds), z, tile_flag::space, tile_flag::none);
	this->expand_settlement_territories(container::to_vector(seeds), z, tile_flag::none, tile_flag::none);

	//set the settlement of the remaining tiles without any to their most-neighbored settlement
	rect::for_each_point(rect, [&](const QPoint &tile_pos) {
		wyrmgus::tile *tile = this->Field(tile_pos, z);

		if (tile->get_settlement() != nullptr) {
			return;
		}

		site_map<int> settlement_neighbor_count;

		for (int x_offset = -1; x_offset <= 1; ++x_offset) {
			for (int y_offset = -1; y_offset <= 1; ++y_offset) {
				if (x_offset == 0 && y_offset == 0) {
					continue;
				}

				const QPoint adjacent_pos(tile_pos.x() + x_offset, tile_pos.y() + y_offset);

				if (!this->Info->IsPointOnMap(adjacent_pos, z)) {
					continue;
				}

				const wyrmgus::tile *adjacent_tile = this->Field(adjacent_pos, z);
				const wyrmgus::site *adjacent_settlement = adjacent_tile->get_settlement();

				if (adjacent_settlement == nullptr) {
					continue;
				}

				settlement_neighbor_count[adjacent_settlement]++;
			}
		}

		const wyrmgus::site *best_settlement = nullptr;
		int best_settlement_neighbor_count = 0;
		for (const auto &kv_pair : settlement_neighbor_count) {
			if (kv_pair.second > best_settlement_neighbor_count) {
				best_settlement = kv_pair.first;
				best_settlement_neighbor_count = kv_pair.second;
			}
		}

		//set the settlement to the same as the most-neighbored one
		tile->set_settlement(best_settlement);
	});

	this->process_settlement_territory_tiles(z);

	//update the settlement of all buildings, as settlement territories have changed
	for (const qunique_ptr<CPlayer> &player : CPlayer::Players) {
		if (!player->is_alive() || player->get_index() == PlayerNumNeutral) {
			continue;
		}

		player->update_building_settlement_assignment(nullptr, z);
	}
}

wyrmgus::point_set CMap::expand_settlement_territories(std::vector<QPoint> &&seeds, const int z, const tile_flag block_flags, const tile_flag same_flags)
{
	//the seeds blocked by the block flags are stored, and then returned by the function
	wyrmgus::point_set blocked_seeds;

	//expand seeds
	wyrmgus::vector::process_randomly(seeds, [&](const QPoint &seed_pos) {
		const wyrmgus::tile *seed_tile = this->Field(seed_pos, z);

		//tiles with a block flag can be expanded to, but they can't serve as a basis for further expansion
		if (seed_tile->CheckMask(block_flags)) {
			blocked_seeds.insert(seed_pos);
			return;
		}

		const wyrmgus::site *settlement = seed_tile->get_settlement();
		const wyrmgus::tile *settlement_tile = this->Field(settlement->get_game_data()->get_site_unit()->get_center_tile_pos(), z);

		const std::vector<QPoint> adjacent_positions = wyrmgus::point::get_diagonally_adjacent_if(seed_pos, [&](const QPoint &diagonal_pos) {
			const QPoint vertical_pos(seed_pos.x(), diagonal_pos.y());
			const QPoint horizontal_pos(diagonal_pos.x(), seed_pos.y());

			if (!this->Info->IsPointOnMap(diagonal_pos, z)) {
				return false;
			}

			const wyrmgus::tile *diagonal_tile = this->Field(diagonal_pos, z);
			const wyrmgus::tile *vertical_tile = this->Field(vertical_pos, z);
			const wyrmgus::tile *horizontal_tile = this->Field(horizontal_pos, z);

			if ( //the tiles must either have no settlement, or have the settlement we want to assign
				(diagonal_tile->get_settlement() != nullptr && diagonal_tile->get_settlement() != settlement)
				|| (vertical_tile->get_settlement() != nullptr && vertical_tile->get_settlement() != settlement)
				|| (horizontal_tile->get_settlement() != nullptr && horizontal_tile->get_settlement() != settlement)
			) {
				return false;
			}

			if (diagonal_tile->get_settlement() != nullptr && vertical_tile->get_settlement() != nullptr && horizontal_tile->get_settlement() != nullptr) { //at least one of the tiles being expanded to must have no assigned settlement
				return false;
			}

			//the same flags function similarly to the block flags, but block only if the tile does not contain the same same_flags as the settlement's original tile, and they block expansion to the tile itself, not just expansion from it
			if ((diagonal_tile->Flags & same_flags) != (settlement_tile->Flags & same_flags) || (vertical_tile->Flags & same_flags) != (settlement_tile->Flags & same_flags) || (horizontal_tile->Flags & same_flags) != (settlement_tile->Flags & same_flags)) {
				blocked_seeds.insert(seed_pos);
				return false;
			}

			return true;
		});

		if (adjacent_positions.size() > 0) {
			if (adjacent_positions.size() > 1) {
				seeds.push_back(seed_pos); //push the seed back again for another try, since it may be able to generate further terrain in the future
			}

			QPoint adjacent_pos = adjacent_positions[SyncRand(adjacent_positions.size())];
			QPoint adjacent_pos_horizontal(adjacent_pos.x(), seed_pos.y());
			QPoint adjacent_pos_vertical(seed_pos.x(), adjacent_pos.y());

			this->Field(adjacent_pos, z)->set_settlement(settlement);
			this->Field(adjacent_pos_horizontal, z)->set_settlement(settlement);
			this->Field(adjacent_pos_vertical, z)->set_settlement(settlement);

			seeds.push_back(std::move(adjacent_pos));
			seeds.push_back(std::move(adjacent_pos_horizontal));
			seeds.push_back(std::move(adjacent_pos_vertical));
		}
	});

	return blocked_seeds;
}

void CMap::process_settlement_territory_tiles(const int z)
{
	for (int x = 0; x < this->Info->MapWidths[z]; ++x) {
		for (int y = 0; y < this->Info->MapHeights[z]; ++y) {
			const QPoint tile_pos(x, y);
			const wyrmgus::tile *tile = this->Field(x, y, z);
			const wyrmgus::site *settlement = tile->get_settlement();

			if (settlement == nullptr) {
				continue;
			}

			wyrmgus::site_game_data *settlement_game_data = settlement->get_game_data();
			settlement_game_data->process_territory_tile(tile, tile_pos, z);
		}
	}
}

void CMap::calculate_settlement_resource_units()
{
	for (const wyrmgus::site *site : wyrmgus::site::get_all()) {
		site->get_game_data()->clear_resource_units();
	}

	//add resource units to the settlement resource unit lists
	for (CUnit *unit : wyrmgus::unit_manager::get()->get_units()) {
		if (!unit->IsAliveOnMap()) {
			continue;
		}

		if (!unit->Type->can_produce_a_resource()) {
			continue;
		}
		
		const wyrmgus::tile *tile = unit->get_center_tile();
		if (tile->get_settlement() != nullptr) {
			tile->get_settlement()->get_game_data()->add_resource_unit(unit);
		}
	}
}

void CMap::generate_neutral_units(const wyrmgus::unit_type *unit_type, const int quantity, const QPoint &min_pos, const QPoint &max_pos, const bool grouped, const int z)
{
	if (SaveGameLoading) {
		return;
	}
	
	QPoint unit_pos(-1, -1);
	
	for (int i = 0; i < quantity; ++i) {
		if (i == 0 || !grouped) {
			unit_pos = this->generate_unit_location(unit_type, nullptr, min_pos, max_pos, z);
		}
		if (!this->Info->IsPointOnMap(unit_pos, z)) {
			continue;
		}
		if (unit_type->get_given_resource() != nullptr) {
			CreateResourceUnit(unit_pos, *unit_type, z);
		} else {
			CreateUnit(unit_pos, *unit_type, CPlayer::get_neutral_player(), z, unit_type->BoolFlag[BUILDING_INDEX].value && unit_type->get_tile_width() > 1 && unit_type->get_tile_height() > 1);
		}
	}
}
//Wyrmgus end

//Wyrmgus start
void CMap::ClearOverlayTile(const Vec2i &pos, int z)
{
	wyrmgus::tile &mf = *this->Field(pos, z);

	if (mf.get_overlay_terrain() == nullptr) {
		return;
	}
	
	this->SetOverlayTerrainDestroyed(pos, true, z);

	//remove decorations if a wall, tree or rock was removed from the tile
	std::vector<CUnit *> table;
	Select(pos, pos, table, z);
	for (size_t i = 0; i != table.size(); ++i) {
		if (table[i]->Type->UnitType == UnitTypeType::Land && table[i]->Type->BoolFlag[DECORATION_INDEX].value) {
			if (!CEditor::get()->is_running()) {
				LetUnitDie(*table[i]);			
			} else {
				EditorActionRemoveUnit(*table[i], false);
			}
		}
	}

	//check if any further tile should be removed with the clearing of this one
	if (!mf.get_overlay_terrain()->allows_single()) {
		for (int x_offset = -1; x_offset <= 1; ++x_offset) {
			for (int y_offset = -1; y_offset <= 1; ++y_offset) {
				if (x_offset != 0 || y_offset != 0) {
					Vec2i adjacent_pos(pos.x + x_offset, pos.y + y_offset);
					if (this->Info->IsPointOnMap(adjacent_pos, z)) {
						wyrmgus::tile &adjacent_mf = *this->Field(adjacent_pos, z);
						
						if (adjacent_mf.get_overlay_terrain() == mf.get_overlay_terrain() && !adjacent_mf.OverlayTerrainDestroyed && !this->CurrentTerrainCanBeAt(adjacent_pos, true, z)) {
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
	wyrmgus::tile &mf = *this->Field(pos);

	mf.setGraphicTile(this->Tileset->getRemovedTreeTile());
	mf.Flags &= ~(tile_flag::tree | tile_flag::impassable);
	mf.set_value(0);

	UI.get_minimap()->UpdateXY(pos);
	FixNeighbors(tile_flag::tree, 0, pos);

	//maybe isExplored
	if (mf.player_info->IsExplored(*ThisPlayer)) {
		UI.get_minimap()->UpdateSeenXY(pos);
		MarkSeenTile(mf);
	}
}

/// Remove rock from the map.
void CMap::ClearRockTile(const Vec2i &pos)
{
	wyrmgus::tile &mf = *this->Field(pos);

	mf.setGraphicTile(this->Tileset->getRemovedRockTile());
	mf.Flags &= ~(tile_flag::rock | tile_flag::impassable);
	mf.set_value(0);
	
	UI.get_minimap()->UpdateXY(pos);
	FixNeighbors(tile_flag::rock, 0, pos);

	//maybe isExplored
	if (mf.player_info->IsExplored(*ThisPlayer)) {
		UI.get_minimap()->UpdateSeenXY(pos);
		MarkSeenTile(mf);
	}
}
*/
//Wyrmgus end

void CMap::handle_destroyed_overlay_terrain()
{
	for (const std::unique_ptr<CMapLayer> &map_layer : this->MapLayers) {
		map_layer->handle_destroyed_overlay_terrain();
	}
}

void CMap::add_landmass(std::unique_ptr<landmass> &&landmass)
{
	this->landmasses.push_back(std::move(landmass));
}

void CMap::remove_settlement_unit(CUnit *settlement_unit)
{
	vector::remove(this->settlement_units, settlement_unit);
}

void CMap::FixSelectionArea(Vec2i &minpos, Vec2i &maxpos, int z)
{
	minpos.x = std::max<short>(0, minpos.x);
	minpos.y = std::max<short>(0, minpos.y);

	//Wyrmgus start
//		maxpos.x = std::min<short>(maxpos.x, Info->MapWidth - 1);
//		maxpos.y = std::min<short>(maxpos.y, Info->MapHeight - 1);
	maxpos.x = std::min<short>(maxpos.x, Info->MapWidths[z] - 1);
	maxpos.y = std::min<short>(maxpos.y, Info->MapHeights[z] - 1);
	//Wyrmgus end
}

/**
**  Load the map presentation
**
**  @param mapname  map filename
*/
void LoadStratagusMapInfo(const std::filesystem::path &map_path)
{
	// Set the default map setup by replacing .smp with .sms
	if (map_path.string().find(".smp") != std::string::npos) {
		CMap::get()->Info->set_presentation_filepath(map_path);
	}

	const std::string filename = LibraryFileName(map_path.string().c_str());
	LuaLoadFile(filename);
}

void SetMapWorld(const std::string &map_world)
{
	CMap::get()->get_info()->MapWorld = map_world;
}

void load_map_data(const std::string &sml_string)
{
	sml_parser parser;
	database::process_sml_data(CMap::get(), parser.parse(sml_string));
}
