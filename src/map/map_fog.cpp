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
//      (c) Copyright 1999-2015 by Lutz Sammer, Vladi Shabanski,
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
//

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "map.h"

#include "actions.h"
#include "minimap.h"
#include "player.h"
//Wyrmgus start
#include "tileset.h"
//Wyrmgus end
#include "ui.h"
#include "unit.h"
//Wyrmgus start
#include "unit_find.h"
//Wyrmgus end
#include "unit_manager.h"
#include "video.h"
#include "../video/intern_video.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

int FogOfWarOpacity;                 /// Fog of war Opacity.
Uint32 FogOfWarColorSDL;
CColor FogOfWarColor;

CGraphic *CMap::FogGraphic;

/**
**  Mapping for fog of war tiles.
*/
static const int FogTable[16] = {
	//Wyrmgus start
//	0, 11, 10, 2,  13, 6, 14, 3,  12, 15, 4, 1,  8, 9, 7, 0,
	0, 11, 10, 2,  13, 6, 14, 3,  12, 15, 4, 1,  8, 9, 7, 16,
	//Wyrmgus end
};

//Wyrmgus start
//static std::vector<unsigned short> VisibleTable;
static std::vector<std::vector<unsigned short>> VisibleTable;
//Wyrmgus end

static SDL_Surface *OnlyFogSurface;
static CGraphic *AlphaFogG;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

class _filter_flags
{
public:
	_filter_flags(const CPlayer &p, int *fogmask) : player(&p), fogmask(fogmask)
	{
		Assert(fogmask != NULL);
	}

	void operator()(const CUnit *const unit) const
	{
		if (!unit->IsVisibleAsGoal(*player)) {
			*fogmask &= ~unit->Type->FieldFlags;
		}
	}
private:
	const CPlayer *player;
	int *fogmask;
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
//int MapFogFilterFlags(CPlayer &player, const unsigned int index, int mask)
int MapFogFilterFlags(CPlayer &player, const unsigned int index, int mask, int z)
//Wyrmgus end
{
	int fogMask = mask;

	_filter_flags filter(player, &fogMask);
	//Wyrmgus start
//	Map.Field(index)->UnitCache.for_each(filter);
	Map.Field(index, z)->UnitCache.for_each(filter);
	//Wyrmgus end
	return fogMask;
}

//Wyrmgus start
//int MapFogFilterFlags(CPlayer &player, const Vec2i &pos, int mask)
int MapFogFilterFlags(CPlayer &player, const Vec2i &pos, int mask, int z)
//Wyrmgus end
{
	//Wyrmgus start
//	if (Map.Info.IsPointOnMap(pos)) {
	if (Map.Info.IsPointOnMap(pos, z)) {
	//Wyrmgus end
		//Wyrmgus start
//		return MapFogFilterFlags(player, Map.getIndex(pos), mask);
		return MapFogFilterFlags(player, Map.getIndex(pos, z), mask, z);
		//Wyrmgus end
	}
	return mask;
}

template<bool MARK>
class _TileSeen
{
public:
	//Wyrmgus start
//	_TileSeen(const CPlayer &p , int c) : player(&p), cloak(c)
	_TileSeen(const CPlayer &p , int c, int e) : player(&p), cloak(c), ethereal(e)
	//Wyrmgus end
	{}

