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
/**@name map.cpp - The map. */
//
//      (c) Copyright 1998-2006 by Lutz Sammer, Vladi Shabanski and
//                                 Francois Beerten
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
//Wyrmgus start
#include <fstream> //for the 0 AD map conversion
//Wyrmgus end
#include "player.h"
#include "tileset.h"
#include "unit.h"
//Wyrmgus start
#include "unit_find.h"
//Wyrmgus end
#include "unit_manager.h"
#include "ui.h"
//Wyrmgus start
#include "upgrade.h"
//Wyrmgus end
#include "version.h"
#include "video.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

CMap Map;                   /// The current map
int FlagRevealMap;          /// Flag must reveal the map
int ReplayRevealMap;        /// Reveal Map is replay
int ForestRegeneration;     /// Forest regeneration
char CurrentMapPath[1024];  /// Path of the current map

/*----------------------------------------------------------------------------
--  Visible and explored handling
----------------------------------------------------------------------------*/

/**
**  Marks seen tile -- used mainly for the Fog Of War
**
**  @param mf  MapField-position.
*/
void CMap::MarkSeenTile(CMapField &mf)
{
	const unsigned int tile = mf.getGraphicTile();
	const unsigned int seentile = mf.playerInfo.SeenTile;

	//  Nothing changed? Seeing already the correct tile.
	if (tile == seentile) {
		return;
	}
	mf.playerInfo.SeenTile = tile;

#ifdef MINIMAP_UPDATE
	//rb - GRRRRRRRRRRRR
	const unsigned int index = &mf - Map.Fields;
	const int y = index / Info.MapWidth;
	const int x = index - (y * Info.MapWidth);
	const Vec2i pos = {x, y}
#endif

	if (this->Tileset->TileTypeTable.empty() == false) {
#ifndef MINIMAP_UPDATE
		//rb - GRRRRRRRRRRRR
		const unsigned int index = &mf - Map.Fields;
		const int y = index / Info.MapWidth;
		const int x = index - (y * Info.MapWidth);
		const Vec2i pos(x, y);
#endif

		//  Handle wood changes. FIXME: check if for growing wood correct?
		//Wyrmgus start
//		if (tile == this->Tileset->getRemovedTreeTile()) {
		if (std::find(this->Tileset->removedTreeTiles.begin(), this->Tileset->removedTreeTiles.end(), tile) != this->Tileset->removedTreeTiles.end() && mf.getFlag() & MapFieldStumps) {
		//Wyrmgus end
			FixNeighbors(MapFieldForest, 1, pos);
		//Wyrmgus start
//		} else if (seentile == this->Tileset->getRemovedTreeTile()) {
		} else if (std::find(this->Tileset->removedTreeTiles.begin(), this->Tileset->removedTreeTiles.end(), seentile) != this->Tileset->removedTreeTiles.end() && ((mf.getFlag() & MapFieldStumps) || (mf.getFlag() & MapFieldForest))) {
		//Wyrmgus end
			FixTile(MapFieldForest, 1, pos);
		} else if (mf.ForestOnMap()) {
			FixTile(MapFieldForest, 1, pos);
			FixNeighbors(MapFieldForest, 1, pos);

			// Handle rock changes.
		//Wyrmgus start
//		} else if (tile == Tileset->getRemovedRockTile()) {
		} else if (std::find(this->Tileset->removedRockTiles.begin(), this->Tileset->removedRockTiles.end(), tile) != this->Tileset->removedRockTiles.end() && mf.getFlag() & MapFieldGravel) {
		//Wyrmgus end
			FixNeighbors(MapFieldRocks, 1, pos);
		//Wyrmgus start
//		} else if (seentile == Tileset->getRemovedRockTile()) {
		} else if (std::find(this->Tileset->removedRockTiles.begin(), this->Tileset->removedRockTiles.end(), seentile) != this->Tileset->removedRockTiles.end() && ((mf.getFlag() & MapFieldGravel) || (mf.getFlag() & MapFieldRocks))) {
		//Wyrmgus end
			FixTile(MapFieldRocks, 1, pos);
		} else if (mf.RockOnMap()) {
			FixTile(MapFieldRocks, 1, pos);
			FixNeighbors(MapFieldRocks, 1, pos);

			//  Handle Walls changes.
		} else if (this->Tileset->isAWallTile(tile)
				   || this->Tileset->isAWallTile(seentile)) {
			MapFixSeenWallTile(pos);
			MapFixSeenWallNeighbors(pos);
		}
	}

#ifdef MINIMAP_UPDATE
	UI.Minimap.UpdateXY(pos);
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
	//  Global seen recount. Simple and effective.
	for (CUnitManager::Iterator it = UnitManager.begin(); it != UnitManager.end(); ++it) {
		CUnit &unit = **it;
		//  Reveal neutral buildings. Gold mines:)
		if (unit.Player->Type == PlayerNeutral) {
			for (int p = 0; p < PlayerMax; ++p) {
				//Wyrmgus start
//				if (Players[p].Type != PlayerNobody && (!(unit.Seen.ByPlayer & (1 << p)))) {
				if (Players[p].Type != PlayerNobody && (Players[p].Type == PlayerPerson || !only_person_players) && (!(unit.Seen.ByPlayer & (1 << p)))) {
				//Wyrmgus end
					UnitGoesOutOfFog(unit, Players[p]);
					UnitGoesUnderFog(unit, Players[p]);
				}
			}
		}
		UnitCountSeen(unit);
	}
}

/*----------------------------------------------------------------------------
--  Map queries
----------------------------------------------------------------------------*/

Vec2i CMap::MapPixelPosToTilePos(const PixelPos &mapPos) const
{
	const Vec2i tilePos(mapPos.x / PixelTileSize.x, mapPos.y / PixelTileSize.y);

	return tilePos;
}

PixelPos CMap::TilePosToMapPixelPos_TopLeft(const Vec2i &tilePos) const
{
	PixelPos mapPixelPos(tilePos.x * PixelTileSize.x, tilePos.y * PixelTileSize.y);

	return mapPixelPos;
}

PixelPos CMap::TilePosToMapPixelPos_Center(const Vec2i &tilePos) const
{
	return TilePosToMapPixelPos_TopLeft(tilePos) + PixelTileSize / 2;
}

/**
**  Wall on map tile.
**
**  @param pos  map tile position.
**
**  @return    True if wall, false otherwise.
*/
bool CMap::WallOnMap(const Vec2i &pos) const
{
	Assert(Map.Info.IsPointOnMap(pos));
	return Field(pos)->isAWall();
}

//Wyrmgus start
CTerrainType *CMap::GetTileTerrain(const Vec2i &pos, bool overlay) const
{
	if (!Map.Info.IsPointOnMap(pos)) {
		return NULL;
	}
	
	CMapField &mf = *this->Field(pos);
	
	if (overlay) {
		return mf.OverlayTerrain;
	} else {
		return mf.Terrain;
	}
}
//Wyrmgus end

/**
**  Human wall on map tile.
**
**  @param pos  map tile position.
**
**  @return    True if human wall, false otherwise.
*/
bool CMap::HumanWallOnMap(const Vec2i &pos) const
{
	Assert(Map.Info.IsPointOnMap(pos));
	return Field(pos)->isAHumanWall();
}

/**
**  Orc wall on map tile.
**
**  @param pos  map tile position.
**
**  @return    True if orcish wall, false otherwise.
*/
bool CMap::OrcWallOnMap(const Vec2i &pos) const
{
	Assert(Map.Info.IsPointOnMap(pos));
	return Field(pos)->isAOrcWall();
}

/**
**  Can move to this point, applying mask.
**
**  @param pos   map tile position.
**  @param mask  Mask for movement to apply.
**
**  @return      True if could be entered, false otherwise.
*/
bool CheckedCanMoveToMask(const Vec2i &pos, int mask)
{
	return Map.Info.IsPointOnMap(pos) && CanMoveToMask(pos, mask);
}

