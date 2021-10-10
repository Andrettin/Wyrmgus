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
/**@name map_fog.cpp - The map fog of war handling. */
//
//      (c) Copyright 1999-2021 by Lutz Sammer, Vladi Shabanski,
//		Russell Smith, Jimmy Salmon, Pali Rohár and Andrettin
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

#include "actions.h"
#include "database/defines.h"
#include "database/preferences.h"
#include "map/map_info.h"
#include "map/map_layer.h"
#include "map/minimap.h"
#include "map/tile.h"
#include "map/tile_flag.h"
#include "player/player.h"
#include "ui/ui.h"
#include "unit/unit.h"
//Wyrmgus start
#include "unit/unit_find.h"
//Wyrmgus end
#include "unit/unit_manager.h"
#include "util/assert_util.h"
#include "util/util.h"
#include "video/intern_video.h"
#include "video/renderer.h"
#include "video/video.h"


int FogOfWarOpacity;                 /// Fog of war Opacity.
uint32_t FogOfWarColorSDL;
CColor FogOfWarColor;

std::shared_ptr<CGraphic> CMap::FogGraphics;

/**
**  Mapping for fog of war tiles.
*/
static constexpr std::array<int, 16> FogTable = {
	//Wyrmgus start
//	0, 11, 10, 2,  13, 6, 14, 3,  12, 15, 4, 1,  8, 9, 7, 0,
	0, 11, 10, 2,  13, 6, 14, 3,  12, 15, 4, 1,  8, 9, 7, 16,
	//Wyrmgus end
};

//Wyrmgus start
//static std::vector<unsigned short> VisibleTable;
static std::vector<std::vector<unsigned short>> VisibleTable;
//Wyrmgus end

class _filter_flags
{
public:
	explicit _filter_flags(const CPlayer &p, tile_flag *fogmask) : player(&p), fogmask(fogmask)
	{
		assert_throw(fogmask != nullptr);
	}

	void operator()(const CUnit *const unit) const
	{
		if (!unit->IsVisibleAsGoal(*player)) {
			*fogmask &= ~unit->Type->FieldFlags;
		}
	}
private:
	const CPlayer *player;
	tile_flag *fogmask;
};

/**
**  Find out what the tile flags are a tile is covered by fog
**
**  @param player  player who is doing operation
**  @param index   map location
**  @param mask    input mask to filter
**
**  @return        Filtered mask after taking fog into account
*/
//Wyrmgus start
//int MapFogFilterFlags(CPlayer &player, const unsigned int index, const tile_flag mask)
tile_flag MapFogFilterFlags(CPlayer &player, const unsigned int index, const tile_flag mask, int z)
//Wyrmgus end
{
	tile_flag fogMask = mask;

	_filter_flags filter(player, &fogMask);
	//Wyrmgus start
//	CMap::get()->Field(index)->UnitCache.for_each(filter);
	CMap::get()->Field(index, z)->UnitCache.for_each(filter);
	//Wyrmgus end
	return fogMask;
}

tile_flag MapFogFilterFlags(CPlayer &player, const Vec2i &pos, const tile_flag mask, const int z)
{
	if (CMap::get()->Info->IsPointOnMap(pos, z)) {
		return MapFogFilterFlags(player, CMap::get()->get_pos_index(pos, z), mask, z);
	}
	return mask;
}

template<bool MARK>
class _TileSeen final
{
public:
	//Wyrmgus start
//	explicit _TileSeen(const CPlayer &p , int c) : player(&p), cloak(c)
	explicit _TileSeen(const CPlayer &p , int c, int e) : player(&p), cloak(c), ethereal(e)
	//Wyrmgus end
	{}