	void operator()(CUnit *const unit) const
	{
		//Wyrmgus start
		if (unit->Type == NULL) {
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
		const int p = player->Index;
		if (MARK) {
			//  If the unit goes out of fog, this can happen for any player that
			//  this player shares vision with, and can't YET see the unit.
			//  It will be able to see the unit after the Unit->VisCount ++
			if (!unit->VisCount[p]) {
				for (int pi = 0; pi < PlayerMax; ++pi) {
					if ((pi == p /*player->Index*/)
						//Wyrmgus start
//						|| player->IsBothSharedVision(Players[pi])) {
						|| player->IsBothSharedVision(Players[pi]) || player->Revealed) {
						//Wyrmgus end
						if (!unit->IsVisible(Players[pi])) {
							UnitGoesOutOfFog(*unit, Players[pi]);
						}
					}
				}
			}
			unit->VisCount[p/*player->Index*/]++;
		} else {
			/*
			 * HACK: UGLY !!!
			 * There is bug in Seen code conneded with
			 * UnitActionDie and Cloaked units.
			 */
			if (!unit->VisCount[p] && unit->CurrentAction() == UnitActionDie) {
				return;
			}

			Assert(unit->VisCount[p]);
			unit->VisCount[p]--;
			//  If the unit goes under of fog, this can happen for any player that
			//  this player shares vision to. First of all, before unmarking,
			//  every player that this player shares vision to can see the unit.
			//  Now we have to check who can't see the unit anymore.
			if (!unit->VisCount[p]) {
				for (int pi = 0; pi < PlayerMax; ++pi) {
					if (pi == p/*player->Index*/ ||
						//Wyrmgus start
//						player->IsBothSharedVision(Players[pi])) {
						player->IsBothSharedVision(Players[pi]) || player->Revealed) {
						//Wyrmgus end
						if (!unit->IsVisible(Players[pi])) {
							UnitGoesUnderFog(*unit, Players[pi]);
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
//static void UnitsOnTileMarkSeen(const CPlayer &player, CMapField &mf, int cloak)
static void UnitsOnTileMarkSeen(const CPlayer &player, CMapField &mf, int cloak, int ethereal)
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
//static void UnitsOnTileUnmarkSeen(const CPlayer &player, CMapField &mf, int cloak)
static void UnitsOnTileUnmarkSeen(const CPlayer &player, CMapField &mf, int cloak, int ethereal)
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
//	CMapField &mf = *Map.Field(index);
	CMapField &mf = *Map.Field(index, z);
	//Wyrmgus end
	unsigned short *v = &(mf.playerInfo.Visible[player.Index]);
	if (*v == 0 || *v == 1) { // Unexplored or unseen
		// When there is no fog only unexplored tiles are marked.
		if (!Map.NoFogOfWar || *v == 0) {
			//Wyrmgus start
//			UnitsOnTileMarkSeen(player, mf, 0);
			UnitsOnTileMarkSeen(player, mf, 0, 0);
			//Wyrmgus end
		}
		*v = 2;
		if (mf.playerInfo.IsTeamVisible(*ThisPlayer)) {
			//Wyrmgus start
//			Map.MarkSeenTile(mf);
			Map.MarkSeenTile(mf, z);
			//Wyrmgus end
		}
		return;
	}
	Assert(*v != 65535);
	++*v;
}

//Wyrmgus start
//void MapMarkTileSight(const CPlayer &player, const Vec2i &pos)
void MapMarkTileSight(const CPlayer &player, const Vec2i &pos, int z)
//Wyrmgus end
{
	//Wyrmgus start
//	Assert(Map.Info.IsPointOnMap(pos));
//	MapMarkTileSight(player, Map.getIndex(pos));
	Assert(Map.Info.IsPointOnMap(pos, z));
	MapMarkTileSight(player, Map.getIndex(pos, z), z);
	//Wyrmgus end
}

/**
**  Unmark a tile's sight. (Explore and make visible.)
**
**  @param player  Player to mark sight.
**  @param indexx  tile to mark.
*/
//Wyrmgus start
//void MapUnmarkTileSight(const CPlayer &player, const unsigned int index)
void MapUnmarkTileSight(const CPlayer &player, const unsigned int index, int z)
//Wyrmgus end
{
	//Wyrmgus start
//	CMapField &mf = *Map.Field(index);
	CMapField &mf = *Map.Field(index, z);
	//Wyrmgus end
	unsigned short *v = &mf.playerInfo.Visible[player.Index];
	switch (*v) {
		case 0:  // Unexplored
		case 1:
			// This happens when we unmark everything in CommandSharedVision
			break;
		case 2:
			// When there is NoFogOfWar units never get unmarked.
			if (!Map.NoFogOfWar) {
				//Wyrmgus start
//				UnitsOnTileUnmarkSeen(player, mf, 0);
				UnitsOnTileUnmarkSeen(player, mf, 0, 0);
				//Wyrmgus end
			}
			// Check visible Tile, then deduct...
			if (mf.playerInfo.IsTeamVisible(*ThisPlayer)) {
				//Wyrmgus start
//				Map.MarkSeenTile(mf);
				Map.MarkSeenTile(mf, z);
				//Wyrmgus end
			}
		default:  // seen -> seen
			--*v;
			break;
	}
}

//Wyrmgus start
//void MapUnmarkTileSight(const CPlayer &player, const Vec2i &pos)
void MapUnmarkTileSight(const CPlayer &player, const Vec2i &pos, int z)
//Wyrmgus end
{
	//Wyrmgus start
//	Assert(Map.Info.IsPointOnMap(pos));
//	MapUnmarkTileSight(player, Map.getIndex(pos));
	Assert(Map.Info.IsPointOnMap(pos, z));
	MapUnmarkTileSight(player, Map.getIndex(pos, z), z);
	//Wyrmgus end
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
//	CMapField &mf = *Map.Field(index);
	CMapField &mf = *Map.Field(index, z);
	//Wyrmgus end
	unsigned char *v = &mf.playerInfo.VisCloak[player.Index];
	if (*v == 0) {
		//Wyrmgus start
//		UnitsOnTileMarkSeen(player, mf, 1);
		UnitsOnTileMarkSeen(player, mf, 1, 0);
		//Wyrmgus end
	}
	Assert(*v != 255);
	++*v;
}

//Wyrmgus start
//void MapMarkTileDetectCloak(const CPlayer &player, const Vec2i &pos)
void MapMarkTileDetectCloak(const CPlayer &player, const Vec2i &pos, int z)
//Wyrmgus end
{
	//Wyrmgus start
//	MapMarkTileDetectCloak(player, Map.getIndex(pos));
	MapMarkTileDetectCloak(player, Map.getIndex(pos, z), z);
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
//	CMapField &mf = *Map.Field(index);
	CMapField &mf = *Map.Field(index, z);
	//Wyrmgus end
	unsigned char *v = &mf.playerInfo.VisCloak[player.Index];
	Assert(*v != 0);
	if (*v == 1) {
		//Wyrmgus start
//		UnitsOnTileUnmarkSeen(player, mf, 1);
		UnitsOnTileUnmarkSeen(player, mf, 1, 0);
		//Wyrmgus end
	}
	--*v;
}

//Wyrmgus start
//void MapUnmarkTileDetectCloak(const CPlayer &player, const Vec2i &pos)
void MapUnmarkTileDetectCloak(const CPlayer &player, const Vec2i &pos, int z)
//Wyrmgus end
{
	//Wyrmgus start
//	MapUnmarkTileDetectCloak(player, Map.getIndex(pos));
	MapUnmarkTileDetectCloak(player, Map.getIndex(pos, z), z);
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
	CMapField &mf = *Map.Field(index, z);
	unsigned char *v = &mf.playerInfo.VisEthereal[player.Index];
	if (*v == 0) {
		UnitsOnTileMarkSeen(player, mf, 0, 1);
	}
	Assert(*v != 255);
	++*v;
}

void MapMarkTileDetectEthereal(const CPlayer &player, const Vec2i &pos, int z)
{
	MapMarkTileDetectEthereal(player, Map.getIndex(pos, z), z);
}

/**
**  Unmark a tile for ethereal detection.
**
**  @param player  Player to mark sight.
**  @param index   tile to mark.
*/
void MapUnmarkTileDetectEthereal(const CPlayer &player, const unsigned int index, int z)
{
	CMapField &mf = *Map.Field(index, z);
	unsigned char *v = &mf.playerInfo.VisEthereal[player.Index];
	Assert(*v != 0);
	if (*v == 1) {
		UnitsOnTileUnmarkSeen(player, mf, 0, 1);
	}
	--*v;
}

void MapUnmarkTileDetectEthereal(const CPlayer &player, const Vec2i &pos, int z)
{
	MapUnmarkTileDetectEthereal(player, Map.getIndex(pos, z), z);
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
//Wyrmgus start
//void MapSight(const CPlayer &player, const Vec2i &pos, int w, int h, int range, MapMarkerFunc *marker)
void MapSight(const CPlayer &player, const Vec2i &pos, int w, int h, int range, MapMarkerFunc *marker, int z)
//Wyrmgus end
{
	// Units under construction have no sight range.
	if (!range) {
		return;
	}
	
	//Wyrmgus start
	std::vector<unsigned long> obstacle_flags;
	int max_obstacle_difference = 1; //how many tiles are seen after the obstacle; set to 1 here so that the obstacle tiles themselves don't have fog drawn over them
	
	if (marker == MapMarkTileOwnership || marker == MapUnmarkTileOwnership) {
	} else {
		if (Map.IsLayerUnderground(z)) {
			obstacle_flags.push_back(MapFieldAirUnpassable);
		}
	}
	//Wyrmgus end
	
	// Up hemi-cyle
	const int miny = std::max(-range, 0 - pos.y);
	
	for (int offsety = miny; offsety != 0; ++offsety) {
		const int offsetx = isqrt(square(range + 1) - square(-offsety) - 1);
		const int minx = std::max(0, pos.x - offsetx);
		//Wyrmgus start
		const int maxx = std::min<int>(Map.Info.LayersSizes[z].x, pos.x + w + offsetx);
		//Wyrmgus end
		Vec2i mpos(minx, pos.y + offsety);
#ifdef MARKER_ON_INDEX
		//Wyrmgus start
		const unsigned int index = mpos.y * Map.Info.LayersSizes[z].x;
		//Wyrmgus end
#endif

		for (mpos.x = minx; mpos.x < maxx; ++mpos.x) {
			//Wyrmgus start
			bool obstacle_check = true;
			for (size_t i = 0; i < obstacle_flags.size(); ++i) {
				bool obstacle_subcheck = false;
				for (int x = 0; x < w; ++x) {
					for (int y = 0; y < h; ++y) {
						if (CheckObstaclesBetweenTiles(pos + Vec2i(x, y), mpos, obstacle_flags[i], z, max_obstacle_difference)) { //the obstacle must be avoidable from at least one of the unit's tiles
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
			//Wyrmgus end
#ifdef MARKER_ON_INDEX
			//Wyrmgus start
//			marker(player, mpos.x + index);
			marker(player, mpos.x + index, z);
			//Wyrmgus end
#else
			//Wyrmgus start
//			marker(player, mpos);
			marker(player, mpos, z);
			//Wyrmgus end
#endif
			//Wyrmgus start
			if (marker == MapMarkTileOwnership || marker == MapUnmarkTileOwnership) {
				Map.CalculateTileOwnership(mpos, z);
			}
			//Wyrmgus end
		}
	}
	for (int offsety = 0; offsety < h; ++offsety) {
		const int minx = std::max(0, pos.x - range);
		//Wyrmgus start
		const int maxx = std::min<int>(Map.Info.LayersSizes[z].x, pos.x + w + range);
		//Wyrmgus end
		Vec2i mpos(minx, pos.y + offsety);
#ifdef MARKER_ON_INDEX
		//Wyrmgus start
		const unsigned int index = mpos.y * Map.Info.LayersSizes[z].x;
		//Wyrmgus end
#endif

		for (mpos.x = minx; mpos.x < maxx; ++mpos.x) {
			//Wyrmgus start
			bool obstacle_check = true;
			for (size_t i = 0; i < obstacle_flags.size(); ++i) {
				bool obstacle_subcheck = false;
				for (int x = 0; x < w; ++x) {
					for (int y = 0; y < h; ++y) {
						if (CheckObstaclesBetweenTiles(pos + Vec2i(x, y), mpos, obstacle_flags[i], z, max_obstacle_difference)) { //the obstacle must be avoidable from at least one of the unit's tiles
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
			//Wyrmgus end
#ifdef MARKER_ON_INDEX
			//Wyrmgus start
//			marker(player, mpos.x + index);
			marker(player, mpos.x + index, z);
			//Wyrmgus end
#else
			//Wyrmgus start
//			marker(player, mpos);
			marker(player, mpos, z);
			//Wyrmgus end
#endif
			//Wyrmgus start
			if (marker == MapMarkTileOwnership || marker == MapUnmarkTileOwnership) {
				Map.CalculateTileOwnership(mpos, z);
			}
			//Wyrmgus end
		}
	}
	// bottom hemi-cycle
	//Wyrmgus start
	const int maxy = std::min<int>(range, Map.Info.LayersSizes[z].y - pos.y - h);
	//Wyrmgus end
	for (int offsety = 0; offsety < maxy; ++offsety) {
		const int offsetx = isqrt(square(range + 1) - square(offsety + 1) - 1);
		const int minx = std::max(0, pos.x - offsetx);
		//Wyrmgus start
		const int maxx = std::min<int>(Map.Info.LayersSizes[z].x, pos.x + w + offsetx);
		//Wyrmgus end
		Vec2i mpos(minx, pos.y + h + offsety);
#ifdef MARKER_ON_INDEX
		//Wyrmgus start
		const unsigned int index = mpos.y * Map.Info.LayersSizes[z].x;
		//Wyrmgus end
#endif

		for (mpos.x = minx; mpos.x < maxx; ++mpos.x) {
			//Wyrmgus start
			bool obstacle_check = true;
			for (size_t i = 0; i < obstacle_flags.size(); ++i) {
				bool obstacle_subcheck = false;
				for (int x = 0; x < w; ++x) {
					for (int y = 0; y < h; ++y) {
						if (CheckObstaclesBetweenTiles(pos + Vec2i(x, y), mpos, obstacle_flags[i], z, max_obstacle_difference)) { //the obstacle must be avoidable from at least one of the unit's tiles
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
			//Wyrmgus end
#ifdef MARKER_ON_INDEX
			//Wyrmgus start
//			marker(player, mpos.x + index);
			marker(player, mpos.x + index, z);
			//Wyrmgus end
#else
			//Wyrmgus start
//			marker(player, mpos);
			marker(player, mpos, z);
			//Wyrmgus end
#endif
			//Wyrmgus start
			if (marker == MapMarkTileOwnership || marker == MapUnmarkTileOwnership) {
				Map.CalculateTileOwnership(mpos, z);
			}
			//Wyrmgus end
		}
	}
}

/**
**  Update fog of war.
*/
void UpdateFogOfWarChange()
{
	DebugPrint("::UpdateFogOfWarChange\n");
	//  Mark all explored fields as visible again.
	if (Map.NoFogOfWar) {
		//Wyrmgus start
		for (size_t z = 0; z < Map.Fields.size(); ++z) {
			const unsigned int w = Map.Info.LayersSizes[z].x * Map.Info.LayersSizes[z].y;
			for (unsigned int index = 0; index != w; ++index) {
				CMapField &mf = *Map.Field(index, z);
				if (mf.playerInfo.IsExplored(*ThisPlayer)) {
					Map.MarkSeenTile(mf, z);
				}
			}
		}
		//Wyrmgus end
	}
	//  Global seen recount.
	for (CUnitManager::Iterator it = UnitManager.begin(); it != UnitManager.end(); ++it) {
		CUnit &unit = **it;
		UnitCountSeen(unit);
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
void VideoDrawOnlyFog(int x, int y)
{
#if defined(USE_OPENGL) || defined(USE_GLES)
	if (UseOpenGL) {
		Video.FillRectangleClip(Video.MapRGBA(0, 0, 0, 0, FogOfWarOpacity),
								x, y, PixelTileSize.x, PixelTileSize.y);
	} else
#endif
	{
		int oldx;
		int oldy;
		SDL_Rect srect;
		SDL_Rect drect;

		srect.x = 0;
		srect.y = 0;
		srect.w = OnlyFogSurface->w;
		srect.h = OnlyFogSurface->h;

		oldx = x;
		oldy = y;
		CLIP_RECTANGLE(x, y, srect.w, srect.h);
		srect.x += x - oldx;
		srect.y += y - oldy;

		drect.x = x;
		drect.y = y;

		SDL_BlitSurface(OnlyFogSurface, &srect, TheScreen, &drect);
	}
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
	int w = Map.Info.LayersSizes[z].x;
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
		unsigned int index = sy - Map.Info.LayersSizes[z].x;//(y-1) * Map.Info.MapWidth;
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
	if (sy + w < Map.Info.LayersSizes[z].y * w) {
	//Wyrmgus end
		//Wyrmgus start
		unsigned int index = sy + Map.Info.LayersSizes[z].x;//(y+1) * Map.Info.MapWidth;
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
	if (sx % 3 == 0 && Map.FogGraphic->Surface->h / 32 >= 3) {
		FogTileVariation = 2;
	} else if (sx % 2 == 0 && Map.FogGraphic->Surface->h / 32 >= 2) {
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
static void DrawFogOfWarTile(int sx, int sy, int dx, int dy)
{
	int fogTile = 0;
	int blackFogTile = 0;

	//Wyrmgus start
//	GetFogOfWarTile(sx, sy, &fogTile, &blackFogTile);
	GetFogOfWarTile(sx, sy, &fogTile, &blackFogTile, CurrentMapLayer);
	//Wyrmgus end

	//Wyrmgus start
//	if (IsMapFieldVisibleTable(sx) || ReplayRevealMap) {
	if ((IsMapFieldVisibleTable(sx, CurrentMapLayer) && blackFogTile != 16 && fogTile != 16) || ReplayRevealMap) {
	//Wyrmgus end
		if (fogTile && fogTile != blackFogTile) {
#if defined(USE_OPENGL) || defined(USE_GLES)
			if (UseOpenGL) {
				Map.FogGraphic->DrawFrameClipTrans(fogTile, dx, dy, FogOfWarOpacity);
			} else
#endif
			{
				AlphaFogG->DrawFrameClip(fogTile, dx, dy);
			}
		}
	} else {
		VideoDrawOnlyFog(dx, dy);
	}
	if (blackFogTile) {
		Map.FogGraphic->DrawFrameClip(blackFogTile, dx, dy);
	}

#undef IsMapFieldExploredTable
#undef IsMapFieldVisibleTable
}

/**
**  Draw the map fog of war.
*/
void CViewport::DrawMapFogOfWar() const
{
	// flags must redraw or not
	if (ReplayRevealMap) {
		return;
	}

	int sx = std::max<int>(MapPos.x - 1, 0);
	//Wyrmgus start
	int ex = std::min<int>(MapPos.x + MapWidth + 1, Map.Info.LayersSizes[CurrentMapLayer].x);
	//Wyrmgus end
	int my = std::max<int>(MapPos.y - 1, 0);
	//Wyrmgus start
	int ey = std::min<int>(MapPos.y + MapHeight + 1, Map.Info.LayersSizes[CurrentMapLayer].y);
	//Wyrmgus end

	// Update for visibility all tile in viewport
	// and 1 tile around viewport (for fog-of-war connection display)

	//Wyrmgus start
	unsigned int my_index = my * Map.Info.LayersSizes[CurrentMapLayer].x;
	//Wyrmgus end
	for (; my < ey; ++my) {
		for (int mx = sx; mx < ex; ++mx) {
			//Wyrmgus start
//			VisibleTable[my_index + mx] = Map.Field(mx + my_index)->playerInfo.TeamVisibilityState(*ThisPlayer);
			VisibleTable[CurrentMapLayer][my_index + mx] = Map.Field(mx + my_index, CurrentMapLayer)->playerInfo.TeamVisibilityState(*ThisPlayer);
			//Wyrmgus end
		}
		//Wyrmgus start
		my_index += Map.Info.LayersSizes[CurrentMapLayer].x;
		//Wyrmgus end
	}
	ex = this->BottomRightPos.x;
	//Wyrmgus start
	int sy = MapPos.y * Map.Info.LayersSizes[CurrentMapLayer].x;
	//Wyrmgus end
	int dy = this->TopLeftPos.y - Offset.y;
	ey = this->BottomRightPos.y;

	while (dy <= ey) {
		sx = MapPos.x + sy;
		int dx = this->TopLeftPos.x - Offset.x;
		while (dx <= ex) {
			//Wyrmgus start
//			if (VisibleTable[sx]) {
			if (VisibleTable[CurrentMapLayer][sx]) {
			//Wyrmgus end
				DrawFogOfWarTile(sx, sy, dx, dy);
			} else {
				Video.FillRectangleClip(FogOfWarColorSDL, dx, dy, PixelTileSize.x, PixelTileSize.y);
			}
			++sx;
			dx += PixelTileSize.x;
		}
		//Wyrmgus start
		sy += Map.Info.LayersSizes[CurrentMapLayer].x;
		//Wyrmgus end
		dy += PixelTileSize.y;
	}
}

/**
**  Initialize the fog of war.
**  Build tables, setup functions.
*/
void CMap::InitFogOfWar()
{
	//calculate this once from the settings and store it
	FogOfWarColorSDL = Video.MapRGB(TheScreen->format, FogOfWarColor);

	Uint8 r, g, b;
	SDL_Surface *s;

	FogGraphic->Load();

#if defined(USE_OPENGL) || defined(USE_GLES)
	if (!UseOpenGL)
#endif
	{
		//
		// Generate Only Fog surface.
		//
		s = SDL_CreateRGBSurface(SDL_SWSURFACE, PixelTileSize.x, PixelTileSize.y,
								 32, RMASK, GMASK, BMASK, AMASK);

		SDL_GetRGB(FogOfWarColorSDL, TheScreen->format, &r, &g, &b);
		Uint32 color = Video.MapRGB(s->format, r, g, b);

		SDL_FillRect(s, NULL, color);
		OnlyFogSurface = SDL_DisplayFormat(s);
		SDL_SetAlpha(OnlyFogSurface, SDL_SRCALPHA | SDL_RLEACCEL, FogOfWarOpacity);
		VideoPaletteListRemove(s);
		SDL_FreeSurface(s);

		//
		// Generate Alpha Fog surface.
		//
		if (FogGraphic->Surface->format->BytesPerPixel == 1) {
			s = SDL_DisplayFormat(FogGraphic->Surface);
			SDL_SetAlpha(s, SDL_SRCALPHA | SDL_RLEACCEL, FogOfWarOpacity);
		} else {
			// Copy the top row to a new surface
			SDL_PixelFormat *f = FogGraphic->Surface->format;
			//Wyrmgus start
//			s = SDL_CreateRGBSurface(SDL_SWSURFACE, FogGraphic->Surface->w, PixelTileSize.y,
			s = SDL_CreateRGBSurface(SDL_SWSURFACE, FogGraphic->Surface->w, FogGraphic->Surface->h,
			//Wyrmgus end
									 f->BitsPerPixel, f->Rmask, f->Gmask, f->Bmask, f->Amask);
			SDL_LockSurface(s);
			SDL_LockSurface(FogGraphic->Surface);
			for (int i = 0; i < s->h; ++i) {
				memcpy(reinterpret_cast<Uint8 *>(s->pixels) + i * s->pitch,
					   reinterpret_cast<Uint8 *>(FogGraphic->Surface->pixels) + i * FogGraphic->Surface->pitch,
					   FogGraphic->Surface->w * f->BytesPerPixel);
			}
			SDL_UnlockSurface(s);
			SDL_UnlockSurface(FogGraphic->Surface);

			// Convert any non-transparent pixels to use FogOfWarOpacity as alpha
			SDL_LockSurface(s);
			for (int j = 0; j < s->h; ++j) {
				for (int i = 0; i < s->w; ++i) {
					Uint32 c = *reinterpret_cast<Uint32 *>(&reinterpret_cast<Uint8 *>(s->pixels)[i * 4 + j * s->pitch]);
					Uint8 a;

					Video.GetRGBA(c, s->format, &r, &g, &b, &a);
					if (a) {
						//Wyrmgus start
//						c = Video.MapRGBA(s->format, r, g, b, FogOfWarOpacity);
						if (a >= 255) {
							c = Video.MapRGBA(s->format, r, g, b, FogOfWarOpacity);
						} else {
							c = Video.MapRGBA(s->format, r, g, b, a / (256 / FogOfWarOpacity));
						}
						//Wyrmgus end
						*reinterpret_cast<Uint32 *>(&reinterpret_cast<Uint8 *>(s->pixels)[i * 4 + j * s->pitch]) = c;
					}
				}
			}
			SDL_UnlockSurface(s);
		}
		AlphaFogG = CGraphic::New("");
		AlphaFogG->Surface = s;
		AlphaFogG->Width = PixelTileSize.x;
		AlphaFogG->Height = PixelTileSize.y;
		AlphaFogG->GraphicWidth = s->w;
		AlphaFogG->GraphicHeight = s->h;
		//Wyrmgus start
//		AlphaFogG->NumFrames = 16;//1;
		AlphaFogG->NumFrames = 16 * (s->h / 32);//1;
		//Wyrmgus end
		AlphaFogG->GenFramesMap();
		AlphaFogG->UseDisplayFormat();
	}

	//Wyrmgus start
	VisibleTable.clear();
	VisibleTable.resize(this->Fields.size());
	for (size_t z = 0; z < Map.Fields.size(); ++z) {
		VisibleTable[z].resize(Info.LayersSizes[z].x * Info.LayersSizes[z].y);
	}
	//Wyrmgus end
}

/**
**  Cleanup the fog of war.
*/
void CMap::CleanFogOfWar()
{
	VisibleTable.clear();

	CGraphic::Free(Map.FogGraphic);
	FogGraphic = NULL;

#if defined(USE_OPENGL) || defined(USE_GLES)
	if (!UseOpenGL)
#endif
	{
		if (OnlyFogSurface) {
			VideoPaletteListRemove(OnlyFogSurface);
			SDL_FreeSurface(OnlyFogSurface);
			OnlyFogSurface = NULL;
		}
		CGraphic::Free(AlphaFogG);
		AlphaFogG = NULL;
	}
}

//@}