/**
**  Can a unit of unit-type be placed at this point.
**
**  @param type  unit-type to be checked.
**  @param pos   map tile position.
**
**  @return      True if could be entered, false otherwise.
*/
bool UnitTypeCanBeAt(const CUnitType &type, const Vec2i &pos)
{
	const int mask = type.MovementMask;
	unsigned int index = pos.y * Map.Info.MapWidth;

	for (int addy = 0; addy < type.TileHeight; ++addy) {
		for (int addx = 0; addx < type.TileWidth; ++addx) {
			if (Map.Info.IsPointOnMap(pos.x + addx, pos.y + addy) == false
				|| Map.Field(pos.x + addx + index)->CheckMask(mask) == true) {
				return false;
			}
		}
		index += Map.Info.MapWidth;
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
bool UnitCanBeAt(const CUnit &unit, const Vec2i &pos)
{
	Assert(unit.Type);
	if (unit.Type->BoolFlag[NONSOLID_INDEX].value) {
		return true;
	}
	return UnitTypeCanBeAt(*unit.Type, pos);
}

/**
**  Fixes initially the wood and seen tiles.
*/
void PreprocessMap()
{
	for (int ix = 0; ix < Map.Info.MapWidth; ++ix) {
		for (int iy = 0; iy < Map.Info.MapHeight; ++iy) {
			CMapField &mf = *Map.Field(ix, iy);
			//Wyrmgus start
			Map.CalculateTileTransitions(Vec2i(ix, iy));
			Map.CalculateTileTransitions(Vec2i(ix, iy), true);
			//Wyrmgus end
			mf.playerInfo.SeenTile = mf.getGraphicTile();
		}
	}
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
}

/**
**  Clear CMapInfo.
*/
void CMapInfo::Clear()
{
	this->Description.clear();
	this->Filename.clear();
	this->MapWidth = this->MapHeight = 0;
	memset(this->PlayerSide, 0, sizeof(this->PlayerSide));
	memset(this->PlayerType, 0, sizeof(this->PlayerType));
	this->MapUID = 0;
}

CMap::CMap() : Fields(NULL), NoFogOfWar(false), TileGraphic(NULL)
{
	Tileset = new CTileset;
}

CMap::~CMap()
{
	delete Tileset;
}

/**
**  Alocate and initialise map table
*/
void CMap::Create()
{
	Assert(!this->Fields);

	this->Fields = new CMapField[this->Info.MapWidth * this->Info.MapHeight];
}

/**
**  Initialize the fog of war.
**  Build tables, setup functions.
*/
void CMap::Init()
{
	InitFogOfWar();
}

/**
**  Cleanup the map module.
*/
void CMap::Clean()
{
	delete[] this->Fields;

	// Tileset freed by Tileset?

	this->Info.Clear();
	this->Fields = NULL;
	this->NoFogOfWar = false;
	//Wyrmgus start
	for (size_t i = 0; i != Map.Tileset->solidTerrainTypes.size(); ++i) {
		if (!Map.Tileset->solidTerrainTypes[i].ImageFile.empty()) {
			if (CGraphic::Get(Map.Tileset->solidTerrainTypes[i].ImageFile) != NULL) {
				CGraphic::Free(this->SolidTileGraphics[i]);
			}
			this->SolidTileGraphics[i] = NULL;
		}
	}
	memset(SolidTileGraphics, 0, sizeof(SolidTileGraphics));
	//Wyrmgus end
	this->Tileset->clear();
	this->TileModelsFileName.clear();
	CGraphic::Free(this->TileGraphic);
	this->TileGraphic = NULL;

	FlagRevealMap = 0;
	ReplayRevealMap = 0;

	UI.Minimap.Destroy();
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
	file.printf("  \"map-fields\", {\n");
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
void CMap::FixTile(unsigned short type, int seen, const Vec2i &pos)
{
	Assert(type == MapFieldForest || type == MapFieldRocks);

	//  Outside of map or no wood.
	if (!Info.IsPointOnMap(pos)) {
		return;
	}
	unsigned int index = getIndex(pos);
	CMapField &mf = *this->Field(index);

	//Wyrmgus start
	/*
	if (!((type == MapFieldForest && Tileset->isAWoodTile(mf.playerInfo.SeenTile))
		  || (type == MapFieldRocks && Tileset->isARockTile(mf.playerInfo.SeenTile)))) {
		if (seen) {
			return;
		}
	}
	*/
	if (!((type == MapFieldForest && Tileset->isAWoodTile(mf.playerInfo.SeenTile) && (mf.getFlag() & type))
		  || (type == MapFieldRocks && Tileset->isARockTile(mf.playerInfo.SeenTile) && (mf.getFlag() & type)))) {
		if (seen) {
			return;
		}
	}
	//Wyrmgus end
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
		//Wyrmgus start
		if (this->Tileset->tiles[mf.getTileIndex()].tileinfo.BaseTerrain != this->Tileset->tiles[new_mf.getTileIndex()].tileinfo.BaseTerrain) {
			ttup = -2;
		}
		//Wyrmgus end
	}
	if (pos.x + 1 >= this->Info.MapWidth) {
		ttright = -1; //Assign trees in all directions
	} else {
		const CMapField &new_mf = *(&mf + 1);
		ttright = seen ? new_mf.playerInfo.SeenTile : new_mf.getGraphicTile();
		//Wyrmgus start
		if (this->Tileset->tiles[mf.getTileIndex()].tileinfo.BaseTerrain != this->Tileset->tiles[new_mf.getTileIndex()].tileinfo.BaseTerrain) {
			ttright = -2;
		}
		//Wyrmgus end
	}
	if (pos.y + 1 >= this->Info.MapHeight) {
		ttdown = -1; //Assign trees in all directions
	} else {
		const CMapField &new_mf = *(&mf + this->Info.MapWidth);
		ttdown = seen ? new_mf.playerInfo.SeenTile : new_mf.getGraphicTile();
		//Wyrmgus start
		if (this->Tileset->tiles[mf.getTileIndex()].tileinfo.BaseTerrain != this->Tileset->tiles[new_mf.getTileIndex()].tileinfo.BaseTerrain) {
			ttdown = -2;
		}
		//Wyrmgus end
	}
	if (pos.x - 1 < 0) {
		ttleft = -1; //Assign trees in all directions
	} else {
		const CMapField &new_mf = *(&mf - 1);
		ttleft = seen ? new_mf.playerInfo.SeenTile : new_mf.getGraphicTile();
		//Wyrmgus start
		if (this->Tileset->tiles[mf.getTileIndex()].tileinfo.BaseTerrain != this->Tileset->tiles[new_mf.getTileIndex()].tileinfo.BaseTerrain) {
			ttleft = -2;
		}
		//Wyrmgus end
	}
	//Wyrmgus start
//	int tile = this->Tileset->getTileBySurrounding(type, ttup, ttright, ttdown, ttleft);
	int tile = this->Tileset->getTileBySurrounding(type, mf.getTileIndex(), ttup, ttright, ttdown, ttleft);
	//Wyrmgus end

	//Update seen tile.
	if (tile == -1) { // No valid wood remove it.
		if (seen) {
			mf.playerInfo.SeenTile = removedtile;
			this->FixNeighbors(type, seen, pos);
		} else {
			mf.setGraphicTile(removedtile);
			//Wyrmgus start
			mf.SetOverlayTerrainDestroyed(true);
			//Wyrmgus end
			mf.Flags &= ~flags;
			//Wyrmgus start
			if (type == MapFieldForest) {
				mf.Flags |= MapFieldStumps;
			} else if (type == MapFieldRocks) {
				mf.Flags |= MapFieldGravel;
				mf.Flags |= MapFieldNoBuilding;
			}
			//Wyrmgus end
			mf.Value = 0;
			UI.Minimap.UpdateXY(pos);
			
			//Wyrmgus start
			this->CalculateTileTransitions(pos, true);
			
			for (int x_offset = -1; x_offset <= 1; ++x_offset) {
				for (int y_offset = -1; y_offset <= 1; ++y_offset) {
					if (x_offset != 0 || y_offset != 0) {
						Vec2i adjacent_pos(pos.x + x_offset, pos.y + y_offset);
						if (Map.Info.IsPointOnMap(adjacent_pos)) {
							this->CalculateTileTransitions(adjacent_pos, true);
						}
					}
				}
			}
			//Wyrmgus end
		}
	//Wyrmgus start
//	} else if (seen && this->Tileset->isEquivalentTile(tile, mf.playerInfo.SeenTile)) { //Same Type
	} else if (seen && this->Tileset->isEquivalentTile(tile, mf.playerInfo.SeenTile, mf.getTileIndex())) { //Same Type
	//Wyrmgus end
		return;
	} else {
		if (seen) {
			mf.playerInfo.SeenTile = tile;
		} else {
			mf.setGraphicTile(tile);
		}
	}

	//maybe isExplored
	//Wyrmgus start
//	if (mf.playerInfo.IsExplored(*ThisPlayer)) {
	if (mf.playerInfo.IsTeamExplored(*ThisPlayer)) {
	//Wyrmgus end
		UI.Minimap.UpdateSeenXY(pos);
		if (!seen) {
			MarkSeenTile(mf);
		}
	}
}

/**
**  Correct the surrounding fields.
**
**  @param type  Tiletype of tile to adjust
**  @param seen  1 if updating seen value, 0 for real
**  @param pos   Map tile-position.
*/
void CMap::FixNeighbors(unsigned short type, int seen, const Vec2i &pos)
{
	const Vec2i offset[] = {Vec2i(1, 0), Vec2i(-1, 0), Vec2i(0, 1), Vec2i(0, -1),
							Vec2i(-1, -1), Vec2i(-1, 1), Vec2i(1, -1), Vec2i(1, 1)
						   };

	for (unsigned int i = 0; i < sizeof(offset) / sizeof(*offset); ++i) {
		FixTile(type, seen, pos + offset[i]);
	}
}

//Wyrmgus start
void CMap::SetTileTerrain(const Vec2i &pos, CTerrainType *terrain)
{
	if (!terrain) {
		return;
	}
	
	CMapField &mf = *this->Field(pos);
	
	mf.SetTerrain(terrain);
	
	this->CalculateTileTransitions(pos, terrain->Overlay);
	
	for (int x_offset = -1; x_offset <= 1; ++x_offset) {
		for (int y_offset = -1; y_offset <= 1; ++y_offset) {
			if (x_offset != 0 || y_offset != 0) {
				Vec2i adjacent_pos(pos.x + x_offset, pos.y + y_offset);
				if (Map.Info.IsPointOnMap(adjacent_pos)) {
					this->CalculateTileTransitions(adjacent_pos, terrain->Overlay);
				}
			}
		}
	}
}

void CMap::CalculateTileTransitions(const Vec2i &pos, bool overlay)
{
	CMapField &mf = *this->Field(pos);
	CTerrainType *terrain = NULL;
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
	std::vector<int> transition_directions;
	
	for (int x_offset = -1; x_offset <= 1; ++x_offset) {
		for (int y_offset = -1; y_offset <= 1; ++y_offset) {
			if (x_offset != 0 || y_offset != 0) {
				Vec2i adjacent_pos(pos.x + x_offset, pos.y + y_offset);
				if (Map.Info.IsPointOnMap(adjacent_pos)) {
					CTerrainType *adjacent_terrain = this->GetTileTerrain(adjacent_pos, overlay);
					if (overlay && adjacent_terrain && this->Field(adjacent_pos)->OverlayTerrainDestroyed) {
						adjacent_terrain = NULL;
					}
					if (adjacent_terrain && terrain != adjacent_terrain && std::find(terrain->InnerBorderTerrains.begin(), terrain->InnerBorderTerrains.end(), adjacent_terrain) != terrain->InnerBorderTerrains.end()) {
						adjacent_terrain_directions[adjacent_terrain->ID].push_back(GetDirectionFromOffset(x_offset, y_offset));
					}
					if (terrain != adjacent_terrain) { // also happens if terrain is NULL, so that i.e. tree transitions display correctly when adjacent to tiles without overlays
						adjacent_terrain_directions[TerrainTypes.size()].push_back(GetDirectionFromOffset(x_offset, y_offset));
					}
				}
			}
		}
	}
	
	for (std::map<int, std::vector<int>>::iterator iterator = adjacent_terrain_directions.begin(); iterator != adjacent_terrain_directions.end(); ++iterator) {
		int adjacent_terrain_id = iterator->first;
		CTerrainType *adjacent_terrain = adjacent_terrain_id < TerrainTypes.size() ? TerrainTypes[adjacent_terrain_id] : NULL;
		int transition_type = -1;
		
		if (terrain->AllowSingle && std::find(iterator->second.begin(), iterator->second.end(), North) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), South) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), West) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), East) != iterator->second.end()) {
			transition_type = SingleTransitionType;
		} else if (terrain->AllowSingle && std::find(iterator->second.begin(), iterator->second.end(), North) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), South) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), West) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), East) != iterator->second.end()) {
			transition_type = NorthSingleTransitionType;
		} else if (terrain->AllowSingle && std::find(iterator->second.begin(), iterator->second.end(), North) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), South) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), West) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), East) != iterator->second.end()) {
			transition_type = SouthSingleTransitionType;
		} else if (terrain->AllowSingle && std::find(iterator->second.begin(), iterator->second.end(), North) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), South) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), West) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), East) == iterator->second.end()) {
			transition_type = WestSingleTransitionType;
		} else if (terrain->AllowSingle && std::find(iterator->second.begin(), iterator->second.end(), North) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), South) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), West) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), East) != iterator->second.end()) {
			transition_type = EastSingleTransitionType;
		} else if (terrain->AllowSingle && std::find(iterator->second.begin(), iterator->second.end(), North) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), South) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), West) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), East) == iterator->second.end()) {
			transition_type = NorthSouthTransitionType;
		} else if (terrain->AllowSingle && std::find(iterator->second.begin(), iterator->second.end(), West) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), East) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), North) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), South) == iterator->second.end()) {
			transition_type = WestEastTransitionType;
		} else if (terrain->AllowSingle && std::find(iterator->second.begin(), iterator->second.end(), West) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), East) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), North) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), South) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), Southeast) != iterator->second.end()) {
			transition_type = WestSoutheastInnerTransitionType;
		} else if (terrain->AllowSingle && std::find(iterator->second.begin(), iterator->second.end(), West) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), East) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), North) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), South) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), Southwest) != iterator->second.end()) {
			transition_type = EastSouthwestInnerTransitionType;
		} else if (terrain->AllowSingle && std::find(iterator->second.begin(), iterator->second.end(), West) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), East) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), North) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), South) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), Northeast) != iterator->second.end()) {
			transition_type = SouthwestOuterNortheastInnerTransitionType;
		} else if (terrain->AllowSingle && std::find(iterator->second.begin(), iterator->second.end(), West) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), East) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), North) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), South) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), Northwest) != iterator->second.end()) {
			transition_type = SoutheastOuterNorthwestInnerTransitionType;
		} else if (std::find(iterator->second.begin(), iterator->second.end(), North) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), South) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), Southwest) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), Southeast) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), West) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), East) == iterator->second.end()) {
			transition_type = NorthTransitionType;
		} else if (std::find(iterator->second.begin(), iterator->second.end(), South) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), North) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), Northwest) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), Northeast) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), West) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), East) == iterator->second.end()) {
			transition_type = SouthTransitionType;
		} else if (std::find(iterator->second.begin(), iterator->second.end(), West) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), East) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), Northeast) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), Southeast) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), North) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), South) == iterator->second.end()) {
			transition_type = WestTransitionType;
		} else if (std::find(iterator->second.begin(), iterator->second.end(), East) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), West) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), Northwest) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), Southwest) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), North) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), South) == iterator->second.end()) {
			transition_type = EastTransitionType;
		} else if ((std::find(iterator->second.begin(), iterator->second.end(), North) != iterator->second.end() || std::find(iterator->second.begin(), iterator->second.end(), West) != iterator->second.end()) && std::find(iterator->second.begin(), iterator->second.end(), South) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), East) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), Southeast) == iterator->second.end()) {
			transition_type = NorthwestOuterTransitionType;
		} else if ((std::find(iterator->second.begin(), iterator->second.end(), North) != iterator->second.end() || std::find(iterator->second.begin(), iterator->second.end(), East) != iterator->second.end()) && std::find(iterator->second.begin(), iterator->second.end(), South) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), West) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), Southwest) == iterator->second.end()) {
			transition_type = NortheastOuterTransitionType;
		} else if ((std::find(iterator->second.begin(), iterator->second.end(), South) != iterator->second.end() || std::find(iterator->second.begin(), iterator->second.end(), West) != iterator->second.end()) && std::find(iterator->second.begin(), iterator->second.end(), North) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), East) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), Northeast) == iterator->second.end()) {
			transition_type = SouthwestOuterTransitionType;
		} else if ((std::find(iterator->second.begin(), iterator->second.end(), South) != iterator->second.end() || std::find(iterator->second.begin(), iterator->second.end(), East) != iterator->second.end()) && std::find(iterator->second.begin(), iterator->second.end(), North) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), West) == iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), Northwest) == iterator->second.end()) {
			transition_type = SoutheastOuterTransitionType;
		} else if (std::find(iterator->second.begin(), iterator->second.end(), Northwest) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), Southeast) != iterator->second.end()) {
			transition_type = NorthwestSoutheastInnerTransitionType;
		} else if (std::find(iterator->second.begin(), iterator->second.end(), Northeast) != iterator->second.end() && std::find(iterator->second.begin(), iterator->second.end(), Southwest) != iterator->second.end()) {
			transition_type = NortheastSouthwestInnerTransitionType;
		} else if (std::find(iterator->second.begin(), iterator->second.end(), Northwest) != iterator->second.end()) {
			transition_type = NorthwestInnerTransitionType;
		} else if (std::find(iterator->second.begin(), iterator->second.end(), Northeast) != iterator->second.end()) {
			transition_type = NortheastInnerTransitionType;
		} else if (std::find(iterator->second.begin(), iterator->second.end(), Southwest) != iterator->second.end()) {
			transition_type = SouthwestInnerTransitionType;
		} else if (std::find(iterator->second.begin(), iterator->second.end(), Southeast) != iterator->second.end()) {
			transition_type = SoutheastInnerTransitionType;
		}
		
		if (transition_type != -1) {
			bool found_transition = false;
			
			if (!overlay) {
				if (adjacent_terrain) {
					if (terrain->TransitionTiles[std::tuple<int, int>(adjacent_terrain_id, transition_type)].size() > 0) {
						mf.TransitionTiles.push_back(std::pair<CTerrainType *, int>(terrain, terrain->TransitionTiles[std::tuple<int, int>(adjacent_terrain_id, transition_type)][SyncRand(terrain->TransitionTiles[std::tuple<int, int>(adjacent_terrain_id, transition_type)].size())]));
						found_transition = true;
					} else if (adjacent_terrain->AdjacentTransitionTiles[std::tuple<int, int>(terrain_id, transition_type)].size() > 0) {
						mf.TransitionTiles.push_back(std::pair<CTerrainType *, int>(adjacent_terrain, adjacent_terrain->AdjacentTransitionTiles[std::tuple<int, int>(terrain_id, transition_type)][SyncRand(adjacent_terrain->AdjacentTransitionTiles[std::tuple<int, int>(terrain_id, transition_type)].size())]));
						found_transition = true;
					} else if (adjacent_terrain->AdjacentTransitionTiles[std::tuple<int, int>(-1, transition_type)].size() > 0) {
						mf.TransitionTiles.push_back(std::pair<CTerrainType *, int>(adjacent_terrain, adjacent_terrain->AdjacentTransitionTiles[std::tuple<int, int>(-1, transition_type)][SyncRand(adjacent_terrain->AdjacentTransitionTiles[std::tuple<int, int>(-1, transition_type)].size())]));
						found_transition = true;
					}
					
					if ((mf.Flags & MapFieldWaterAllowed) && !(adjacent_terrain->Flags & MapFieldWaterAllowed)) { //if this is a water tile adjacent to a non-water tile, replace the water flag with a coast one
						mf.Flags &= ~(MapFieldWaterAllowed);
						mf.Flags |= MapFieldCoastAllowed;
					}
				} else {
					if (terrain->TransitionTiles[std::tuple<int, int>(-1, transition_type)].size() > 0) {
						mf.TransitionTiles.push_back(std::pair<CTerrainType *, int>(terrain, terrain->TransitionTiles[std::tuple<int, int>(-1, transition_type)][SyncRand(terrain->TransitionTiles[std::tuple<int, int>(-1, transition_type)].size())]));
					}
				}
			} else {
				if (adjacent_terrain) {
					if (adjacent_terrain && terrain->TransitionTiles[std::tuple<int, int>(adjacent_terrain_id, transition_type)].size() > 0) {
						mf.OverlayTransitionTiles.push_back(std::pair<CTerrainType *, int>(terrain, terrain->TransitionTiles[std::tuple<int, int>(adjacent_terrain_id, transition_type)][SyncRand(terrain->TransitionTiles[std::tuple<int, int>(adjacent_terrain_id, transition_type)].size())]));
						found_transition = true;
					} else if (adjacent_terrain && adjacent_terrain->AdjacentTransitionTiles[std::tuple<int, int>(terrain_id, transition_type)].size() > 0) {
						mf.OverlayTransitionTiles.push_back(std::pair<CTerrainType *, int>(adjacent_terrain, adjacent_terrain->AdjacentTransitionTiles[std::tuple<int, int>(terrain_id, transition_type)][SyncRand(adjacent_terrain->AdjacentTransitionTiles[std::tuple<int, int>(terrain_id, transition_type)].size())]));
						found_transition = true;
					} else if (adjacent_terrain && adjacent_terrain->AdjacentTransitionTiles[std::tuple<int, int>(-1, transition_type)].size() > 0) {
						mf.OverlayTransitionTiles.push_back(std::pair<CTerrainType *, int>(adjacent_terrain, adjacent_terrain->AdjacentTransitionTiles[std::tuple<int, int>(-1, transition_type)][SyncRand(adjacent_terrain->AdjacentTransitionTiles[std::tuple<int, int>(-1, transition_type)].size())]));
						found_transition = true;
					}
				} else {
					if (terrain->TransitionTiles[std::tuple<int, int>(-1, transition_type)].size() > 0) {
						mf.OverlayTransitionTiles.push_back(std::pair<CTerrainType *, int>(terrain, terrain->TransitionTiles[std::tuple<int, int>(-1, transition_type)][SyncRand(terrain->TransitionTiles[std::tuple<int, int>(-1, transition_type)].size())]));
					}
				}
			}
			
			if (adjacent_terrain && found_transition) {
				for (size_t i = 0; i != iterator->second.size(); ++i) {
					adjacent_terrain_directions[TerrainTypes.size()].erase(std::remove(adjacent_terrain_directions[TerrainTypes.size()].begin(), adjacent_terrain_directions[TerrainTypes.size()].end(), iterator->second[i]), adjacent_terrain_directions[TerrainTypes.size()].end());
				}
			}
		}
	}
}
//Wyrmgus end