	void operator()(CUnit *const unit) const
	{
		//Wyrmgus start
		if (unit->Type == nullptr) {
			fprintf(stderr, "Unit has no type: \"%s\" (%d, %d)\n", unit->Name.c_str(), unit->tilePos.x, unit->tilePos.y);
			return;
		}
		//Wyrmgus end
		//Wyrmgus start
//		if (cloak != (int)unit->Type->BoolFlag[PERMANENTCLOAK_INDEX].value) {
		if (cloak != (int)unit->Type->BoolFlag[PERMANENTCLOAK_INDEX].value || ethereal != (int)unit->Type->BoolFlag[ETHEREAL_INDEX].value) {
		//Wyrmgus end
			return ;
		}
		const int p = player->get_index();
		if constexpr (MARK) {
			//  If the unit goes out of fog, this can happen for any player that
			//  this player shares vision with, and can't YET see the unit.
			//  It will be able to see the unit after the Unit->VisCount ++
			if (!unit->VisCount[p]) {
				for (int pi = 0; pi < PlayerMax; ++pi) {
					if ((pi == p /*player->get_index()*/)
						|| player->has_mutual_shared_vision_with(CPlayer::Players[pi].get()) || player->is_revealed()) {
						if (!unit->IsVisible(*CPlayer::Players[pi])) {
							UnitGoesOutOfFog(*unit, *CPlayer::Players[pi]);
						}
					}
				}
			}
			unit->VisCount[p/*player->get_index()*/]++;
		} else {
			/*
			 * HACK: UGLY !!!
			 * There is bug in Seen code conneded with
			 * UnitAction::Die and Cloaked units.
			 */
			if (!unit->VisCount[p] && unit->CurrentAction() == UnitAction::Die) {
				return;
			}

			assert_throw(unit->VisCount[p] > 0);
			unit->VisCount[p]--;
			//  If the unit goes under of fog, this can happen for any player that
			//  this player shares vision to. First of all, before unmarking,
			//  every player that this player shares vision to can see the unit.
			//  Now we have to check who can't see the unit anymore.
			if (!unit->VisCount[p]) {
				for (int pi = 0; pi < PlayerMax; ++pi) {
					if (pi == p/*player->get_index()*/ ||
						player->has_mutual_shared_vision_with(CPlayer::Players[pi].get()) || player->is_revealed()) {
						if (!unit->IsVisible(*CPlayer::Players[pi])) {
							UnitGoesUnderFog(*unit, *CPlayer::Players[pi]);
						}
					}
				}
			}
		}
	}
private:
	const CPlayer *player;
	int cloak;
	//Wyrmgus start
	int ethereal;
	//Wyrmgus end
};

/**
**  Mark all units on a tile as now visible.
**
**  @param player  The player this is for.
**  @param mf      field location to check
**  @param cloak   If we mark cloaked units too.
*/
//Wyrmgus start
//static void UnitsOnTileMarkSeen(const CPlayer &player, wyrmgus::tile &mf, int cloak)
static void UnitsOnTileMarkSeen(const CPlayer &player, wyrmgus::tile &mf, int cloak, int ethereal)
//Wyrmgus end
{
	//Wyrmgus start
//	_TileSeen<true> seen(player, cloak);
	_TileSeen<true> seen(player, cloak, ethereal);
	//Wyrmgus end
	mf.UnitCache.for_each(seen);
}

/**
**  This function unmarks units on x, y as seen. It uses a reference count.
**
**  @param player    The player to mark for.
**  @param mf        field to check if building is on, and mark as seen
**  @param cloak     If this is for cloaked units.
*/
//Wyrmgus start
//static void UnitsOnTileUnmarkSeen(const CPlayer &player, wyrmgus::tile &mf, int cloak)
static void UnitsOnTileUnmarkSeen(const CPlayer &player, wyrmgus::tile &mf, int cloak, int ethereal)
//Wyrmgus end
{
	//Wyrmgus start
//	_TileSeen<false> seen(player, cloak);
	_TileSeen<false> seen(player, cloak, ethereal);
	//Wyrmgus end
	mf.UnitCache.for_each(seen);
}


/**
**  Mark a tile's sight. (Explore and make visible.)
**
**  @param player  Player to mark sight.
**  @param index   tile to mark.
*/
//Wyrmgus start
//void MapMarkTileSight(const CPlayer &player, const unsigned int index)
void MapMarkTileSight(const CPlayer &player, const unsigned int index, int z)
//Wyrmgus end
{
	//Wyrmgus start
//	wyrmgus::tile &mf = *CMap::get()->Field(index);
	wyrmgus::tile &mf = *CMap::get()->Field(index, z);
	//Wyrmgus end

	unsigned short &v = mf.player_info->get_visibility_state_ref(player.get_index());

	switch (v) {
		case 0:
		case 1:
			// Unexplored or unseen
			// When there is no fog only unexplored tiles are marked.
			if (!CMap::get()->NoFogOfWar || v == 0) {
				UnitsOnTileMarkSeen(player, mf, 0, 0);
			}

			v = 2;

			if (GameRunning) {
				if (CPlayer::GetThisPlayer() == &player || player.shares_visibility_with(CPlayer::GetThisPlayer())) {
					CMap::get()->MarkSeenTile(mf);
					UI.get_minimap()->update_exploration_index(index, z);
				}
			}
			break;
		default:
			assert_throw(v != 65535);
			++v;
			break;
	}
}

//Wyrmgus start
//void MapMarkTileSight(const CPlayer &player, const Vec2i &pos)
void MapMarkTileSight(const CPlayer &player, const Vec2i &pos, int z)
//Wyrmgus end
{
	assert_throw(CMap::get()->Info->IsPointOnMap(pos, z));
	MapMarkTileSight(player, CMap::get()->get_pos_index(pos, z), z);
}

