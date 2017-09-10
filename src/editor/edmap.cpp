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
/**@name edmap.cpp - Editor map functions. */
//
//      (c) Copyright 2002-2005 by Lutz Sammer
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
#include "editor.h"
#include "map.h"
#include "tileset.h"
#include "ui.h"
#include "player.h"
#include "unit.h"
#include "unit_manager.h"
#include "unittype.h"


/// Callback for changed tile (with direction mask)
static void EditorChangeSurrounding(const Vec2i &pos, int d);

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**  Change tile from abstract tile-type.
**
**  @param pos   map tile coordinate.
**  @param tile  tile type to edit.
**
**  @note  this is a rather dumb function, doesn't do any tile fixing.
*/
//Wyrmgus start
/*
void ChangeTile(const Vec2i &pos, int tile)
{
	Assert(Map.Info.IsPointOnMap(pos));

	CMapField &mf = *Map.Field(pos);
	mf.setGraphicTile(tile);
	mf.playerInfo.SeenTile = tile;
	mf.UpdateSeenTile();
}
*/
//Wyrmgus end


/**
**  Get quad from tile.
**
**  A quad is a 32 bit value defining the content of the tile.
**
**  A tile is split into 4 parts, the basic tile type of this part
**    is stored as 8bit value in the quad.
**
**  ab
**  cd -> abcd
*/
//Wyrmgus start
/*
static unsigned QuadFromTile(const Vec2i &pos)
{
	// find the abstact tile number
	const int tile = Map.Field(pos)->getGraphicTile();
	return Map.Tileset->getQuadFromTile(tile);
}
*/
//Wyrmgus end

/**
**  Editor change tile.
**
**  @param pos   map tile coordinate.
**  @param tileIndex  Tile type to edit.
**  @param d     Fix direction flag 8 up, 4 down, 2 left, 1 right.
*/
//Wyrmgus start
//void EditorChangeTile(const Vec2i &pos, int tileIndex, int d)
void EditorChangeTile(const Vec2i &pos, int tileIndex)
//Wyrmgus end
{
	//Wyrmgus start
//	Assert(Map.Info.IsPointOnMap(pos));
	Assert(Map.Info.IsPointOnMap(pos, CurrentMapLayer));
	//Wyrmgus end

	// Change the flags
	//Wyrmgus start
//	CMapField &mf = *Map.Field(pos);
	CMapField &mf = *Map.Field(pos, CurrentMapLayer);
	//Wyrmgus end
	int tile = tileIndex;
	if (TileToolRandom) {
		int n = 0;
		for (int i = 0; i < 16; ++i) {
			if (!Map.Tileset->tiles[tile + i].tile) {
				break;
			} else {
				++n;
			}
		}
		n = MyRand() % n;
		int i = -1;
		do {
			while (++i < 16 && !Map.Tileset->tiles[tile + i].tile) {
			}
		} while (i < 16 && n--);
		Assert(i != 16);
		tile += i;
	}
	//Wyrmgus start
	mf.setTileIndex(*Map.Tileset, tile, 0);
//	mf.playerInfo.SeenTile = mf.getGraphicTile();
	mf.UpdateSeenTile();
	//Wyrmgus end
	
	//Wyrmgus start
	Map.CalculateTileTransitions(pos, false, CurrentMapLayer);
	Map.CalculateTileTransitions(pos, true, CurrentMapLayer);
	
	for (int x_offset = -1; x_offset <= 1; ++x_offset) {
		for (int y_offset = -1; y_offset <= 1; ++y_offset) {
			if (x_offset != 0 || y_offset != 0) {
				Vec2i adjacent_pos(pos.x + x_offset, pos.y + y_offset);
				if (Map.Info.IsPointOnMap(adjacent_pos, CurrentMapLayer)) {
					Map.CalculateTileTransitions(adjacent_pos, false, CurrentMapLayer);
					Map.CalculateTileTransitions(adjacent_pos, true, CurrentMapLayer);
				}
			}
		}
	}
	//Wyrmgus end
	
	//Wyrmgus start
	CUnitCache &unitcache = mf.UnitCache;
	std::vector<CUnit *> units_to_remove;

	for (CUnitCache::iterator it = unitcache.begin(); it != unitcache.end(); ++it) {
		CUnit *unit = *it;

		if (!CanBuildUnitType(unit, *unit->Type, pos, 1, true, CurrentMapLayer)) {
			units_to_remove.push_back(unit);
		}
	}
	
	for (size_t i = 0; i < units_to_remove.size(); ++i) {
		EditorActionRemoveUnit(*units_to_remove[i], false);
	}
	//Wyrmgus end

	UI.Minimap.UpdateSeenXY(pos);
	//Wyrmgus start
//	UI.Minimap.UpdateXY(pos);
	UI.Minimap.UpdateXY(pos, CurrentMapLayer);
	//Wyrmgus end

	//Wyrmgus start
//	EditorChangeSurrounding(pos, d);
	EditorChangeSurrounding(pos, tile);
	//Wyrmgus end
}