/// Remove wood from the map.
void CMap::ClearWoodTile(const Vec2i &pos)
{
	CMapField &mf = *this->Field(pos);

	//Wyrmgus start
	mf.SetOverlayTerrainDestroyed(true);
	//Wyrmgus end
	mf.setGraphicTile(this->Tileset->getRemovedTreeTile());
	mf.Flags &= ~(MapFieldForest | MapFieldUnpassable);
	//Wyrmgus start
	mf.Flags |= MapFieldStumps;
	//Wyrmgus end
	mf.Value = 0;

	UI.Minimap.UpdateXY(pos);
	FixNeighbors(MapFieldForest, 0, pos);
	//Wyrmgus start
	this->CalculateTileTransitions(pos, true);
	
	for (int x_offset = -1; x_offset <= 1; ++x_offset) {
		for (int y_offset = -1; y_offset <= 1; ++y_offset) {
			if (x_offset != 0 || y_offset != 0) {
				Vec2i adjacent_pos(pos.x + x_offset, pos.y + y_offset);
				if (Map.Info.IsPointOnMap(adjacent_pos)) {
					this->CalculateTileTransitions(adjacent_pos, true);
				}
			}
		}
	}
	//Wyrmgus end

	//maybe isExplored
	//Wyrmgus start
//	if (mf.playerInfo.IsExplored(*ThisPlayer)) {
	if (mf.playerInfo.IsTeamExplored(*ThisPlayer)) {
	//Wyrmgus end
		UI.Minimap.UpdateSeenXY(pos);
		MarkSeenTile(mf);
	}
	
	//Wyrmgus start
	//remove decorations if a wall, tree or rock was removed from the tile
	std::vector<CUnit *> table;
	Select(pos, pos, table);
	for (size_t i = 0; i != table.size(); ++i) {
		if (table[i]->Type->UnitType == UnitTypeLand && table[i]->Type->BoolFlag[DECORATION_INDEX].value) {
			LetUnitDie(*table[i]);			
		}
	}
	//Wyrmgus end
}
/// Remove rock from the map.
void CMap::ClearRockTile(const Vec2i &pos)
{
	CMapField &mf = *this->Field(pos);

	mf.setGraphicTile(this->Tileset->getRemovedRockTile());
	//Wyrmgus start
	mf.SetOverlayTerrainDestroyed(true);
	//Wyrmgus end
	mf.Flags &= ~(MapFieldRocks | MapFieldUnpassable);
	//Wyrmgus start
	mf.Flags |= MapFieldGravel;
	mf.Flags |= MapFieldNoBuilding;
	//Wyrmgus end
	mf.Value = 0;
	
	UI.Minimap.UpdateXY(pos);
	FixNeighbors(MapFieldRocks, 0, pos);
	//Wyrmgus start
	this->CalculateTileTransitions(pos, true);
	
	for (int x_offset = -1; x_offset <= 1; ++x_offset) {
		for (int y_offset = -1; y_offset <= 1; ++y_offset) {
			if (x_offset != 0 || y_offset != 0) {
				Vec2i adjacent_pos(pos.x + x_offset, pos.y + y_offset);
				if (Map.Info.IsPointOnMap(adjacent_pos)) {
					this->CalculateTileTransitions(adjacent_pos, true);
				}
			}
		}
	}
	//Wyrmgus end

	//maybe isExplored
	//Wyrmgus start
//	if (mf.playerInfo.IsExplored(*ThisPlayer)) {
	if (mf.playerInfo.IsTeamExplored(*ThisPlayer)) {
	//Wyrmgus end
		UI.Minimap.UpdateSeenXY(pos);
		MarkSeenTile(mf);
	}
	
	//Wyrmgus start
	//remove decorations if a wall, tree or rock was removed from the tile
	std::vector<CUnit *> table;
	Select(pos, pos, table);
	for (size_t i = 0; i != table.size(); ++i) {
		if (table[i]->Type->UnitType == UnitTypeLand && table[i]->Type->BoolFlag[DECORATION_INDEX].value) {
			LetUnitDie(*table[i]);			
		}
	}
	//Wyrmgus end
}