/**
**  Unmark a tile's sight. (Explore and make visible.)
**
**  @param player  Player to mark sight.
**  @param index   tile to mark.
*/
//Wyrmgus start
//void MapUnmarkTileSight(const CPlayer &player, const unsigned int index)
void MapUnmarkTileSight(const CPlayer &player, const unsigned int index, int z)
//Wyrmgus end
{
	//Wyrmgus start
//	wyrmgus::tile &mf = *CMap::get()->Field(index);
	wyrmgus::tile &mf = *CMap::get()->Field(index, z);
	//Wyrmgus end
	unsigned short &v = mf.player_info->get_visibility_state_ref(player.get_index());

	switch (v) {
		case 0:  // Unexplored
		case 1:
			// This happens when we unmark everything in CommandSharedVision
			break;
		case 2:
			//when there is NoFogOfWar units never get unmarked.
			if (!CMap::get()->NoFogOfWar) {
				//Wyrmgus start
//				UnitsOnTileUnmarkSeen(player, mf, 0);
				UnitsOnTileUnmarkSeen(player, mf, 0, 0);
				//Wyrmgus end
			}

			//check visible tile, then deduct...
			if (mf.player_info->IsTeamVisible(*CPlayer::GetThisPlayer())) {
				CMap::get()->MarkSeenTile(mf);
			}

			--v;

			if (GameRunning) {
				if (CPlayer::GetThisPlayer() == &player || player.shares_visibility_with(CPlayer::GetThisPlayer())) {
					UI.get_minimap()->update_exploration_index(index, z);
				}
			}

			break;
		default:  // seen -> seen
			--v;
			break;
	}
}

//Wyrmgus start
//void MapUnmarkTileSight(const CPlayer &player, const Vec2i &pos)
void MapUnmarkTileSight(const CPlayer &player, const Vec2i &pos, int z)
//Wyrmgus end
{
	assert_throw(CMap::get()->Info->IsPointOnMap(pos, z));
	MapUnmarkTileSight(player, CMap::get()->get_pos_index(pos, z), z);
}

/**
**  Mark a tile for cloak detection.
**
**  @param player  Player to mark sight.
**  @param index   Tile to mark.
*/
//Wyrmgus start
//void MapMarkTileDetectCloak(const CPlayer &player, const unsigned int index)
void MapMarkTileDetectCloak(const CPlayer &player, const unsigned int index, int z)
//Wyrmgus end
{
	//Wyrmgus start
//	wyrmgus::tile &mf = *CMap::get()->Field(index);
	wyrmgus::tile &mf = *CMap::get()->Field(index, z);
	//Wyrmgus end
	unsigned char &v = mf.player_info->VisCloak[player.get_index()];
	if (v == 0) {
		//Wyrmgus start
//		UnitsOnTileMarkSeen(player, mf, 1);
		UnitsOnTileMarkSeen(player, mf, 1, 0);
		//Wyrmgus end
	}
	assert_throw(v != 255);
	++v;
}

//Wyrmgus start
//void MapMarkTileDetectCloak(const CPlayer &player, const Vec2i &pos)
void MapMarkTileDetectCloak(const CPlayer &player, const Vec2i &pos, int z)
//Wyrmgus end
{
	//Wyrmgus start
//	MapMarkTileDetectCloak(player, CMap::get()->get_pos_index(pos));
	MapMarkTileDetectCloak(player, CMap::get()->get_pos_index(pos, z), z);
	//Wyrmgus end
}

/**
**  Unmark a tile for cloak detection.
**
**  @param player  Player to mark sight.
**  @param index   tile to mark.
*/
//Wyrmgus start
//void MapUnmarkTileDetectCloak(const CPlayer &player, const unsigned int index)
void MapUnmarkTileDetectCloak(const CPlayer &player, const unsigned int index, int z)
//Wyrmgus end
{
	//Wyrmgus start
//	wyrmgus::tile &mf = *CMap::get()->Field(index);
	wyrmgus::tile &mf = *CMap::get()->Field(index, z);
	//Wyrmgus end
	unsigned char &v = mf.player_info->VisCloak[player.get_index()];
	assert_throw(v != 0);
	if (v == 1) {
		//Wyrmgus start
//		UnitsOnTileUnmarkSeen(player, mf, 1);
		UnitsOnTileUnmarkSeen(player, mf, 1, 0);
		//Wyrmgus end
	}
	--v;
}

//Wyrmgus start
//void MapUnmarkTileDetectCloak(const CPlayer &player, const Vec2i &pos)
void MapUnmarkTileDetectCloak(const CPlayer &player, const Vec2i &pos, int z)
//Wyrmgus end
{
	//Wyrmgus start
//	MapUnmarkTileDetectCloak(player, CMap::get()->get_pos_index(pos));
	MapUnmarkTileDetectCloak(player, CMap::get()->get_pos_index(pos, z), z);
	//Wyrmgus end
}

//Wyrmgus start
/**
**  Mark a tile for ethereal detection.
**
**  @param player  Player to mark sight.
**  @param index   Tile to mark.
*/
void MapMarkTileDetectEthereal(const CPlayer &player, const unsigned int index, int z)
{
	wyrmgus::tile &mf = *CMap::get()->Field(index, z);
	unsigned char &v = mf.player_info->VisEthereal[player.get_index()];
	if (v == 0) {
		UnitsOnTileMarkSeen(player, mf, 0, 1);
	}
	assert_throw(v != 255);
	++v;
}

