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

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "map/map.h"

#include "action/actions.h"
#include "map/map_layer.h"
#include "map/minimap.h"
#include "map/tileset.h"
#include "player.h"
#include "ui/ui.h"
#include "unit/unit.h"
//Wyrmgus start
#include "unit/unit_find.h"
//Wyrmgus end
#include "unit/unit_manager.h"
#include "video/video.h"
#include "video/intern_video.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

int FogOfWarOpacity;                 /// Fog of war Opacity.
Uint32 FogOfWarColorSDL;
CColor FogOfWarColor;

std::map<PixelSize, CGraphic *> CMap::FogGraphics;

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

static std::map<PixelSize, SDL_Surface *> OnlyFogSurfaces;
static std::map<PixelSize, CGraphic *> AlphaFogGraphics;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

class _filter_flags
{
public:
	_filter_flags(const CPlayer &p, int *fogmask) : player(&p), fogmask(fogmask)
	{
		Assert(fogmask != nullptr);
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
	CMap::Map.Field(index, z)->UnitCache.for_each(filter);
	return fogMask;
}

int MapFogFilterFlags(CPlayer &player, const Vec2i &pos, int mask, int z)
{
	if (CMap::Map.Info.IsPointOnMap(pos, z)) {
		return MapFogFilterFlags(player, CMap::Map.getIndex(pos, z), mask, z);
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
		const int p = player->GetIndex();
		if (MARK) {
			//  If the unit goes out of fog, this can happen for any player that
			//  this player shares vision with, and can't YET see the unit.
			//  It will be able to see the unit after the Unit->VisCount ++
			if (!unit->VisCount[p]) {
				for (int pi = 0; pi < PlayerMax; ++pi) {
					if ((pi == p /*player->GetIndex()*/)
						|| player->IsBothSharedVision(*CPlayer::Players[pi]) || player->Revealed) {
						if (!unit->IsVisible(*CPlayer::Players[pi])) {
							UnitGoesOutOfFog(*unit, *CPlayer::Players[pi]);
						}
					}
				}
			}
			unit->VisCount[p/*player->GetIndex()*/]++;
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
					if (pi == p/*player->GetIndex()*/ ||
						player->IsBothSharedVision(*CPlayer::Players[pi]) || player->Revealed) {
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
void MapMarkTileSight(const CPlayer &player, const unsigned int index, int z)
{
	CMapField &mf = *CMap::Map.Field(index, z);
	unsigned short *v = &(mf.playerInfo.Visible[player.GetIndex()]);
	if (*v == 0 || *v == 1) { // Unexplored or unseen
		// When there is no fog only unexplored tiles are marked.
		if (!CMap::Map.NoFogOfWar || *v == 0) {
			UnitsOnTileMarkSeen(player, mf, 0, 0);
		}
		*v = 2;
		if (mf.playerInfo.IsTeamVisible(*CPlayer::GetThisPlayer())) {
			CMap::Map.MarkSeenTile(mf, z);
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
	Assert(CMap::Map.Info.IsPointOnMap(pos, z));
	MapMarkTileSight(player, CMap::Map.getIndex(pos, z), z);
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
	CMapField &mf = *CMap::Map.Field(index, z);
	unsigned short *v = &mf.playerInfo.Visible[player.GetIndex()];
	switch (*v) {
		case 0:  // Unexplored
		case 1:
			// This happens when we unmark everything in CommandSharedVision
			break;
		case 2:
			// When there is NoFogOfWar units never get unmarked.
			if (!CMap::Map.NoFogOfWar) {
				UnitsOnTileUnmarkSeen(player, mf, 0, 0);
			}
			// Check visible Tile, then deduct...
			if (mf.playerInfo.IsTeamVisible(*CPlayer::GetThisPlayer())) {
				CMap::Map.MarkSeenTile(mf, z);
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
	Assert(CMap::Map.Info.IsPointOnMap(pos, z));
	MapUnmarkTileSight(player, CMap::Map.getIndex(pos, z), z);
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
	CMapField &mf = *CMap::Map.Field(index, z);
	unsigned char *v = &mf.playerInfo.VisCloak[player.GetIndex()];
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
//	MapMarkTileDetectCloak(player, CMap::Map.getIndex(pos));
	MapMarkTileDetectCloak(player, CMap::Map.getIndex(pos, z), z);
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
	CMapField &mf = *CMap::Map.Field(index, z);
	unsigned char *v = &mf.playerInfo.VisCloak[player.GetIndex()];
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
//	MapUnmarkTileDetectCloak(player, CMap::Map.getIndex(pos));
	MapUnmarkTileDetectCloak(player, CMap::Map.getIndex(pos, z), z);
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
	CMapField &mf = *CMap::Map.Field(index, z);
	unsigned char *v = &mf.playerInfo.VisEthereal[player.GetIndex()];
	if (*v == 0) {
		UnitsOnTileMarkSeen(player, mf, 0, 1);
	}
	Assert(*v != 255);
	++*v;
}

void MapMarkTileDetectEthereal(const CPlayer &player, const Vec2i &pos, int z)
{
	MapMarkTileDetectEthereal(player, CMap::Map.getIndex(pos, z), z);
}

/**
**  Unmark a tile for ethereal detection.
**
**  @param player  Player to mark sight.
**  @param index   tile to mark.
*/
void MapUnmarkTileDetectEthereal(const CPlayer &player, const unsigned int index, int z)
{
	CMapField &mf = *CMap::Map.Field(index, z);
	unsigned char *v = &mf.playerInfo.VisEthereal[player.GetIndex()];
	Assert(*v != 0);
	if (*v == 1) {
		UnitsOnTileUnmarkSeen(player, mf, 0, 1);
	}
	--*v;
}

void MapUnmarkTileDetectEthereal(const CPlayer &player, const Vec2i &pos, int z)
{
	MapUnmarkTileDetectEthereal(player, CMap::Map.getIndex(pos, z), z);
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
		if (CMap::Map.IsLayerUnderground(z)) {
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
//		const int maxx = std::min(CMap::Map.Info.MapWidth, pos.x + w + offsetx);
		const int maxx = std::min(CMap::Map.Info.MapWidths[z], pos.x + w + offsetx);
		//Wyrmgus end
		Vec2i mpos(minx, pos.y + offsety);
#ifdef MARKER_ON_INDEX
		//Wyrmgus start
//		const unsigned int index = mpos.y * CMap::Map.Info.MapWidth;
		const unsigned int index = mpos.y * CMap::Map.Info.MapWidths[z];
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
				CMap::Map.CalculateTileOwnership(mpos, z);
			}
			//Wyrmgus end
		}
	}
	for (int offsety = 0; offsety < h; ++offsety) {
		const int minx = std::max(0, pos.x - range);
		//Wyrmgus start
//		const int maxx = std::min(CMap::Map.Info.MapWidth, pos.x + w + range);
		const int maxx = std::min(CMap::Map.Info.MapWidths[z], pos.x + w + range);
		//Wyrmgus end
		Vec2i mpos(minx, pos.y + offsety);
#ifdef MARKER_ON_INDEX
		//Wyrmgus start
//		const unsigned int index = mpos.y * CMap::Map.Info.MapWidth;
		const unsigned int index = mpos.y * CMap::Map.Info.MapWidths[z];
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
				CMap::Map.CalculateTileOwnership(mpos, z);
			}
			//Wyrmgus end
		}
	}
	// bottom hemi-cycle
	//Wyrmgus start
//	const int maxy = std::min(range, CMap::Map.Info.MapHeight - pos.y - h);
	const int maxy = std::min(range, CMap::Map.Info.MapHeights[z] - pos.y - h);
	//Wyrmgus end
	for (int offsety = 0; offsety < maxy; ++offsety) {
		const int offsetx = isqrt(square(range + 1) - square(offsety + 1) - 1);
		const int minx = std::max(0, pos.x - offsetx);
		//Wyrmgus start
//		const int maxx = std::min(CMap::Map.Info.MapWidth, pos.x + w + offsetx);
		const int maxx = std::min(CMap::Map.Info.MapWidths[z], pos.x + w + offsetx);
		//Wyrmgus end
		Vec2i mpos(minx, pos.y + h + offsety);
#ifdef MARKER_ON_INDEX
		//Wyrmgus start
//		const unsigned int index = mpos.y * CMap::Map.Info.MapWidth;
		const unsigned int index = mpos.y * CMap::Map.Info.MapWidths[z];
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
				CMap::Map.CalculateTileOwnership(mpos, z);
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
	if (CMap::Map.NoFogOfWar) {
		for (const CMapLayer *map_layer : CMap::Map.MapLayers) {
			const unsigned int w = map_layer->GetHeight() * map_layer->GetWidth();
			for (unsigned int index = 0; index != w; ++index) {
				CMapField &mf = *map_layer->Field(index);
				if (mf.playerInfo.IsExplored(*CPlayer::GetThisPlayer())) {
					CMap::Map.MarkSeenTile(mf, map_layer->ID);
				}
			}
		}
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
								x, y, CMap::Map.GetCurrentPixelTileSize().x, CMap::Map.GetCurrentPixelTileSize().y);
	} else
#endif
	{
		SDL_Surface *only_fog_surface = OnlyFogSurfaces[CMap::Map.GetCurrentPixelTileSize()];
		
		int oldx;
		int oldy;
		SDL_Rect srect;
		SDL_Rect drect;

		srect.x = 0;
		srect.y = 0;
		srect.w = only_fog_surface->w;
		srect.h = only_fog_surface->h;

		oldx = x;
		oldy = y;
		CLIP_RECTANGLE(x, y, srect.w, srect.h);
		srect.x += x - oldx;
		srect.y += y - oldy;

		drect.x = x;
		drect.y = y;

		SDL_BlitSurface(only_fog_surface, &srect, TheScreen, &drect);
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
//	int w = CMap::Map.Info.MapWidth;
	int w = CMap::Map.Info.MapWidths[z];
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
//		unsigned int index = sy - CMap::Map.Info.MapWidth;//(y-1) * CMap::Map.Info.MapWidth;
		unsigned int index = sy - CMap::Map.Info.MapWidths[z];//(y-1) * CMap::Map.Info.MapWidth;
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
		unsigned int index = sy;//(y) * CMap::Map.Info.MapWidth;
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
		unsigned int index = sy;//(y) * CMap::Map.Info.MapWidth;
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
//	if (sy + w < CMap::Map.Info.MapHeight * w) {
	if (sy + w < CMap::Map.Info.MapHeights[z] * w) {
	//Wyrmgus end
		//Wyrmgus start
//		unsigned int index = sy + CMap::Map.Info.MapWidth;//(y+1) * CMap::Map.Info.MapWidth;
		unsigned int index = sy + CMap::Map.Info.MapWidths[z];//(y+1) * CMap::Map.Info.MapWidth;
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
	CGraphic *fog_graphic = CMap::Map.FogGraphics[CMap::Map.GetCurrentPixelTileSize()];
	if (sx % 3 == 0 && fog_graphic->Surface->h / CMap::Map.GetCurrentPixelTileSize().y >= 3) {
		FogTileVariation = 2;
	} else if (sx % 2 == 0 && fog_graphic->Surface->h / CMap::Map.GetCurrentPixelTileSize().y >= 2) {
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
	GetFogOfWarTile(sx, sy, &fogTile, &blackFogTile, UI.CurrentMapLayer->ID);
	//Wyrmgus end

	//Wyrmgus start
	CGraphic *fog_graphic = CMap::Map.FogGraphics[CMap::Map.GetCurrentPixelTileSize()];
	CGraphic *alpha_fog_graphic = AlphaFogGraphics[CMap::Map.GetCurrentPixelTileSize()];
	
//	if (IsMapFieldVisibleTable(sx) || ReplayRevealMap) {
	if ((IsMapFieldVisibleTable(sx, UI.CurrentMapLayer->ID) && blackFogTile != 16 && fogTile != 16) || ReplayRevealMap) {
	//Wyrmgus end
		if (fogTile && fogTile != blackFogTile) {
#if defined(USE_OPENGL) || defined(USE_GLES)
			if (UseOpenGL) {
				fog_graphic->DrawFrameClipTrans(fogTile, dx, dy, FogOfWarOpacity);
			} else
#endif
			{
				alpha_fog_graphic->DrawFrameClip(fogTile, dx, dy);
			}
		}
	} else {
		VideoDrawOnlyFog(dx, dy);
	}
	if (blackFogTile) {
		fog_graphic->DrawFrameClip(blackFogTile, dx, dy);
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
	int ex = std::min<int>(MapPos.x + MapWidth + 1, UI.CurrentMapLayer->GetWidth());
	int my = std::max<int>(MapPos.y - 1, 0);
	int ey = std::min<int>(MapPos.y + MapHeight + 1, UI.CurrentMapLayer->GetHeight());

	// Update for visibility all tile in viewport
	// and 1 tile around viewport (for fog-of-war connection display)

	unsigned int my_index = my * UI.CurrentMapLayer->GetWidth();
	for (; my < ey; ++my) {
		for (int mx = sx; mx < ex; ++mx) {
			//Wyrmgus start
//			VisibleTable[my_index + mx] = CMap::Map.Field(mx + my_index)->playerInfo.TeamVisibilityState(*CPlayer::GetThisPlayer());
			VisibleTable[UI.CurrentMapLayer->ID][my_index + mx] = CMap::Map.Field(mx + my_index, UI.CurrentMapLayer->ID)->playerInfo.TeamVisibilityState(*CPlayer::GetThisPlayer());
			//Wyrmgus end
		}
		my_index += UI.CurrentMapLayer->GetWidth();
	}
	ex = this->BottomRightPos.x;
	int sy = MapPos.y * UI.CurrentMapLayer->GetWidth();
	int dy = this->TopLeftPos.y - Offset.y;
	ey = this->BottomRightPos.y;

	while (dy <= ey) {
		sx = MapPos.x + sy;
		int dx = this->TopLeftPos.x - Offset.x;
		while (dx <= ex) {
			//Wyrmgus start
//			if (VisibleTable[sx]) {
			if (VisibleTable[UI.CurrentMapLayer->ID][sx]) {
			//Wyrmgus end
				DrawFogOfWarTile(sx, sy, dx, dy);
			} else {
				Video.FillRectangleClip(FogOfWarColorSDL, dx, dy, CMap::Map.GetCurrentPixelTileSize().x, CMap::Map.GetCurrentPixelTileSize().y);
			}
			++sx;
			dx += CMap::Map.GetCurrentPixelTileSize().x;
		}
		sy += UI.CurrentMapLayer->GetWidth();
		dy += CMap::Map.GetCurrentPixelTileSize().y;
	}
}

/**
**  Initialize the fog of war.
**  Build tables, setup functions.
*/
void CMap::InitFogOfWar(PixelSize pixel_tile_size)
{
	CGraphic *fog_graphic = this->FogGraphics[pixel_tile_size];
	
	//calculate this once from the settings and store it
	FogOfWarColorSDL = Video.MapRGB(TheScreen->format, FogOfWarColor);

	Uint8 r, g, b;
	SDL_Surface *s;

	fog_graphic->Load();

#if defined(USE_OPENGL) || defined(USE_GLES)
	if (!UseOpenGL)
#endif
	{
		//
		// Generate Only Fog surface.
		//
		s = SDL_CreateRGBSurface(SDL_SWSURFACE, pixel_tile_size.x, pixel_tile_size.y,
								 32, RMASK, GMASK, BMASK, AMASK);

		SDL_GetRGB(FogOfWarColorSDL, TheScreen->format, &r, &g, &b);
		Uint32 color = Video.MapRGB(s->format, r, g, b);

		SDL_FillRect(s, nullptr, color);
		SDL_Surface *only_fog_surface = SDL_DisplayFormat(s);
		SDL_SetAlpha(only_fog_surface, SDL_SRCALPHA | SDL_RLEACCEL, FogOfWarOpacity);
		VideoPaletteListRemove(s);
		SDL_FreeSurface(s);
		
		OnlyFogSurfaces[pixel_tile_size] = only_fog_surface;

		//
		// Generate Alpha Fog surface.
		//
		if (fog_graphic->Surface->format->BytesPerPixel == 1) {
			s = SDL_DisplayFormat(fog_graphic->Surface);
			SDL_SetAlpha(s, SDL_SRCALPHA | SDL_RLEACCEL, FogOfWarOpacity);
		} else {
			// Copy the top row to a new surface
			SDL_PixelFormat *f = fog_graphic->Surface->format;
			//Wyrmgus start
//			s = SDL_CreateRGBSurface(SDL_SWSURFACE, fog_graphic->Surface->w, pixel_tile_size.y,
			s = SDL_CreateRGBSurface(SDL_SWSURFACE, fog_graphic->Surface->w, fog_graphic->Surface->h,
			//Wyrmgus end
									 f->BitsPerPixel, f->Rmask, f->Gmask, f->Bmask, f->Amask);
			SDL_LockSurface(s);
			SDL_LockSurface(fog_graphic->Surface);
			for (int i = 0; i < s->h; ++i) {
				memcpy(reinterpret_cast<Uint8 *>(s->pixels) + i * s->pitch,
					   reinterpret_cast<Uint8 *>(fog_graphic->Surface->pixels) + i * fog_graphic->Surface->pitch,
					   fog_graphic->Surface->w * f->BytesPerPixel);
			}
			SDL_UnlockSurface(s);
			SDL_UnlockSurface(fog_graphic->Surface);

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
		CGraphic *alpha_fog_graphic = CGraphic::New("");
		alpha_fog_graphic->Surface = s;
		alpha_fog_graphic->Width = pixel_tile_size.x;
		alpha_fog_graphic->Height = pixel_tile_size.y;
		alpha_fog_graphic->GraphicWidth = s->w;
		alpha_fog_graphic->GraphicHeight = s->h;
		//Wyrmgus start
//		alpha_fog_graphic->NumFrames = 16;//1;
		alpha_fog_graphic->NumFrames = 16 * (s->h / pixel_tile_size.y);//1;
		//Wyrmgus end
		alpha_fog_graphic->GenFramesMap();
		alpha_fog_graphic->UseDisplayFormat();
		AlphaFogGraphics[pixel_tile_size] = alpha_fog_graphic;
	}

	//Wyrmgus start
//	VisibleTable.clear();
//	VisibleTable.resize(Info.MapWidth * Info.MapHeight);
	VisibleTable.clear();
	VisibleTable.resize(this->MapLayers.size());
	for (size_t z = 0; z < CMap::Map.MapLayers.size(); ++z) {
		VisibleTable[z].resize(Info.MapWidths[z] * Info.MapHeights[z]);
	}
	//Wyrmgus end
}

/**
**  Cleanup the fog of war.
*/
void CMap::CleanFogOfWar()
{
	VisibleTable.clear();

	for (std::map<PixelSize, CGraphic *>::iterator iterator = this->FogGraphics.begin(); iterator != this->FogGraphics.end(); ++iterator) {
		CGraphic::Free(iterator->second);
		iterator->second = nullptr;
		
#if defined(USE_OPENGL) || defined(USE_GLES)
		if (!UseOpenGL)
#endif
		{
			if (OnlyFogSurfaces.find(iterator->first) != OnlyFogSurfaces.end()) {
				VideoPaletteListRemove(OnlyFogSurfaces[iterator->first]);
				SDL_FreeSurface(OnlyFogSurfaces[iterator->first]);
				OnlyFogSurfaces[iterator->first] = nullptr;
			}
			CGraphic::Free(AlphaFogGraphics[iterator->first]);
			AlphaFogGraphics[iterator->first] = nullptr;
		}
	}
	
	this->FogGraphics.clear();
	OnlyFogSurfaces.clear();
	AlphaFogGraphics.clear();
}