/**
**  Regenerate forest.
**
**  @param pos  Map tile pos
*/
void CMap::RegenerateForestTile(const Vec2i &pos)
{
	Assert(Map.Info.IsPointOnMap(pos));
	CMapField &mf = *this->Field(pos);

	//Wyrmgus start
//	if (mf.getGraphicTile() != this->Tileset->getRemovedTreeTile()) {
	if (std::find(this->Tileset->removedTreeTiles.begin(), this->Tileset->removedTreeTiles.end(), mf.getGraphicTile()) == this->Tileset->removedTreeTiles.end() || !(mf.getFlag() & MapFieldStumps)) {
	//Wyrmgus end
		return;
	}

	//  Increment each value of no wood.
	//  If grown up, place new wood.
	//  FIXME: a better looking result would be fine
	//    Allow general updates to any tiletype that regrows

	//Wyrmgus start
	/*
	const unsigned int occupedFlag = (MapFieldWall | MapFieldUnpassable | MapFieldLandUnit | MapFieldBuilding);
	++mf.Value;
	if (mf.Value < ForestRegeneration) {
		return;
	}
	mf.Value = ForestRegeneration;
	if ((mf.Flags & occupedFlag) || pos.y == 0) {
		return;
	}
	*/
	
	const unsigned int permanent_occupied_flag = (MapFieldWall | MapFieldUnpassable | MapFieldBuilding);
	const unsigned int occupedFlag = (MapFieldWall | MapFieldUnpassable | MapFieldLandUnit | MapFieldBuilding | MapFieldItem); // don't regrow forests under items
	
	if ((mf.Flags & permanent_occupied_flag)) { //if the tree tile is occupied by buildings and the like, reset the regeneration process
		mf.Value = 0;
		return;
	}

	if ((mf.Flags & occupedFlag) || pos.y == 0) {
		return;
	}
	
	++mf.Value;
	if (mf.Value < ForestRegeneration) {
		return;
	}
	mf.Value = ForestRegeneration;
	//Wyrmgus end
	
	//Wyrmgus start
//	const Vec2i offset(0, -1);
//	CMapField &topMf = *(&mf - this->Info.MapWidth);

	for (int x_offset = -1; x_offset <= 1; x_offset+=2) { //increment by 2 to avoid instances where it is 0
		for (int y_offset = -1; y_offset <= 1; y_offset+=2) {
			const Vec2i verticalOffset(0, y_offset);
			CMapField &verticalMf = *this->Field(pos + verticalOffset);
			const Vec2i horizontalOffset(x_offset, 0);
			CMapField &horizontalMf = *this->Field(pos + horizontalOffset);
			const Vec2i diagonalOffset(x_offset, y_offset);
			CMapField &diagonalMf = *this->Field(pos + diagonalOffset);
			
			if (
				this->Info.IsPointOnMap(pos + verticalOffset)
				&& ((std::find(this->Tileset->removedTreeTiles.begin(), this->Tileset->removedTreeTiles.end(), verticalMf.getGraphicTile()) != this->Tileset->removedTreeTiles.end() && (verticalMf.getFlag() & MapFieldStumps) && verticalMf.Value >= ForestRegeneration && !(verticalMf.Flags & occupedFlag)) || (verticalMf.getFlag() & MapFieldForest))
				&& this->Info.IsPointOnMap(pos + diagonalOffset)
				&& ((std::find(this->Tileset->removedTreeTiles.begin(), this->Tileset->removedTreeTiles.end(), diagonalMf.getGraphicTile()) != this->Tileset->removedTreeTiles.end() && (diagonalMf.getFlag() & MapFieldStumps) && diagonalMf.Value >= ForestRegeneration && !(diagonalMf.Flags & occupedFlag)) || (diagonalMf.getFlag() & MapFieldForest))
				&& this->Info.IsPointOnMap(pos + horizontalOffset)
				&& ((std::find(this->Tileset->removedTreeTiles.begin(), this->Tileset->removedTreeTiles.end(), horizontalMf.getGraphicTile()) != this->Tileset->removedTreeTiles.end() && (horizontalMf.getFlag() & MapFieldStumps) && horizontalMf.Value >= ForestRegeneration && !(horizontalMf.Flags & occupedFlag)) || (horizontalMf.getFlag() & MapFieldForest))
			) {
				DebugPrint("Real place wood\n");
				verticalMf.setTileIndex(*Map.Tileset, Map.Tileset->getDefaultWoodTileIndex(), DefaultResourceAmounts[WoodCost]);
				verticalMf.setGraphicTile(Map.Tileset->tiles[Map.Tileset->getDefaultWoodTileIndex()].tile);
				verticalMf.playerInfo.SeenTile = verticalMf.getGraphicTile();
				verticalMf.Value = DefaultResourceAmounts[WoodCost];
				UI.Minimap.UpdateSeenXY(pos + verticalOffset);
				UI.Minimap.UpdateXY(pos + verticalOffset);
				
				diagonalMf.setTileIndex(*Map.Tileset, Map.Tileset->getDefaultWoodTileIndex(), DefaultResourceAmounts[WoodCost]);
				diagonalMf.setGraphicTile(Map.Tileset->tiles[Map.Tileset->getDefaultWoodTileIndex()].tile);
				diagonalMf.playerInfo.SeenTile = diagonalMf.getGraphicTile();
				diagonalMf.Value = DefaultResourceAmounts[WoodCost];
				UI.Minimap.UpdateSeenXY(pos + diagonalOffset);
				UI.Minimap.UpdateXY(pos + diagonalOffset);
				
				horizontalMf.setTileIndex(*Map.Tileset, Map.Tileset->getDefaultWoodTileIndex(), DefaultResourceAmounts[WoodCost]);
				horizontalMf.setGraphicTile(Map.Tileset->tiles[Map.Tileset->getDefaultWoodTileIndex()].tile);
				horizontalMf.playerInfo.SeenTile = horizontalMf.getGraphicTile();
				horizontalMf.Value = DefaultResourceAmounts[WoodCost];
				UI.Minimap.UpdateSeenXY(pos + horizontalOffset);
				UI.Minimap.UpdateXY(pos + horizontalOffset);
				
				mf.setTileIndex(*Map.Tileset, Map.Tileset->getDefaultWoodTileIndex(), DefaultResourceAmounts[WoodCost]);
				mf.setGraphicTile(Map.Tileset->tiles[Map.Tileset->getDefaultWoodTileIndex()].tile);
				mf.playerInfo.SeenTile = mf.getGraphicTile();
				mf.Value = DefaultResourceAmounts[WoodCost];
				UI.Minimap.UpdateSeenXY(pos);
				UI.Minimap.UpdateXY(pos);
				
				if (mf.playerInfo.IsTeamVisible(*ThisPlayer)) {
					MarkSeenTile(mf);
				}
				if (Map.Field(pos + verticalOffset)->playerInfo.IsTeamVisible(*ThisPlayer)) {
					MarkSeenTile(verticalMf);
				}
				if (Map.Field(pos + diagonalOffset)->playerInfo.IsTeamVisible(*ThisPlayer)) {
					MarkSeenTile(diagonalMf);
				}
				if (Map.Field(pos + horizontalOffset)->playerInfo.IsTeamVisible(*ThisPlayer)) {
					MarkSeenTile(horizontalMf);
				}

				FixNeighbors(MapFieldForest, 0, pos + diagonalOffset);
				FixNeighbors(MapFieldForest, 0, pos + horizontalOffset);
				FixNeighbors(MapFieldForest, 0, pos + verticalOffset);
				FixNeighbors(MapFieldForest, 0, pos);
				
				return;
			}
		}
	}

	/*
	if (topMf.getGraphicTile() == this->Tileset->getRemovedTreeTile()
		&& topMf.Value >= ForestRegeneration
		&& !(topMf.Flags & occupedFlag)) {
		DebugPrint("Real place wood\n");
		topMf.setTileIndex(*Map.Tileset, Map.Tileset->getTopOneTreeTile(), 0);
		topMf.setGraphicTile(Map.Tileset->getTopOneTreeTile());
		topMf.playerInfo.SeenTile = topMf.getGraphicTile();
		topMf.Value = 0;
		topMf.Flags |= MapFieldForest | MapFieldUnpassable;
		UI.Minimap.UpdateSeenXY(pos + offset);
		UI.Minimap.UpdateXY(pos + offset);
		
		mf.setTileIndex(*Map.Tileset, Map.Tileset->getBottomOneTreeTile(), 0);
		mf.setGraphicTile(Map.Tileset->getBottomOneTreeTile());
		mf.playerInfo.SeenTile = mf.getGraphicTile();
		mf.Value = 0;
		mf.Flags |= MapFieldForest | MapFieldUnpassable;
		UI.Minimap.UpdateSeenXY(pos);
		UI.Minimap.UpdateXY(pos);
		
		if (mf.playerInfo.IsTeamVisible(*ThisPlayer)) {
			MarkSeenTile(mf);
		}
		if (Map.Field(pos + offset)->playerInfo.IsTeamVisible(*ThisPlayer)) {
			MarkSeenTile(topMf);
		}
		FixNeighbors(MapFieldForest, 0, pos + offset);
		FixNeighbors(MapFieldForest, 0, pos);
	}
	*/
}