void MapMarkTileDetectEthereal(const CPlayer &player, const Vec2i &pos, int z)
{
	MapMarkTileDetectEthereal(player, CMap::get()->get_pos_index(pos, z), z);
}

/**
**  Unmark a tile for ethereal detection.
**
**  @param player  Player to mark sight.
**  @param index   tile to mark.
*/
void MapUnmarkTileDetectEthereal(const CPlayer &player, const unsigned int index, int z)
{
	wyrmgus::tile &mf = *CMap::get()->Field(index, z);
	unsigned char &v = mf.player_info->VisEthereal[player.get_index()];
	assert_throw(v != 0);
	if (v == 1) {
		UnitsOnTileUnmarkSeen(player, mf, 0, 1);
	}
	--v;
}

void MapUnmarkTileDetectEthereal(const CPlayer &player, const Vec2i &pos, int z)
{
	MapUnmarkTileDetectEthereal(player, CMap::get()->get_pos_index(pos, z), z);
}
//Wyrmgus end

/**
**  Mark the sight of unit. (Explore and make visible.)
**
**  @param player  player to mark the sight for (not unit owner)
**  @param pos     location to mark
**  @param w       width to mark, in square
**  @param h       height to mark, in square
**  @param range   Radius to mark.
**  @param marker  Function to mark or unmark sight
*/
template <wyrmgus::map_marker_func_ptr marker>
//Wyrmgus start
//void MapSight(const CPlayer &player, const Vec2i &pos, int w, int h, int range)
void MapSight(const CPlayer &player, const Vec2i &pos, int w, int h, int range, int z)
//Wyrmgus end
{
	// Units under construction have no sight range.
	if (!range) {
		return;
	}

	static constexpr tile_flag sight_obstacle_flag = tile_flag::air_impassable;
	static constexpr int max_obstacle_difference = 1; //how many tiles are seen after the obstacle; set to 1 here so that the obstacle tiles themselves don't have fog drawn over them
	
	// Up hemi-cyle
	const int miny = std::max(-range, 0 - pos.y);
	
	for (int offsety = miny; offsety != 0; ++offsety) {
		const int offsetx = isqrt(square(range + 1) - square(-offsety) - 1);
		const int minx = std::max(0, pos.x - offsetx);
		//Wyrmgus start
//		const int maxx = std::min(CMap::get()->Info->MapWidth, pos.x + w + offsetx);
		const int maxx = std::min(CMap::get()->Info->MapWidths[z], pos.x + w + offsetx);
		//Wyrmgus end
		Vec2i mpos(minx, pos.y + offsety);
		//Wyrmgus start
//		const unsigned int index = mpos.y * CMap::get()->Info->MapWidth;
		const unsigned int index = mpos.y * CMap::get()->Info->MapWidths[z];
		//Wyrmgus end

		for (mpos.x = minx; mpos.x < maxx; ++mpos.x) {
			bool obstacle_check = false;
			for (int x = 0; x < w; ++x) {
				for (int y = 0; y < h; ++y) {
					if (CheckObstaclesBetweenTiles(pos + Vec2i(x, y), mpos, sight_obstacle_flag, z, max_obstacle_difference)) { //the obstacle must be avoidable from at least one of the unit's tiles
						obstacle_check = true;
						break;
					}
				}
				if (obstacle_check) {
					break;
				}
			}
			if (!obstacle_check) {
				continue;
			}

			//Wyrmgus start
//			marker(player, mpos.x + index);
			marker(player, mpos.x + index, z);
			//Wyrmgus end
		}
	}
	for (int offsety = 0; offsety < h; ++offsety) {
		const int minx = std::max(0, pos.x - range);
		//Wyrmgus start
//		const int maxx = std::min(CMap::get()->Info->MapWidth, pos.x + w + range);
		const int maxx = std::min(CMap::get()->Info->MapWidths[z], pos.x + w + range);
		//Wyrmgus end
		Vec2i mpos(minx, pos.y + offsety);
		//Wyrmgus start
//		const unsigned int index = mpos.y * CMap::get()->Info->MapWidth;
		const unsigned int index = mpos.y * CMap::get()->Info->MapWidths[z];
		//Wyrmgus end

		for (mpos.x = minx; mpos.x < maxx; ++mpos.x) {
			bool obstacle_check = false;
			for (int x = 0; x < w; ++x) {
				for (int y = 0; y < h; ++y) {
					if (CheckObstaclesBetweenTiles(pos + Vec2i(x, y), mpos, sight_obstacle_flag, z, max_obstacle_difference)) { //the obstacle must be avoidable from at least one of the unit's tiles
						obstacle_check = true;
						break;
					}
				}
				if (obstacle_check) {
					break;
				}
			}
			if (!obstacle_check) {
				continue;
			}

			//Wyrmgus start
//			marker(player, mpos.x + index);
			marker(player, mpos.x + index, z);
			//Wyrmgus end
		}
	}
	// bottom hemi-cycle
	//Wyrmgus start
//	const int maxy = std::min(range, CMap::get()->Info->MapHeight - pos.y - h);
	const int maxy = std::min(range, CMap::get()->Info->MapHeights[z] - pos.y - h);
	//Wyrmgus end
	for (int offsety = 0; offsety < maxy; ++offsety) {
		const int offsetx = isqrt(square(range + 1) - square(offsety + 1) - 1);
		const int minx = std::max(0, pos.x - offsetx);
		//Wyrmgus start
//		const int maxx = std::min(CMap::get()->Info->MapWidth, pos.x + w + offsetx);
		const int maxx = std::min(CMap::get()->Info->MapWidths[z], pos.x + w + offsetx);
		//Wyrmgus end
		Vec2i mpos(minx, pos.y + h + offsety);
		//Wyrmgus start
//		const unsigned int index = mpos.y * CMap::get()->Info->MapWidth;
		const unsigned int index = mpos.y * CMap::get()->Info->MapWidths[z];
		//Wyrmgus end

		for (mpos.x = minx; mpos.x < maxx; ++mpos.x) {
			bool obstacle_check = false;
			for (int x = 0; x < w; ++x) {
				for (int y = 0; y < h; ++y) {
					if (CheckObstaclesBetweenTiles(pos + Vec2i(x, y), mpos, sight_obstacle_flag, z, max_obstacle_difference)) { //the obstacle must be avoidable from at least one of the unit's tiles
						obstacle_check = true;
						break;
					}
				}
				if (obstacle_check) {
					break;
				}
			}
			if (!obstacle_check) {
				continue;
			}

			//Wyrmgus start
//			marker(player, mpos.x + index);
			marker(player, mpos.x + index, z);
			//Wyrmgus end
		}
	}
}