/**
**  Update surroundings for tile changes.
**
**  @param pos  Map tile position of change.
**  @param d  Fix direction flag 8 up, 4 down, 2 left, 1 right.
*/
//Wyrmgus start
//static void EditorChangeSurrounding(const Vec2i &pos, int d)
static void EditorChangeSurrounding(const Vec2i &pos, int tile)
//Wyrmgus end
{
	// Special case 1) Walls.
	//Wyrmgus start
//	CMapField &mf = *Map.Field(pos);
	CMapField &mf = *Map.Field(pos, CurrentMapLayer);
	//Wyrmgus end
	
	//Wyrmgus start
	//see if the tile's terrain can be here as is, or if it is needed to change surrounding tiles
	CTerrainType *terrain = Map.GetTileTopTerrain(pos, false, CurrentMapLayer);
	bool overlay = mf.OverlayTerrain ? true : false;
	if (!terrain->AllowSingle) {
		std::vector<int> transition_directions;
		
		for (int x_offset = -1; x_offset <= 1; ++x_offset) {
			for (int y_offset = -1; y_offset <= 1; ++y_offset) {
				if (x_offset != 0 || y_offset != 0) {
					Vec2i adjacent_pos(pos.x + x_offset, pos.y + y_offset);
					if (Map.Info.IsPointOnMap(adjacent_pos, CurrentMapLayer)) {
						CMapField &adjacent_mf = *Map.Field(adjacent_pos, CurrentMapLayer);
							
						CTerrainType *adjacent_terrain = Map.GetTileTerrain(adjacent_pos, overlay, CurrentMapLayer);
						if (overlay && adjacent_terrain && Map.Field(adjacent_pos, CurrentMapLayer)->OverlayTerrainDestroyed) {
							adjacent_terrain = NULL;
						}
						if (terrain != adjacent_terrain) { // also happens if terrain is NULL, so that i.e. tree transitions display correctly when adjacent to tiles without overlays
							transition_directions.push_back(GetDirectionFromOffset(x_offset, y_offset));
						}
					}
				}
			}
		}
		
		if (std::find(transition_directions.begin(), transition_directions.end(), North) != transition_directions.end() && std::find(transition_directions.begin(), transition_directions.end(), South) != transition_directions.end()) {
			EditorChangeTile(pos + Vec2i(0, -1), tile);
			EditorChangeTile(pos + Vec2i(0, 1), tile);
		} else if (std::find(transition_directions.begin(), transition_directions.end(), West) != transition_directions.end() && std::find(transition_directions.begin(), transition_directions.end(), East) != transition_directions.end()) {
			EditorChangeTile(pos + Vec2i(-1, 0), tile);
			EditorChangeTile(pos + Vec2i(1, 0), tile);
		}
		
	}
	//Wyrmgus end
}