/**
**  Regenerate forest.
*/
void CMap::RegenerateForest()
{
	if (!ForestRegeneration) {
		return;
	}
	Vec2i pos;
	for (pos.y = 0; pos.y < Info.MapHeight; ++pos.y) {
		for (pos.x = 0; pos.x < Info.MapWidth; ++pos.x) {
			RegenerateForestTile(pos);
		}
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
		Map.Info.Filename = mapname;
		Map.Info.Filename.replace(loc, 4, ".sms");
	}

	const std::string filename = LibraryFileName(mapname.c_str());
	LuaLoadFile(filename);
}

//Wyrmgus start
std::string RawTiles[MaxMapWidth][MaxMapHeight];

/**
**  Convert 0 AD map
**
**  @param mapname  map filename
*/
void Convert0ADMap(const std::string &mapname)
{
	const std::string xml_filename = LibraryFileName(mapname.c_str());
	std::string pmp_filename = FindAndReplaceStringEnding(xml_filename, ".xml", ".pmp");
	
	for (int x = 0; x < MaxMapWidth; ++x) {
		for (int y = 0; y < MaxMapHeight; ++y) {
			RawTiles[x][y].clear();
		}
	}
	int map_size = 0;
	int tile_heights_xy[MaxMapWidth][MaxMapHeight];

	if (!CanAccessFile(pmp_filename.c_str())) {
		fprintf(stderr, "File \"%s\" not found.\n", pmp_filename.c_str());
	}
	
	std::ifstream is_pmp(pmp_filename, std::ifstream::binary);
	if (is_pmp) {
		is_pmp.seekg (0, is_pmp.end);
		int length = is_pmp.tellg();
		is_pmp.seekg (0, is_pmp.beg);

		char * buffer = new char [length];

		is_pmp.read(buffer,length);

		if (!is_pmp) {
			fprintf(stderr, "Error loading file \"%s\".\n", pmp_filename.c_str());
		}
		
		int current_byte = 0;
		current_byte += 4; //char magic[4]; // == "PSMP"
		current_byte += 4; //u32 version; // == 6
		current_byte += 4; //u32 data_size; // == filesize-12
		
		map_size = (map_size << 8) + buffer[current_byte + 3];
		map_size = (map_size << 8) + buffer[current_byte + 2];
		map_size = (map_size << 8) + buffer[current_byte + 1];
		map_size = (map_size << 8) + buffer[current_byte + 0];
		current_byte += 4; //u32 map_size; // number of patches (16x16 tiles) per side
		
		int tile_heights[(MaxMapWidth + 1) * (MaxMapHeight + 1)];
		for (int i = 0; i < ((map_size * 16 + 1) * (map_size * 16 + 1)); ++i) { //u16 heightmap[(mapsize*16 + 1)*(mapsize*16 + 1)]; // vertex heights
			int height = 0;
			height = (height << 8) + buffer[current_byte + 1];
			height = (height << 8) + buffer[current_byte + 0];
			current_byte += 2;
			
			tile_heights[i] = height;
		}
		
		int num_terrain_textures = 0;
		num_terrain_textures = (num_terrain_textures << 8) + buffer[current_byte + 3];
		num_terrain_textures = (num_terrain_textures << 8) + buffer[current_byte + 2];
		num_terrain_textures = (num_terrain_textures << 8) + buffer[current_byte + 1];
		num_terrain_textures = (num_terrain_textures << 8) + buffer[current_byte + 0];
		current_byte += 4; //u32 num_terrain_textures;
		
		std::string terrain_texture_names[1024];
		for (int i = 0; i < num_terrain_textures; ++i) { //String terrain_textures[num_terrain_textures]; // filenames (no path), e.g. "cliff1.dds"
			int string_length = 0;
			string_length = (string_length << 8) + buffer[current_byte + 3];
			string_length = (string_length << 8) + buffer[current_byte + 2];
			string_length = (string_length << 8) + buffer[current_byte + 1];
			string_length = (string_length << 8) + buffer[current_byte + 0];
			current_byte += 4; //u32 length;
			
			char string_data[256];
			for (int j = 0; j < string_length; ++j) {
				string_data[j] = buffer[current_byte + j];
			}
			string_data[string_length] = '\0';
			current_byte += string_length; //char data[length]; // not NUL-terminated
			
			terrain_texture_names[i] = std::string(string_data);
		}
		
		int patch_quantity = map_size * map_size;
		for (int i = 0; i < patch_quantity; ++i) { //Patch patches[mapsize^2];
			int tile_quantity = 16 * 16;
			for (int j = 0; j < tile_quantity; ++j) { //Tile tiles[16*16];
				int texture1 = 0;
				texture1 = (texture1 << 8) + buffer[current_byte + 1];
				texture1 = (texture1 << 8) + buffer[current_byte + 0];
				current_byte += 2; //u16 texture1; // index into terrain_textures[]
					
				int texture2 = 0;
				texture2 = (texture2 << 8) + buffer[current_byte + 1];
				texture2 = (texture2 << 8) + buffer[current_byte + 0];
				current_byte += 2; //u16 texture2; // index, or 0xFFFF for 'none'
					
				current_byte += 4; //u32 priority; // Used for blending between edges of tiles with different textures. A higher priority is blended on top of a lower priority.
					
				int patch_x = i % map_size;
				int x = (patch_x * 16) + (j % 16);
					
				int patch_y = i / map_size;
				int y = (map_size * 16) - 1 - ((patch_y * 16) + (j / 16));
				
				int height = abs(tile_heights[(((map_size * 16) - 1 - y) * (map_size * 16 + 1)) + x + 1]);
				tile_heights_xy[x][y] = height;
				
				std::string tile_type;
					
				if (texture1 >= 0) {
					std::string texture1_string = terrain_texture_names[texture1];
					tile_type = Convert0ADTextureToTileType(texture1_string);
				}
						
				if (texture2 >= 0) {
					std::string texture2_string = terrain_texture_names[texture2];
				}

				RawTiles[x][y] = tile_type;
			}
		}
		
		is_pmp.close();

		delete[] buffer;
	}
	
	float water_height;
	std::string map_title;
	std::vector<std::string> units_unit_types;
	std::vector<int> units_players;
	std::vector<Vec2i> units_positions;

	int player_quantity = 0;
	Vec2i player_start_points[PlayerMax];
	std::string player_civilizations[PlayerMax];
	std::string player_factions[PlayerMax];
	
	if (!CanAccessFile(xml_filename.c_str())) {
		fprintf(stderr, "File \"%s\" not found.\n", xml_filename.c_str());
	}
	
	std::ifstream is_xml(xml_filename);
	
	std::string processing_state;
	std::string line_str;
	while (std::getline(is_xml, line_str))
	{
		std::string line_contents = FindAndReplaceString(line_str, "	", "");
		
		if (processing_state == "<WaterBody>") {
			if (line_contents.find("<Height>", 0) != std::string::npos) {
				line_contents = FindAndReplaceString(line_contents, "<Height>", "");
				line_contents = FindAndReplaceString(line_contents, "</Height>", "");
				water_height = std::stof(line_contents);
			} else if (line_contents == "</WaterBody>") {
				processing_state = "";
			}
		} else if (processing_state == "<ScriptSettings>") {
			if (line_contents.find("\"Name\": ", 0) != std::string::npos) {
				line_contents = FindAndReplaceStringBeginning(line_contents, "  \"Name\": \"", "");
				line_contents = FindAndReplaceStringEnding(line_contents, "\",", "");
				map_title = line_contents;
			} else if (line_contents.find("\"PlayerData\": [", 0) != std::string::npos) {
				processing_state = "PlayerData";
			} else if (line_contents.find("</ScriptSettings>", 0) != std::string::npos) {
				processing_state = "";
			}
		} else if (processing_state == "PlayerData") {
			if (line_contents.find("\"Civ\": ", 0) != std::string::npos) {
				line_contents = FindAndReplaceString(line_contents, " ", "");
				line_contents = FindAndReplaceString(line_contents, "\"Civ\":", "");
				line_contents = FindAndReplaceString(line_contents, "\"", "");
				line_contents = FindAndReplaceString(line_contents, ",", "");
				player_civilizations[player_quantity] = Convert0ADCivilizationToCivilization(line_contents);
				if (Convert0ADCivilizationToFaction(line_contents) != "") {
					player_factions[player_quantity] = Convert0ADCivilizationToFaction(line_contents);
				}
				player_quantity += 1;
			} else if (line_contents.find("],", 0) != std::string::npos) {
				processing_state = "<ScriptSettings>";
			}
		} else if (processing_state == "<Entity>") {
			if (line_contents.find("<Template>", 0) != std::string::npos) {
				line_contents = FindAndReplaceString(line_contents, "<Template>", "");
				line_contents = FindAndReplaceString(line_contents, "</Template>", "");
				units_unit_types.push_back(Convert0ADTemplateToUnitTypeIdent(line_contents));
				if (line_contents.find("actor|", 0) != std::string::npos) { //these don't have a player defined, so we need to push one for them
					units_players.push_back(15);
				}
			} else if (line_contents.find("<Player>", 0) != std::string::npos) {
				line_contents = FindAndReplaceString(line_contents, "<Player>", "");
				line_contents = FindAndReplaceString(line_contents, "</Player>", "");
				int unit_player = std::stoi(line_contents) - 1;
				if (unit_player == -1) { //gaia units use player 0 in 0 AD
					unit_player = 15;
				}
				units_players.push_back(unit_player);
			} else if (line_contents.find("<Position", 0) != std::string::npos) {
				std::string x_position_string = FindAndReplaceString(line_contents, "<Position x=\"", "");
				int x_position_string_end = x_position_string.find("\"", 0);
				x_position_string.replace(x_position_string_end, x_position_string.length() - x_position_string_end, "");
				float x_position_float = std::stof(x_position_string) / 4;
				int x_position = x_position_float;
				
				std::string y_position_string = FindAndReplaceStringEnding(line_contents, "\"/>", "");
				int y_position_string_beginning = y_position_string.find("z=\"", 0);
				y_position_string.replace(0, y_position_string_beginning, "");
				y_position_string = FindAndReplaceString(y_position_string, "z=\"", "");
				float y_position_float = ((map_size * 16 * 4) - std::stof(y_position_string)) / 4;
				int y_position = y_position_float;
				
				units_positions.push_back(Vec2i(x_position, y_position));
				
				//if we've reached the position and the unit still has no player defined, define the neutral player for it
				if (units_players.size() < units_unit_types.size()) {
					units_players.push_back(15);
				}
				
				if (!units_unit_types[units_unit_types.size() - 1].empty() && units_unit_types[units_unit_types.size() - 1] != "Tree" && units_unit_types[units_unit_types.size() - 1] != "Rock" && units_unit_types[units_unit_types.size() - 1] != "Gold" && UnitTypes[UnitTypeIdByIdent(units_unit_types[units_unit_types.size() - 1])]->Class == "town-hall" && units_players[units_players.size() - 1] != 15) { //if this is a town hall building not belonging to the neutral player, set its owner's starting position to it
					int start_point_player = units_players[units_players.size() - 1];
					player_start_points[start_point_player].x = x_position;
					player_start_points[start_point_player].y = y_position;
				}
			} else if (line_contents == "</Entity>") {
				processing_state = "";
			}
		} else {
			if (line_contents == "<WaterBody>") {
				processing_state = "<WaterBody>";
			} else if (line_contents.find("<ScriptSettings>", 0) != std::string::npos) {
				processing_state = "<ScriptSettings>";
			} else if (line_contents.find("<Entity uid", 0) != std::string::npos) {
				processing_state = "<Entity>";
			}
		}
	}  
	
	water_height *= 65536.f;
	float height_scale = 1.f / 92;
	water_height *= height_scale;
		
	for (int x = 0; x < map_size * 16; ++x) {
		for (int y = 0; y < map_size * 16; ++y) {
			if (tile_heights_xy[x][y] < water_height) {
				RawTiles[x][y] = "Water";
			}
		}
	}
	
	for (size_t i = 0; i < units_unit_types.size(); ++i) {
		if (units_unit_types[i] == "Tree") {
			RawTiles[units_positions[i].x][units_positions[i].y] = "Tree";
			if ((units_positions[i].x + 1) < (map_size * 16) && (RawTiles[units_positions[i].x + 1][units_positions[i].y] == "Land" || RawTiles[units_positions[i].x + 1][units_positions[i].y] == "Rough")) {
				RawTiles[units_positions[i].x + 1][units_positions[i].y] = "Tree";
			}
			if ((units_positions[i].y + 1) < (map_size * 16) && (RawTiles[units_positions[i].x][units_positions[i].y + 1] == "Land" || RawTiles[units_positions[i].x][units_positions[i].y + 1] == "Rough")) {
				RawTiles[units_positions[i].x][units_positions[i].y + 1] = "Tree";
			}
			if ((units_positions[i].x + 1) < (map_size * 16) && (units_positions[i].y + 1) < (map_size * 16) && (RawTiles[units_positions[i].x + 1][units_positions[i].y + 1] == "Land" || RawTiles[units_positions[i].x + 1][units_positions[i].y + 1] == "Rough")) {
				RawTiles[units_positions[i].x + 1][units_positions[i].y + 1] = "Tree";
			}
		} else if (units_unit_types[i] == "Rock") {
			RawTiles[units_positions[i].x][units_positions[i].y] = "Rock";
			if ((units_positions[i].x + 1) < (map_size * 16) && (RawTiles[units_positions[i].x + 1][units_positions[i].y] == "Land" || RawTiles[units_positions[i].x + 1][units_positions[i].y] == "Rough")) {
				RawTiles[units_positions[i].x + 1][units_positions[i].y] = "Rock";
			}
			if ((units_positions[i].y + 1) < (map_size * 16) && (RawTiles[units_positions[i].x][units_positions[i].y + 1] == "Land" || RawTiles[units_positions[i].x][units_positions[i].y + 1] == "Rough")) {
				RawTiles[units_positions[i].x][units_positions[i].y + 1] = "Rock";
			}
			if ((units_positions[i].x + 1) < (map_size * 16) && (units_positions[i].y + 1) < (map_size * 16) && (RawTiles[units_positions[i].x + 1][units_positions[i].y + 1] == "Land" || RawTiles[units_positions[i].x + 1][units_positions[i].y + 1] == "Rough")) {
				RawTiles[units_positions[i].x + 1][units_positions[i].y + 1] = "Rock";
			}
		} else if (units_unit_types[i] == "Gold") {
			units_unit_types.push_back("unit-gold-deposit");
			units_positions.push_back(units_positions[i]);
			units_players.push_back(units_players[i]);
			for (int j = 0; j < 5; ++j) {
				units_unit_types.push_back("unit-gold-rock");
				units_positions.push_back(units_positions[i]);
				units_players.push_back(units_players[i]);
			}
		} else if (!units_unit_types[i].empty()) {
			int unit_type_id = UnitTypeIdByIdent(units_unit_types[i]);
			if (UnitTypes[unit_type_id]->BoolFlag[BUILDING_INDEX].value) { //if is a building, set the terrain below it to buildable land
				int unit_tile_width = UnitTypes[unit_type_id]->TileWidth;
				int unit_tile_height = UnitTypes[unit_type_id]->TileHeight;
				int unit_x_offset = - ((unit_tile_width - 1) / 2); //0 AD units' position is mapped to their center
				int unit_y_offset = - ((unit_tile_height - 1) / 2);
				int x = units_positions[i].x + unit_x_offset;
				int y = units_positions[i].y + unit_y_offset;
				for (int sub_x = 0; sub_x < unit_tile_width; ++sub_x) {
					for (int sub_y = 0; sub_y < unit_tile_height; ++sub_y) {
						if ((x + sub_x) >= (map_size * 16) || (y + sub_y) >= (map_size * 16)) {
							continue;
						}
						if (RawTiles[x + sub_x][y + sub_y] != "Land" && RawTiles[x + sub_x][y + sub_y] != "Dark-Land") {
							RawTiles[x + sub_x][y + sub_y] = "Land";
						}
					}
				}
			}
		}
	}
	
	AdjustRawTileMapIrregularities(map_size * 16, map_size * 16);
	AdjustRawTileMapTransitions(map_size * 16, map_size * 16);
	AdjustRawTileMapIrregularities(map_size * 16, map_size * 16);
		
	//write the map presentation
	std::string smp_filename = FindAndReplaceStringEnding(xml_filename, ".xml", ".smp");
	FileWriter *f_smp = NULL;

	try {
		f_smp = CreateFileWriter(smp_filename);
		f_smp->printf("-- Stratagus Map Presentation\n");
		f_smp->printf("-- Map converted from 0 AD's map format by the Stratagus built-in map converter.\n");
		f_smp->printf("-- File licensed under the GNU GPL version 2.\n\n");

		f_smp->printf("DefinePlayerTypes(");
		for (int i = 0; i < player_quantity; ++i) {
			f_smp->printf("%s\"%s\"", (i ? ", " : ""), "person");
		}
		f_smp->printf(")\n");

		f_smp->printf("PresentMap(\"%s\", %d, %d, %d, %d)\n",
				  map_title.c_str(), player_quantity, map_size * 16, map_size * 16, 1);
	} catch (const FileException &) {
		fprintf(stderr, "Couldn't write the map presentation: \"%s\"\n", smp_filename.c_str());
		delete f_smp;
		return;
	}

	delete f_smp;
	//end of map presentation writing

	//begin writing the map setup
	FileWriter *f = NULL;
	std::string sms_filename = FindAndReplaceStringEnding(xml_filename, ".xml", ".sms");

	try {
		f = CreateFileWriter(sms_filename);

		f->printf("-- Stratagus Map Setup\n");
		f->printf("-- Map converted from 0 AD's map format by the Stratagus built-in map converter.\n");
		f->printf("-- File licensed under the GNU GPL version 2.\n\n");

		f->printf("-- load tilesets\n");
		f->printf("LoadTileModels(\"%s\")\n\n", "scripts/tilesets/conifer_forest_summer.lua");
			
		for (int x = 0; x < map_size * 16; ++x) {
			for (int y = 0; y < map_size * 16; ++y) {
				f->printf("SetRawTile(%d, %d, \"%s\")\n", x, y, RawTiles[x][y].c_str());
			}
		}
			
		f->printf("ApplyRawTiles()\n");

		f->printf("-- player configuration\n");
		for (int i = 0; i < player_quantity; ++i) {
			f->printf("SetStartView(%d, %d, %d)\n", i, Players[i].StartPos.x, Players[i].StartPos.y);
			f->printf("SetPlayerData(%d, \"Resources\", \"%s\", %d)\n",
					  i, DefaultResourceNames[GoldCost].c_str(),
					  5000);
			f->printf("SetPlayerData(%d, \"Resources\", \"%s\", %d)\n",
					  i, DefaultResourceNames[WoodCost].c_str(),
					  2000);
			f->printf("SetPlayerData(%d, \"Resources\", \"%s\", %d)\n",
					  i, DefaultResourceNames[StoneCost].c_str(),
					  2000);
			f->printf("SetStartView(%d, %d, %d)\n", i, player_start_points[i].x, player_start_points[i].y);
			f->printf("SetPlayerData(%d, \"RaceName\", \"%s\")\n",
					  i, player_civilizations[i].c_str());
			if (!player_factions[i].empty()) {
				f->printf("SetPlayerData(%d, \"Faction\", \"%s\")\n",
						  i, player_factions[i].c_str());
			}
			f->printf("SetAiType(%d, \"%s\")\n",
					  i, "land-attack");
		}
					  
		for (size_t i = 0; i < units_unit_types.size(); ++i) {
			if (!units_unit_types[i].empty() && units_unit_types[i] != "Tree" && units_unit_types[i] != "Rock" && units_unit_types[i] != "Gold") {
				int unit_x_offset = - ((UnitTypes[UnitTypeIdByIdent(units_unit_types[i])]->TileWidth - 1) / 2); //0 AD units' position is mapped to their center
				int unit_y_offset = - ((UnitTypes[UnitTypeIdByIdent(units_unit_types[i])]->TileHeight - 1) / 2);
				f->printf("unit = CreateUnit(\"%s\", %d, {%d, %d})\n",
						  units_unit_types[i].c_str(),
						  units_players[i],
						  units_positions[i].x + unit_x_offset, units_positions[i].y + unit_y_offset);
			}
		}

		f->printf("\n");
	} catch (const FileException &) {
		fprintf(stderr, "Couldn't write the map setup: \"%s\"\n", sms_filename.c_str());
		delete f;
		return;
	}
		
	delete f; // end of the writing of the map setup
}