template void MapSight<MapMarkTileSight>(const CPlayer &player, const Vec2i &pos, const int w, const int h, const int range, const int z);
template void MapSight<MapUnmarkTileSight>(const CPlayer &player, const Vec2i &pos, const int w, const int h, const int range, const int z);
template void MapSight<MapMarkTileDetectCloak>(const CPlayer &player, const Vec2i &pos, const int w, const int h, const int range, const int z);
template void MapSight<MapUnmarkTileDetectCloak>(const CPlayer &player, const Vec2i &pos, const int w, const int h, const int range, const int z);
template void MapSight<MapMarkTileDetectEthereal>(const CPlayer &player, const Vec2i &pos, const int w, const int h, const int range, const int z);
template void MapSight<MapUnmarkTileDetectEthereal>(const CPlayer &player, const Vec2i &pos, const int w, const int h, const int range, const int z);
template void MapSight<MapMarkTileRadar>(const CPlayer &player, const Vec2i &pos, const int w, const int h, const int range, const int z);
template void MapSight<MapUnmarkTileRadar>(const CPlayer &player, const Vec2i &pos, const int w, const int h, const int range, const int z);
template void MapSight<MapMarkTileRadarJammer>(const CPlayer &player, const Vec2i &pos, const int w, const int h, const int range, const int z);
template void MapSight<MapUnmarkTileRadarJammer>(const CPlayer &player, const Vec2i &pos, const int w, const int h, const int range, const int z);

/**
**  Update fog of war.
*/
void UpdateFogOfWarChange()
{
	DebugPrint("::UpdateFogOfWarChange\n");
	//  Mark all explored fields as visible again.
	if (CMap::get()->NoFogOfWar) {
		//Wyrmgus start
		/*
		const unsigned int w = Map.Info.MapHeight * Map.Info.MapWidth;
		for (unsigned int index = 0; index != w; ++index) {
			wyrmgus::tile &mf = *Map.Field(index);
			if (mf.player_info->is_explored(*ThisPlayer)) {
				Map.MarkSeenTile(mf);
			}
		}
		*/
		for (size_t z = 0; z < CMap::get()->MapLayers.size(); ++z) {
			const unsigned int w = CMap::get()->Info->MapHeights[z] * CMap::get()->Info->MapWidths[z];
			for (unsigned int index = 0; index != w; ++index) {
				wyrmgus::tile &mf = *CMap::get()->Field(index, z);
				if (mf.player_info->is_explored(*CPlayer::GetThisPlayer())) {
					CMap::get()->MarkSeenTile(mf);
				}
			}
		}
		//Wyrmgus end
	}

	//  Global seen recount.
	for (CUnit *unit : wyrmgus::unit_manager::get()->get_units()) {
		UnitCountSeen(*unit);
	}

	for (size_t z = 0; z < CMap::get()->MapLayers.size(); ++z) {
		UI.get_minimap()->update_exploration(z);
	}
}

/*----------------------------------------------------------------------------
--  Draw fog solid
----------------------------------------------------------------------------*/

