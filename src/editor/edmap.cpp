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
//      (c) Copyright 2002-2021 by Lutz Sammer and Andrettin
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
#include "editor.h"
#include "map/map.h"
#include "map/map_layer.h"
#include "map/minimap.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "map/tileset.h"
#include "ui/ui.h"
#include "player.h"
#include "unit/unit.h"
#include "unit/unit_manager.h"
#include "unit/unit_type.h"
#include "util/random.h"
#include "util/util.h"

/// Callback for changed tile (with direction mask)
static void EditorChangeSurrounding(const Vec2i &pos, int d);

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

	wyrmgus::tile &mf = *Map.Field(pos);
	mf.setGraphicTile(tile);
	mf.player_info->SeenTile = tile;
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
void EditorChangeTile(const Vec2i &pos, int tileIndex)
{
	Assert(CMap::Map.Info.IsPointOnMap(pos, UI.CurrentMapLayer));

	// Change the flags
	wyrmgus::tile &mf = *UI.CurrentMapLayer->Field(pos);

	int tile = tileIndex;
	if (TileToolRandom) {
		int n = 0;
		for (int i = 0; i < 16; ++i) {
			if (!CMap::Map.Tileset->tiles[tile + i].tile) {
				break;
			} else {
				++n;
			}
		}
		n = random::get()->generate_async(n);
		int i = -1;
		do {
			while (++i < 16 && !CMap::Map.Tileset->tiles[tile + i].tile) {
			}
		} while (i < 16 && n--);
		Assert(i != 16);
		tile += i;
	}
	//Wyrmgus start
	mf.setTileIndex(*CMap::Map.Tileset, tile, 0);
//	mf.player_info->SeenTile = mf.getGraphicTile();
	mf.UpdateSeenTile();
	//Wyrmgus end
	
	//Wyrmgus start
	CMap::Map.CalculateTileTransitions(pos, false, UI.CurrentMapLayer->ID);
	CMap::Map.CalculateTileTransitions(pos, true, UI.CurrentMapLayer->ID);
	
	for (int x_offset = -1; x_offset <= 1; ++x_offset) {
		for (int y_offset = -1; y_offset <= 1; ++y_offset) {
			if (x_offset != 0 || y_offset != 0) {
				Vec2i adjacent_pos(pos.x + x_offset, pos.y + y_offset);
				if (CMap::Map.Info.IsPointOnMap(adjacent_pos, UI.CurrentMapLayer)) {
					CMap::Map.CalculateTileTransitions(adjacent_pos, false, UI.CurrentMapLayer->ID);
					CMap::Map.CalculateTileTransitions(adjacent_pos, true, UI.CurrentMapLayer->ID);
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

		if (!CanBuildUnitType(unit, *unit->Type, pos, 1, true, UI.CurrentMapLayer->ID)) {
			units_to_remove.push_back(unit);
		}
	}
	
	for (size_t i = 0; i < units_to_remove.size(); ++i) {
		EditorActionRemoveUnit(*units_to_remove[i], false);
	}
	//Wyrmgus end

	UI.get_minimap()->UpdateSeenXY(pos);
	UI.get_minimap()->UpdateXY(pos, UI.CurrentMapLayer->ID);

	EditorChangeSurrounding(pos, tile);
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
	wyrmgus::tile &mf = *UI.CurrentMapLayer->Field(pos);
	
	//Wyrmgus start
	//see if the tile's terrain can be here as is, or if it is needed to change surrounding tiles
	const wyrmgus::terrain_type *terrain = CMap::Map.GetTileTopTerrain(pos, false, UI.CurrentMapLayer->ID);
	const bool overlay = mf.get_overlay_terrain() != nullptr ? true : false;
	if (!terrain->allows_single()) {
		std::vector<int> transition_directions;
		
		for (int x_offset = -1; x_offset <= 1; ++x_offset) {
			for (int y_offset = -1; y_offset <= 1; ++y_offset) {
				if (x_offset != 0 || y_offset != 0) {
					Vec2i adjacent_pos(pos.x + x_offset, pos.y + y_offset);
					if (CMap::Map.Info.IsPointOnMap(adjacent_pos, UI.CurrentMapLayer)) {
						const wyrmgus::terrain_type *adjacent_terrain = CMap::Map.GetTileTerrain(adjacent_pos, overlay, UI.CurrentMapLayer->ID);
						if (overlay && adjacent_terrain && UI.CurrentMapLayer->Field(adjacent_pos)->OverlayTerrainDestroyed) {
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
			EditorChangeTile(pos + Vec2i(0, -1), tile);
			EditorChangeTile(pos + Vec2i(0, 1), tile);
		} else if (std::find(transition_directions.begin(), transition_directions.end(), West) != transition_directions.end() && std::find(transition_directions.begin(), transition_directions.end(), East) != transition_directions.end()) {
			EditorChangeTile(pos + Vec2i(-1, 0), tile);
			EditorChangeTile(pos + Vec2i(1, 0), tile);
		}
		
	}
	//Wyrmgus end

	//Wyrmgus start
	/*
	if (mf.isAWall()) {
		Map.SetWall(pos, mf.isHuman());
		return;
	}
	*/
	//Wyrmgus end

	//Wyrmgus start
	/*
	const unsigned int quad = QuadFromTile(pos);
	const unsigned int TH_QUAD_M = 0xFFFF0000; // Top half quad mask
	const unsigned int BH_QUAD_M = 0x0000FFFF; // Bottom half quad mask
	const unsigned int LH_QUAD_M = 0xFF00FF00; // Left half quad mask
	const unsigned int RH_QUAD_M = 0x00FF00FF; // Right half quad mask
	const unsigned int DIR_UP =    8; // Go up allowed
	const unsigned int DIR_DOWN =  4; // Go down allowed
	const unsigned int DIR_LEFT =  2; // Go left allowed
	const unsigned int DIR_RIGHT = 1; // Go right allowed

	// How this works:
	//  first get the quad of the neighbouring tile,
	//  then check if the margin matches.
	//  Otherwise, call EditorChangeTile again.
	if ((d & DIR_UP) && pos.y) {
		const Vec2i offset(0, -1);
		// Insert into the bottom the new tile.
		unsigned q2 = QuadFromTile(pos + offset);
		unsigned u = (q2 & TH_QUAD_M) | ((quad >> 16) & BH_QUAD_M);
		if (u != q2) {
			int tile = Map.Tileset->tileFromQuad(u & BH_QUAD_M, u);
			EditorChangeTile(pos + offset, tile, d & ~DIR_DOWN);
		}
	}
	if ((d & DIR_DOWN) && pos.y < Map.Info.MapHeight - 1) {
		const Vec2i offset(0, 1);
		// Insert into the top the new tile.
		unsigned q2 = QuadFromTile(pos + offset);
		unsigned u = (q2 & BH_QUAD_M) | ((quad << 16) & TH_QUAD_M);
		if (u != q2) {
			int tile = Map.Tileset->tileFromQuad(u & TH_QUAD_M, u);
			EditorChangeTile(pos + offset, tile, d & ~DIR_UP);
		}
	}
	if ((d & DIR_LEFT) && pos.x) {
		const Vec2i offset(-1, 0);
		// Insert into the left the new tile.
		unsigned q2 = QuadFromTile(pos + offset);
		unsigned u = (q2 & LH_QUAD_M) | ((quad >> 8) & RH_QUAD_M);
		if (u != q2) {
			int tile = Map.Tileset->tileFromQuad(u & RH_QUAD_M, u);
			EditorChangeTile(pos + offset, tile, d & ~DIR_RIGHT);
		}
	}
	if ((d & DIR_RIGHT) && pos.x < Map.Info.MapWidth - 1) {
		const Vec2i offset(1, 0);
		// Insert into the right the new tile.
		unsigned q2 = QuadFromTile(pos + offset);
		unsigned u = (q2 & RH_QUAD_M) | ((quad << 8) & LH_QUAD_M);
		if (u != q2) {
			int tile = Map.Tileset->tileFromQuad(u & LH_QUAD_M, u);
			EditorChangeTile(pos + offset, tile, d & ~DIR_LEFT);
		}
	}
	*/
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

	CMap::Map.FixSelectionArea(ipos, apos, UI.CurrentMapLayer->ID);

	Vec2i itPos;
	for (itPos.x = ipos.x; itPos.x <= apos.x; ++itPos.x) {
		for (itPos.y = ipos.y; itPos.y <= apos.y; ++itPos.y) {
			EditorChangeTile(itPos, tile);
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
	const Vec2i mpos(UI.CurrentMapLayer->get_width() - 1, UI.CurrentMapLayer->get_height() - 1);

	for (int i = 0; i < count; ++i) {
		const Vec2i rpos(random::get()->generate_async((1 + mpos.x) / 2), random::get()->generate_async((1 + mpos.y) / 2));
		const Vec2i mirror = mpos - rpos;
		const Vec2i mirrorh(rpos.x, mirror.y);
		const Vec2i mirrorv(mirror.x, rpos.y);
		const int rz = random::get()->generate_async(max_size) + 1;

		TileFill(rpos, tile, rz);
		TileFill(mirrorh, tile, rz);
		TileFill(mirrorv, tile, rz);
		TileFill(mirror, tile, rz);
	}
}

static constexpr int WATER_TILE = 0x10;
static constexpr int COAST_TILE = 0x30;
static constexpr int GRASS_TILE = 0x50;
static constexpr int WOOD_TILE = 0x70;
static constexpr int ROCK_TILE = 0x80;

/**
**  Add a unit to random locations on the map, unit will be neutral
**
**  @param unit_type  unit type to add to map as a character string
**  @param count      the number of times to add the unit
**  @param value      resources to be stored in that unit
*/
static void EditorRandomizeUnit(const char *unit_type, int count, int value)
{
	const Vec2i mpos(UI.CurrentMapLayer->get_width(), UI.CurrentMapLayer->get_height());
	wyrmgus::unit_type *typeptr = wyrmgus::unit_type::get(unit_type);

	wyrmgus::unit_type &type = *typeptr;
	const Vec2i tpos(type.get_tile_size());

	for (int i = 0; i < count; ++i) {
		const Vec2i rpos(random::get()->generate_async(mpos.x / 2 - tpos.x + 1), random::get()->generate_async(mpos.y / 2 - tpos.y + 1));
		const Vec2i mirror(mpos.x - rpos.x - 1, mpos.y - rpos.y - 1);
		const Vec2i mirrorh(rpos.x, mirror.y);
		const Vec2i mirrorv(mirror.x, rpos.y);
		const Vec2i tmirror(mpos.x - rpos.x - tpos.x, mpos.y - rpos.y - tpos.y);
		const Vec2i tmirrorh(rpos.x, tmirror.y);
		const Vec2i tmirrorv(tmirror.x, rpos.y);
		int tile = GRASS_TILE;
		const int z = type.get_tile_height();

		// FIXME: vladi: the idea is simple: make proper land for unit(s) :)
		// FIXME: handle units larger than 1 square
		TileFill(rpos, tile, z * 2);
		TileFill(mirrorh, tile, z * 2);
		TileFill(mirrorv, tile, z * 2);
		TileFill(mirror, tile, z * 2);

		// FIXME: can overlap units
		CUnit *unit = MakeUnitAndPlace(rpos, type, CPlayer::Players[PlayerNumNeutral], UI.CurrentMapLayer->ID);
		if (unit == nullptr) {
			DebugPrint("Unable to allocate Unit");
		} else {
			unit->SetResourcesHeld(value);
		}

		unit = MakeUnitAndPlace(tmirrorh, type, CPlayer::Players[PlayerNumNeutral], UI.CurrentMapLayer->ID);

		if (unit == nullptr) {
			DebugPrint("Unable to allocate Unit");
		} else {
			//Wyrmgus start
//			unit->ResourcesHeld = value;
			unit->SetResourcesHeld(value);
			//Wyrmgus end
		}

		unit = MakeUnitAndPlace(tmirrorv, type, CPlayer::Players[PlayerNumNeutral], UI.CurrentMapLayer->ID);

		if (unit == nullptr) {
			DebugPrint("Unable to allocate Unit");
		} else {
			unit->SetResourcesHeld(value);
		}

		unit = MakeUnitAndPlace(tmirror, type, CPlayer::Players[PlayerNumNeutral], UI.CurrentMapLayer->ID);

		if (unit == nullptr) {
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
	while (wyrmgus::unit_manager::get()->empty() == false) {
		CUnit &unit = **wyrmgus::unit_manager::get()->get_units().begin();

		unit.Remove(nullptr);
		UnitLost(unit);
		unit.clear_orders();
		unit.Release();
	}
}

/**
**  Create a random map
*/
void CEditor::CreateRandomMap() const
{
	const int mz = std::max(UI.CurrentMapLayer->get_height(), UI.CurrentMapLayer->get_width());

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