std::string Convert0ADTextureToTileType(const std::string zero_ad_texture)
{
	//Mediterranean biome
	if (zero_ad_texture == "medit_city_pavement") {
		return "Land";
	} else if (zero_ad_texture == "medit_cliff_aegean") {
		return "Rock";
	} else if (zero_ad_texture == "medit_cliff_aegean_shrubs") {
		return "Rock";
	} else if (zero_ad_texture == "medit_cliff_grass") {
		return "Rock";
	} else if (zero_ad_texture == "medit_cliff_greek") {
		return "Rock";
	} else if (zero_ad_texture == "medit_cliff_greek_2") {
		return "Rock";
	} else if (zero_ad_texture == "medit_cliff_italia_grass") {
		return "Rock";
	} else if (zero_ad_texture == "medit_dirt") {
		return "Rough";
	} else if (zero_ad_texture == "medit_dirt_b") {
		return "Rough";
	} else if (zero_ad_texture == "medit_dirt_c") {
		return "Rough";
	} else if (zero_ad_texture == "medit_dirt_d") {
		return "Rough";
	} else if (zero_ad_texture == "medit_dirt_e") {
		return "Rough";
	} else if (zero_ad_texture == "medit_farmland") {
		return "Land";
	} else if (zero_ad_texture == "medit_forestfloor_a") {
		return "Tree";
	} else if (zero_ad_texture == "medit_grass_field") {
		return "Land";
	} else if (zero_ad_texture == "medit_grass_field_a") {
		return "Land";
	} else if (zero_ad_texture == "medit_grass_field_b") {
		return "Land";
	} else if (zero_ad_texture == "medit_grass_field_brown") {
		return "Land";
	} else if (zero_ad_texture == "medit_grass_field_dry") {
		return "Land";
	} else if (zero_ad_texture == "medit_grass_flowers") {
		return "Land";
	} else if (zero_ad_texture == "medit_grass_shrubs") {
		return "Land";
	} else if (zero_ad_texture == "medit_grass_wild") {
		return "Land";
	} else if (zero_ad_texture == "medit_plants_dirt") {
		return "Rough";
	} else if (zero_ad_texture == "medit_riparian_mud") {
		return "Rough";
	} else if (zero_ad_texture == "medit_rocks") {
		return "Rough";
	} else if (zero_ad_texture == "medit_rocks_grass") {
		return "Rough";
	} else if (zero_ad_texture == "medit_rocks_grass_shrubs") {
		return "Rough";
	} else if (zero_ad_texture == "medit_rocks_shrubs") {
		return "Rough";
	} else if (zero_ad_texture == "medit_rocks_wet") {
		return "Rough";
	} else if (zero_ad_texture == "medit_sand_messy") {
		return "Rough";
	} else if (zero_ad_texture == "medit_sea_coral_deep") {
		return "Water";
	} else if (zero_ad_texture == "medit_sea_coral_plants") {
		return "Water";
	} else if (zero_ad_texture == "medit_sea_depths") {
		return "Water";
	} else if (zero_ad_texture == "medit_shrubs") {
		return "Land";
	} else if (zero_ad_texture == "medit_shrubs_dry") {
		return "Land";
	} else if (zero_ad_texture == "medit_shrubs_golden") {
		return "Land";
	//Temperate biome
	} else if (zero_ad_texture == "temp_dirt_a") {
		return "Rough";
	//City
	} else if (zero_ad_texture == "medit_city_tile") {
		return "Land";
	} else if (zero_ad_texture == "medit_road_broken") {
		return "Land";
	} else if (zero_ad_texture == "savanna_tile_a") {
		return "Land";
	} else if (zero_ad_texture == "savanna_tile_a_red") {
		return "Land";
	} else if (zero_ad_texture == "temp_road_muddy") {
		return "Land";
	} else if (zero_ad_texture == "tropic_citytile_a") {
		return "Land";
	//Grass
	} else if (zero_ad_texture == "grass1_spring") {
		return "Land";
	//Special
	} else if (zero_ad_texture == "farmland_a") {
		return "Land";
	} else {
		fprintf(stderr, "0 AD terrain texture \"%s\" not recognized.\n", zero_ad_texture.c_str());
		return "";
	}
}