/**
**  Draw only fog of war
**
**  @param x  X position into video memory
**  @param y  Y position into video memory
*/
static void VideoDrawOnlyFog(const int x, const int y, std::vector<std::function<void(renderer *)>> &render_commands)
{
	Video.FillRectangleClip(CVideo::MapRGBA(0, 0, 0, FogOfWarOpacity), x, y, defines::get()->get_scaled_tile_width(), defines::get()->get_scaled_tile_height(), render_commands);
}

/*----------------------------------------------------------------------------
--  Old version correct working but not 100% original
----------------------------------------------------------------------------*/

//Wyrmgus start
//static void GetFogOfWarTile(int sx, int sy, int *fogTile, int *blackFogTile)
static void GetFogOfWarTile(int sx, int sy, int *fogTile, int *blackFogTile, int z)
//Wyrmgus end
{
//Wyrmgus start
//#define IsMapFieldExploredTable(index) (VisibleTable[(index)])
//#define IsMapFieldVisibleTable(index) (VisibleTable[(index)] > 1)
#define IsMapFieldExploredTable(index, layer) (VisibleTable[(layer)][(index)])
#define IsMapFieldVisibleTable(index, layer) (VisibleTable[(layer)][(index)] > 1)
//Wyrmgus end

	//Wyrmgus start
//	int w = CMap::get()->Info->MapWidth;
	int w = CMap::get()->Info->MapWidths[z];
	//Wyrmgus end
	int fogTileIndex = 0;
	int blackFogTileIndex = 0;
	int x = sx - sy;

	if (ReplayRevealMap) {
		*fogTile = 0;
		*blackFogTile = 0;
		return;
	}

	//
	//  Which Tile to draw for fog
	//
	// Investigate tiles around current tile
	// 1 2 3
	// 4 * 5
	// 6 7 8

	//    2  3 1
	//   10 ** 5
	//    8 12 4

	if (sy) {
		//Wyrmgus start
//		unsigned int index = sy - CMap::get()->Info->MapWidth;//(y-1) * Map.Info.MapWidth;
		unsigned int index = sy - CMap::get()->Info->MapWidths[z];//(y-1) * Map.Info.MapWidth;
		//Wyrmgus end
		if (sx != sy) {
			//if (!IsMapFieldExploredTable(x - 1, y - 1)) {
			//Wyrmgus start
//			if (!IsMapFieldExploredTable(x - 1 + index)) {
			if (!IsMapFieldExploredTable(x - 1 + index, z)) {
			//Wyrmgus end
				blackFogTileIndex |= 2;
				fogTileIndex |= 2;
				//} else if (!IsMapFieldVisibleTable(x - 1, y - 1)) {
			//Wyrmgus start
//			} else if (!IsMapFieldVisibleTable(x - 1 + index)) {
			} else if (!IsMapFieldVisibleTable(x - 1 + index, z)) {
			//Wyrmgus end
				fogTileIndex |= 2;
			}
		}
		//if (!IsMapFieldExploredTable(x, y - 1)) {
		//Wyrmgus start
//		if (!IsMapFieldExploredTable(x + index)) {
		if (!IsMapFieldExploredTable(x + index, z)) {
		//Wyrmgus end
			blackFogTileIndex |= 3;
			fogTileIndex |= 3;
			//} else if (!IsMapFieldVisibleTable(x, y - 1)) {
		//Wyrmgus start
//		} else if (!IsMapFieldVisibleTable(x + index)) {
		} else if (!IsMapFieldVisibleTable(x + index, z)) {
		//Wyrmgus end
			fogTileIndex |= 3;
		}
		if (sx != sy + w - 1) {
			//if (!IsMapFieldExploredTable(x + 1, y - 1)) {
			//Wyrmgus start
//			if (!IsMapFieldExploredTable(x + 1 + index)) {
			if (!IsMapFieldExploredTable(x + 1 + index, z)) {
			//Wyrmgus end
				blackFogTileIndex |= 1;
				fogTileIndex |= 1;
				//} else if (!IsMapFieldVisibleTable(x + 1, y - 1)) {
			//Wyrmgus start
//			} else if (!IsMapFieldVisibleTable(x + 1 + index)) {
			} else if (!IsMapFieldVisibleTable(x + 1 + index, z)) {
			//Wyrmgus end
				fogTileIndex |= 1;
			}
		}
	}

	if (sx != sy) {
		unsigned int index = sy;//(y) * Map.Info.MapWidth;
		//if (!IsMapFieldExploredTable(x - 1, y)) {
		//Wyrmgus start
//		if (!IsMapFieldExploredTable(x - 1 + index)) {
		if (!IsMapFieldExploredTable(x - 1 + index, z)) {
		//Wyrmgus end
			blackFogTileIndex |= 10;
			fogTileIndex |= 10;
			//} else if (!IsMapFieldVisibleTable(x - 1, y)) {
		//Wyrmgus start
//		} else if (!IsMapFieldVisibleTable(x - 1 + index)) {
		} else if (!IsMapFieldVisibleTable(x - 1 + index, z)) {
		//Wyrmgus end
			fogTileIndex |= 10;
		}
	}
	if (sx != sy + w - 1) {
		unsigned int index = sy;//(y) * Map.Info.MapWidth;
		//if (!IsMapFieldExploredTable(x + 1, y)) {
		//Wyrmgus start
//		if (!IsMapFieldExploredTable(x + 1 + index)) {
		if (!IsMapFieldExploredTable(x + 1 + index, z)) {
		//Wyrmgus end
			blackFogTileIndex |= 5;
			fogTileIndex |= 5;
			//} else if (!IsMapFieldVisibleTable(x + 1, y)) {
		//Wyrmgus start
//		} else if (!IsMapFieldVisibleTable(x + 1 + index)) {
		} else if (!IsMapFieldVisibleTable(x + 1 + index, z)) {
		//Wyrmgus end
			fogTileIndex |= 5;
		}
	}

	//Wyrmgus start
//	if (sy + w < CMap::get()->Info->MapHeight * w) {
	if (sy + w < CMap::get()->Info->MapHeights[z] * w) {
	//Wyrmgus end
		//Wyrmgus start
//		unsigned int index = sy + CMap::get()->Info->MapWidth;//(y+1) * Map.Info.MapWidth;
		unsigned int index = sy + CMap::get()->Info->MapWidths[z];//(y+1) * Map.Info.MapWidth;
		//Wyrmgus end
		if (sx != sy) {
			//if (!IsMapFieldExploredTable(x - 1, y + 1)) {
			//Wyrmgus start
//			if (!IsMapFieldExploredTable(x - 1 + index)) {
			if (!IsMapFieldExploredTable(x - 1 + index, z)) {
			//Wyrmgus end
				blackFogTileIndex |= 8;
				fogTileIndex |= 8;
				//} else if (!IsMapFieldVisibleTable(x - 1, y + 1)) {
			//Wyrmgus start
//			} else if (!IsMapFieldVisibleTable(x - 1 + index)) {
			} else if (!IsMapFieldVisibleTable(x - 1 + index, z)) {
			//Wyrmgus end
				fogTileIndex |= 8;
			}
		}
		//if (!IsMapFieldExploredTable(x, y + 1)) {
		//Wyrmgus start
//		if (!IsMapFieldExploredTable(x + index)) {
		if (!IsMapFieldExploredTable(x + index, z)) {
		//Wyrmgus end
			blackFogTileIndex |= 12;
			fogTileIndex |= 12;
			//} else if (!IsMapFieldVisibleTable(x, y + 1)) {
		//Wyrmgus start
//		} else if (!IsMapFieldVisibleTable(x + index)) {
		} else if (!IsMapFieldVisibleTable(x + index, z)) {
		//Wyrmgus end
			fogTileIndex |= 12;
		}
		if (sx != sy + w - 1) {
			//if (!IsMapFieldExploredTable(x + 1, y + 1)) {
			//Wyrmgus start
//			if (!IsMapFieldExploredTable(x + 1 + index)) {
			if (!IsMapFieldExploredTable(x + 1 + index, z)) {
			//Wyrmgus end
				blackFogTileIndex |= 4;
				fogTileIndex |= 4;
				//} else if (!IsMapFieldVisibleTable(x + 1, y + 1)) {
			//Wyrmgus start
//			} else if (!IsMapFieldVisibleTable(x + 1 + index)) {
			} else if (!IsMapFieldVisibleTable(x + 1 + index, z)) {
			//Wyrmgus end
				fogTileIndex |= 4;
			}
		}
	}

	//Wyrmgus start
//	*fogTile = FogTable[fogTileIndex];
//	*blackFogTile = FogTable[blackFogTileIndex];
	//apply variation according to tile index (sx is equal to the tile index, so let's use it)
	int FogTileVariation = 0;
	const std::shared_ptr<CGraphic> &fog_graphic = CMap::get()->FogGraphics;
	if (sx % 3 == 0 && fog_graphic->get_height() / wyrmgus::defines::get()->get_scaled_tile_height() >= 3) {
		FogTileVariation = 2;
	} else if (sx % 2 == 0 && fog_graphic->get_height() / wyrmgus::defines::get()->get_scaled_tile_height() >= 2) {
		FogTileVariation = 1;
	}
	if (FogTable[fogTileIndex] && FogTable[fogTileIndex] != 16) {
		*fogTile = FogTable[fogTileIndex] + (FogTileVariation * 16);
	} else {
		*fogTile = FogTable[fogTileIndex];
	}
	if (FogTable[blackFogTileIndex] && FogTable[blackFogTileIndex] != 16) {
		*blackFogTile = FogTable[blackFogTileIndex] + (FogTileVariation * 16);
	} else {
		*blackFogTile = FogTable[blackFogTileIndex];
	}
	//Wyrmgus end
}