/**
**  Update surroundings for tile changes.
**
**  @param pos  Map tile position of change.
*/
//Wyrmgus start
//void EditorTileChanged(const Vec2i &pos)
void EditorTileChanged(const Vec2i &pos, int tile)
//Wyrmgus end
{
	//Wyrmgus start
//	EditorChangeSurrounding(pos, 0x0F);
	EditorChangeSurrounding(pos, tile);
	//Wyrmgus end
}

/**
**  Make random map
**  FIXME: vladi: we should have parameters control here...
*/

/**
**  TileFill
**
**  @param pos   map tile coordinate for area center.
**  @param tile  Tile type to edit.
**  @param size  Size of surrounding rectangle.
**
**  TileFill(centerx, centery, tile_type_water, map_width)
**  will fill map with water...
*/
static void TileFill(const Vec2i &pos, int tile, int size)
{
	const Vec2i diag(size / 2, size / 2);
	Vec2i ipos = pos - diag;
	Vec2i apos = pos + diag;

	//Wyrmgus start
//	Map.FixSelectionArea(ipos, apos);
	Map.FixSelectionArea(ipos, apos, CurrentMapLayer);
	//Wyrmgus end

	Vec2i itPos;
	for (itPos.x = ipos.x; itPos.x <= apos.x; ++itPos.x) {
		for (itPos.y = ipos.y; itPos.y <= apos.y; ++itPos.y) {
			//Wyrmgus start
//			EditorChangeTile(itPos, tile, 15);
			EditorChangeTile(itPos, tile);
			//Wyrmgus end
		}
	}
}

/**
**  Randomize tiles and fill in map
**
**  @param tile      tile number to use
**  @param count     number of times to apply randomization
**  @param max_size  maximum size of the fill rectangle
*/
static void EditorRandomizeTile(int tile, int count, int max_size)
{
	//Wyrmgus start
	const Vec2i mpos(Map.Info.LayersSizes[CurrentMapLayer].x - 1, Map.Info.LayersSizes[CurrentMapLayer].y - 1);
	//Wyrmgus end

	for (int i = 0; i < count; ++i) {
		const Vec2i rpos(rand() % ((1 + mpos.x) / 2), rand() % ((1 + mpos.y) / 2));
		const Vec2i mirror = mpos - rpos;
		const Vec2i mirrorh(rpos.x, mirror.y);
		const Vec2i mirrorv(mirror.x, rpos.y);
		const int rz = rand() % max_size + 1;

		TileFill(rpos, tile, rz);
		TileFill(mirrorh, tile, rz);
		TileFill(mirrorv, tile, rz);
		TileFill(mirror, tile, rz);
	}
}

#define WATER_TILE  0x10
#define COAST_TILE  0x30
#define GRASS_TILE  0x50
#define WOOD_TILE   0x70
#define ROCK_TILE   0x80