std::map<std::string, std::string> ZeroADTemplateUnitTypeEquivalency;

void Set0ADTemplateUnitTypeEquivalency(const std::string zero_ad_template, const std::string unit_type_ident)
{
	ZeroADTemplateUnitTypeEquivalency[zero_ad_template] = unit_type_ident;
}

std::string Convert0ADTemplateToUnitTypeIdent(const std::string zero_ad_template)
{	
	if (ZeroADTemplateUnitTypeEquivalency.find(zero_ad_template) != ZeroADTemplateUnitTypeEquivalency.end()) {
		return ZeroADTemplateUnitTypeEquivalency[zero_ad_template];
	} else {
		fprintf(stderr, "0 AD template \"%s\" not recognized.\n", zero_ad_template.c_str());
		return "";
	}
}

std::map<std::string, std::string> ZeroADCivilizationEquivalency;

void Set0ADCivilizationEquivalency(const std::string zero_ad_civilization, const std::string civilization)
{
	ZeroADCivilizationEquivalency[zero_ad_civilization] = civilization;
}

std::string Convert0ADCivilizationToCivilization(const std::string zero_ad_civilization)
{
	if (ZeroADCivilizationEquivalency.find(zero_ad_civilization) != ZeroADCivilizationEquivalency.end()) {
		return ZeroADCivilizationEquivalency[zero_ad_civilization];
	} else {
		fprintf(stderr, "0 AD civilization \"%s\" not recognized.\n", zero_ad_civilization.c_str());
		return "";
	}
}