/**
**  Draw fog of war tile.
**
**  @param sx  Offset into fields to current tile.
**  @param sy  Start of the current row.
**  @param dx  X position into video memory.
**  @param dy  Y position into video memory.
*/
static void DrawFogOfWarTile(const int sx, const int sy, const int dx, const int dy, std::vector<std::function<void(renderer *)>> &render_commands)
{
	int fogTile = 0;
	int blackFogTile = 0;

	//Wyrmgus start
//	GetFogOfWarTile(sx, sy, &fogTile, &blackFogTile);
	GetFogOfWarTile(sx, sy, &fogTile, &blackFogTile, UI.CurrentMapLayer->ID);
	//Wyrmgus end

	//Wyrmgus start
	const std::shared_ptr<CGraphic> &fog_graphic = CMap::get()->FogGraphics;
	
//	if (IsMapFieldVisibleTable(sx) || ReplayRevealMap) {
	if ((IsMapFieldVisibleTable(sx, UI.CurrentMapLayer->ID) && blackFogTile != 16 && fogTile != 16) || ReplayRevealMap) {
	//Wyrmgus end
		if (fogTile && fogTile != blackFogTile) {
			fog_graphic->DrawFrameClipTrans(fogTile, dx, dy, FogOfWarOpacity, render_commands);
		}
	} else {
		VideoDrawOnlyFog(dx, dy, render_commands);
	}
	if (blackFogTile) {
		fog_graphic->DrawFrameClip(blackFogTile, dx, dy, render_commands);
	}

#undef IsMapFieldExploredTable
#undef IsMapFieldVisibleTable
}