/**
**  Add a unit to random locations on the map, unit will be neutral
**
**  @param unit_type  unit type to add to map as a character string
**  @param count      the number of times to add the unit
**  @param value      resources to be stored in that unit
*/
static void EditorRandomizeUnit(const char *unit_type, int count, int value)
{
	//Wyrmgus start
	const Vec2i mpos(Map.Info.LayersSizes[CurrentMapLayer].x, Map.Info.LayersSizes[CurrentMapLayer].y);
	//Wyrmgus end
	CUnitType *typeptr = UnitTypeByIdent(unit_type);

	if (!typeptr) { // Error
		return;
	}
	CUnitType &type = *typeptr;
	const Vec2i tpos(type.TileSize);

	for (int i = 0; i < count; ++i) {
		const Vec2i rpos(rand() % (mpos.x / 2 - tpos.x + 1), rand() % (mpos.y / 2 - tpos.y + 1));
		const Vec2i mirror(mpos.x - rpos.x - 1, mpos.y - rpos.y - 1);
		const Vec2i mirrorh(rpos.x, mirror.y);
		const Vec2i mirrorv(mirror.x, rpos.y);
		const Vec2i tmirror(mpos.x - rpos.x - tpos.x, mpos.y - rpos.y - tpos.y);
		const Vec2i tmirrorh(rpos.x, tmirror.y);
		const Vec2i tmirrorv(tmirror.x, rpos.y);
		int tile = GRASS_TILE;
		const int z = type.TileSize.y;

		// FIXME: vladi: the idea is simple: make proper land for unit(s) :)
		// FIXME: handle units larger than 1 square
		TileFill(rpos, tile, z * 2);
		TileFill(mirrorh, tile, z * 2);
		TileFill(mirrorv, tile, z * 2);
		TileFill(mirror, tile, z * 2);

		// FIXME: can overlap units
		//Wyrmgus start
//		CUnit *unit = MakeUnitAndPlace(rpos, type, &Players[PlayerNumNeutral]);
		CUnit *unit = MakeUnitAndPlace(rpos, type, &Players[PlayerNumNeutral], CurrentMapLayer);
		//Wyrmgus end
		if (unit == NULL) {
			DebugPrint("Unable to allocate Unit");
		} else {
			//Wyrmgus start
//			unit->ResourcesHeld = value;
			unit->SetResourcesHeld(value);
			//Wyrmgus end
		}

		//Wyrmgus start
//		unit = MakeUnitAndPlace(tmirrorh, type, &Players[PlayerNumNeutral]);
		unit = MakeUnitAndPlace(tmirrorh, type, &Players[PlayerNumNeutral], CurrentMapLayer);
		//Wyrmgus end
		if (unit == NULL) {
			DebugPrint("Unable to allocate Unit");
		} else {
			//Wyrmgus start
//			unit->ResourcesHeld = value;
			unit->SetResourcesHeld(value);
			//Wyrmgus end
		}

		//Wyrmgus start
//		unit = MakeUnitAndPlace(tmirrorv, type, &Players[PlayerNumNeutral]);
		unit = MakeUnitAndPlace(tmirrorv, type, &Players[PlayerNumNeutral], CurrentMapLayer);
		//Wyrmgus end
		if (unit == NULL) {
			DebugPrint("Unable to allocate Unit");
		} else {
			//Wyrmgus start
//			unit->ResourcesHeld = value;
			unit->SetResourcesHeld(value);
			//Wyrmgus end
		}

		//Wyrmgus start
//		unit = MakeUnitAndPlace(tmirror, type, &Players[PlayerNumNeutral]);
		unit = MakeUnitAndPlace(tmirror, type, &Players[PlayerNumNeutral], CurrentMapLayer);
		//Wyrmgus end
		if (unit == NULL) {
			DebugPrint("Unable to allocate Unit");
		} else {
			//Wyrmgus start
//			unit->ResourcesHeld = value;
			unit->SetResourcesHeld(value);
			//Wyrmgus end
		}
	}
}

/**
**  Destroy all units
*/
static void EditorDestroyAllUnits()
{
	while (UnitManager.empty() == false) {
		CUnit &unit = **UnitManager.begin();

		unit.Remove(NULL);
		UnitLost(unit);
		UnitClearOrders(unit);
		unit.Release();
	}
}

/**
**  Create a random map
*/
void CEditor::CreateRandomMap() const
{
	//Wyrmgus start
	const int mz = std::max(Map.Info.LayersSizes[CurrentMapLayer].y, Map.Info.LayersSizes[CurrentMapLayer].x);
	//Wyrmgus end

	// make water-base
	const Vec2i zeros(0, 0);
	TileFill(zeros, WATER_TILE, mz * 3);
	// remove all units
	EditorDestroyAllUnits();

	EditorRandomizeTile(COAST_TILE, 10, 16);
	EditorRandomizeTile(GRASS_TILE, 20, 16);
	EditorRandomizeTile(WOOD_TILE,  60,  4);
	EditorRandomizeTile(ROCK_TILE,  30,  2);

	EditorRandomizeUnit("unit-gold-mine", 5, 50000);
}

//@}