std::map<std::string, std::string> ZeroADCivilizationFactionEquivalency;

void Set0ADCivilizationFactionEquivalency(const std::string zero_ad_civilization, const std::string faction)
{
	ZeroADCivilizationFactionEquivalency[zero_ad_civilization] = faction;
}

std::string Convert0ADCivilizationToFaction(const std::string zero_ad_civilization)
{
	if (ZeroADCivilizationFactionEquivalency.find(zero_ad_civilization) != ZeroADCivilizationFactionEquivalency.end()) {
		return ZeroADCivilizationFactionEquivalency[zero_ad_civilization];
	} else {
		fprintf(stderr, "0 AD civilization \"%s\" not recognized.\n", zero_ad_civilization.c_str());
		return "";
	}
}

void AdjustRawTileMapIrregularities(int map_width, int map_height)
{
	bool no_irregularities_found = false;
	while (!no_irregularities_found) {
		no_irregularities_found = true;
		for (int x = 0; x < map_width; ++x) {
			for (int y = 0; y < map_height; ++y) {
				if (RawTiles[x][y] == "Land" || RawTiles[x][y] == "") {
					continue;
				}
				std::vector<std::string> acceptable_adjacent_tile_types;
				acceptable_adjacent_tile_types.push_back(RawTiles[x][y]);
				std::string base_tile;
				
				if (RawTiles[x][y] == "Rough") {
					acceptable_adjacent_tile_types.push_back("Rock");
					acceptable_adjacent_tile_types.push_back("Water");
					base_tile = "Land";
				} else if (RawTiles[x][y] == "Rock") {
					base_tile = "Rough";
				} else if (RawTiles[x][y] == "Water") {
					base_tile = "Rough";
				} else if (RawTiles[x][y] == "Tree") {
					base_tile = "Land";
				}
				
				int horizontal_adjacent_tiles = 0;
				int vertical_adjacent_tiles = 0;
				int nw_quadrant_adjacent_tiles = 0; //should be 4 if the wrong tile types are present in X-1,Y; X-1,Y-1; X,Y-1; and X+1,Y+1
				int ne_quadrant_adjacent_tiles = 0;
				int sw_quadrant_adjacent_tiles = 0;
				int se_quadrant_adjacent_tiles = 0;
				
				if ((x - 1) >= 0 && std::find(acceptable_adjacent_tile_types.begin(), acceptable_adjacent_tile_types.end(), RawTiles[x - 1][y]) == acceptable_adjacent_tile_types.end()) {
					horizontal_adjacent_tiles += 1;
					nw_quadrant_adjacent_tiles += 1;
					sw_quadrant_adjacent_tiles += 1;
				}
				if ((x + 1) < map_width && std::find(acceptable_adjacent_tile_types.begin(), acceptable_adjacent_tile_types.end(), RawTiles[x + 1][y]) == acceptable_adjacent_tile_types.end()) {
					horizontal_adjacent_tiles += 1;
					ne_quadrant_adjacent_tiles += 1;
					se_quadrant_adjacent_tiles += 1;
				}
				
				if ((y - 1) >= 0 && std::find(acceptable_adjacent_tile_types.begin(), acceptable_adjacent_tile_types.end(), RawTiles[x][y - 1]) == acceptable_adjacent_tile_types.end()) {
					vertical_adjacent_tiles += 1;
					nw_quadrant_adjacent_tiles += 1;
					ne_quadrant_adjacent_tiles += 1;
				}
				if ((y + 1) < map_height && std::find(acceptable_adjacent_tile_types.begin(), acceptable_adjacent_tile_types.end(), RawTiles[x][y + 1]) == acceptable_adjacent_tile_types.end()) {
					vertical_adjacent_tiles += 1;
					sw_quadrant_adjacent_tiles += 1;
					se_quadrant_adjacent_tiles += 1;
				}

				if ((x - 1) >= 0 && (y - 1) >= 0 && std::find(acceptable_adjacent_tile_types.begin(), acceptable_adjacent_tile_types.end(), RawTiles[x - 1][y - 1]) == acceptable_adjacent_tile_types.end()) {
					nw_quadrant_adjacent_tiles += 1;
					se_quadrant_adjacent_tiles += 1;
				}
				if ((x - 1) >= 0 && (y + 1) < map_height && std::find(acceptable_adjacent_tile_types.begin(), acceptable_adjacent_tile_types.end(), RawTiles[x - 1][y + 1]) == acceptable_adjacent_tile_types.end()) {
					sw_quadrant_adjacent_tiles += 1;
					ne_quadrant_adjacent_tiles += 1;
				}
				if ((x + 1) < map_width && (y - 1) >= 0 && std::find(acceptable_adjacent_tile_types.begin(), acceptable_adjacent_tile_types.end(), RawTiles[x + 1][y - 1]) == acceptable_adjacent_tile_types.end()) {
					ne_quadrant_adjacent_tiles += 1;
					sw_quadrant_adjacent_tiles += 1;
				}
				if ((x + 1) < map_width && (y + 1) < map_height && std::find(acceptable_adjacent_tile_types.begin(), acceptable_adjacent_tile_types.end(), RawTiles[x + 1][y + 1]) == acceptable_adjacent_tile_types.end()) {
					se_quadrant_adjacent_tiles += 1;
					nw_quadrant_adjacent_tiles += 1;
				}
				
				
				if (horizontal_adjacent_tiles >= 2 || vertical_adjacent_tiles >= 2 || nw_quadrant_adjacent_tiles >= 4 || ne_quadrant_adjacent_tiles >= 4 || sw_quadrant_adjacent_tiles >= 4 || se_quadrant_adjacent_tiles >= 4) {
					RawTiles[x][y] = base_tile;
					no_irregularities_found = false;
				}
			}
		}
	}
}

void AdjustRawTileMapTransitions(int map_width, int map_height)
{
	for (int x = 0; x < map_width; ++x) {
		for (int y = 0; y < map_height; ++y) {
			if (RawTiles[x][y] != "Rock" && RawTiles[x][y] != "Water" && RawTiles[x][y] != "Rough" && RawTiles[x][y] != "Dark-Rough") {
				for (int sub_x = -1; sub_x <= 1; ++sub_x) {
					for (int sub_y = -1; sub_y <= 1; ++sub_y) {
						if ((x + sub_x) < 0 || (x + sub_x) >= map_width || (y + sub_y) < 0 || (y + sub_y) >= map_height) {
							continue;
						}
						if (RawTiles[x + sub_x][y + sub_y] == "Rock" || RawTiles[x + sub_x][y + sub_y] == "Water" || RawTiles[x + sub_x][y + sub_y] == "Dark-Rough") {
							RawTiles[x][y] = "Rough";
						}
					}
				}
			}
		}
	}
}
//Wyrmgus end

//@}