/**
**  Draw the map fog of war.
*/
void CViewport::draw_map_fog_of_war(std::vector<std::function<void(renderer *)>> &render_commands) const
{
	// flags must redraw or not
	if (ReplayRevealMap) {
		return;
	}

	int sx = std::max<int>(MapPos.x - 1, 0);
	int ex = std::min<int>(MapPos.x + MapWidth + 1, UI.CurrentMapLayer->get_width());
	int my = std::max<int>(MapPos.y - 1, 0);
	int ey = std::min<int>(MapPos.y + MapHeight + 1, UI.CurrentMapLayer->get_height());

	// Update for visibility all tile in viewport
	// and 1 tile around viewport (for fog-of-war connection display)

	unsigned int my_index = my * UI.CurrentMapLayer->get_width();
	for (; my < ey; ++my) {
		for (int mx = sx; mx < ex; ++mx) {
			//Wyrmgus start
//			VisibleTable[my_index + mx] = CMap::get()->Field(mx + my_index)->player_info->TeamVisibilityState(*ThisPlayer);
			VisibleTable[UI.CurrentMapLayer->ID][my_index + mx] = CMap::get()->Field(mx + my_index, UI.CurrentMapLayer->ID)->player_info->get_team_visibility_state(*CPlayer::GetThisPlayer());
			//Wyrmgus end
		}
		my_index += UI.CurrentMapLayer->get_width();
	}
	ex = this->get_bottom_right_pos().x();
	int sy = MapPos.y * UI.CurrentMapLayer->get_width();
	int dy = this->get_top_left_pos().y() - Offset.y;
	ey = this->get_bottom_right_pos().y();

	while (dy <= ey) {
		sx = MapPos.x + sy;
		int dx = this->get_top_left_pos().x() - Offset.x;
		while (dx <= ex) {
			//Wyrmgus start
//			if (VisibleTable[sx]) {
			if (VisibleTable[UI.CurrentMapLayer->ID][sx]) {
			//Wyrmgus end
				DrawFogOfWarTile(sx, sy, dx, dy, render_commands);
			} else {
				Video.FillRectangleClip(FogOfWarColorSDL, dx, dy, wyrmgus::defines::get()->get_scaled_tile_width(), defines::get()->get_scaled_tile_height(), render_commands);
			}
			++sx;
			dx += defines::get()->get_scaled_tile_width();
		}
		sy += UI.CurrentMapLayer->get_width();
		dy += defines::get()->get_scaled_tile_height();
	}
}

/**
**  Initialize the fog of war.
**  Build tables, setup functions.
*/
void CMap::InitFogOfWar()
{
	const std::shared_ptr<CGraphic> &fog_graphic = CMap::FogGraphics;
	
	//calculate this once from the settings and store it
	FogOfWarColorSDL = CVideo::MapRGB(FogOfWarColor);

	fog_graphic->Load(preferences::get()->get_scale_factor());

	//Wyrmgus start
//	VisibleTable.clear();
//	VisibleTable.resize(Info.MapWidth * Info.MapHeight);
	VisibleTable.clear();
	VisibleTable.resize(this->MapLayers.size());
	for (size_t z = 0; z < this->MapLayers.size(); ++z) {
		VisibleTable[z].resize(this->Info->MapWidths[z] * this->Info->MapHeights[z]);
	}
	//Wyrmgus end
}

/**
**  Cleanup the fog of war.
*/
void CMap::CleanFogOfWar()
{
	VisibleTable.clear();

	CMap::FogGraphics.reset();
}
