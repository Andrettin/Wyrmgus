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
/**@name map_wall.cpp - The map wall handling. */
//
//      (c) Copyright 1999-2021 by Vladi Shabanski and Andrettin
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

#include "stratagus.h"

#include "map/map.h"

#include "map/terrain_type.h"
#include "map/tile.h"
#include "ui/ui.h"
#include "player.h"
#include "settings.h"
#include "unit/unit_find.h"
#include "unit/unit_type.h"

/*----------------------------------------------------------------------------
-- Fix walls (connections)
----------------------------------------------------------------------------*/

/*
  Vladi:
  NOTE: this is not the original behaviour of the wall demolishing,
  instead I'm replacing tiles just as the wood fixing, so if part of
  a wall is down side neighbours are fixed just as current tile is
  empty one. It is still nice... :)

  For the connecting new walls -- all's fine.
*/

//Wyrmgus start
/*
//  Calculate the correct tile. Depends on the surrounding.
static int GetDirectionFromSurrounding(const Vec2i &pos, bool human, bool seen)
{
	const Vec2i offsets[4] = {Vec2i(0, -1), Vec2i(1, 0), Vec2i(0, 1), Vec2i(-1, 0)};
	int dirFlag = 0;

	for (int i = 0; i != 4; ++i) {
		const Vec2i newpos = pos + offsets[i];

		if (!CMap::Map.Info.IsPointOnMap(newpos)) {
			dirFlag |= 1 << i;
		} else {
			const wyrmgus::tile &mf = *Map.Field(newpos);
			const unsigned int tile = seen ? mf.player_info->SeenTile : mf.getGraphicTile();

			if (Map.Tileset->isARaceWallTile(tile, human)) {
				dirFlag |= 1 << i;
			}
		}
	}
	return dirFlag;
}
*/
//Wyrmgus end

/**
** Correct the seen wall field, depending on the surrounding.
**
** @param pos Map tile-position.
*/
//Wyrmgus start
/*
void MapFixSeenWallTile(const Vec2i &pos)
{
	//  Outside of map or no wall.
	if (!Map.Info.IsPointOnMap(pos)) {
		return;
	}
	wyrmgus::tile &mf = *Map.Field(pos);
	const CTileset &tileset = *Map.Tileset;
	const unsigned tile = mf.player_info->SeenTile;
	if (!tileset.isAWallTile(tile)) {
		return;
	}
	const bool human = tileset.isARaceWallTile(tile, true);
	const int dirFlag = GetDirectionFromSurrounding(pos, human, true);
	const int wallTile = getWallTile(tileset, human, dirFlag, mf.get_value(), tile);

	if (mf.player_info->SeenTile != wallTile) { // Already there!
		mf.player_info->SeenTile = wallTile;
		// FIXME: can this only happen if seen?
		if (mf.player_info->IsTeamVisible(*ThisPlayer)) {
			UI.get_minimap()->UpdateSeenXY(pos);
		}
	}
}
*/
//Wyrmgus end

/**
** Correct the surrounding seen wall fields.
**
** @param pos Map tile-position.
*/
//Wyrmgus start
/*
void MapFixSeenWallNeighbors(const Vec2i &pos)
{
	const Vec2i offset[] = {Vec2i(1, 0), Vec2i(-1, 0), Vec2i(0, 1), Vec2i(0, -1)};

	for (unsigned int i = 0; i < 4; ++i) {
		MapFixSeenWallTile(pos + offset[i]);
	}
}
*/
//Wyrmgus end

/**
** Correct the real wall field, depending on the surrounding.
**
** @param pos Map tile-position.
*/
//Wyrmgus start
/*
void MapFixWallTile(const Vec2i &pos)
{
	//  Outside of map or no wall.
	if (!Map.Info.IsPointOnMap(pos)) {
		return;
	}
	wyrmgus::tile &mf = *Map.Field(pos);
	const CTileset &tileset = *Map.Tileset;
	const int tile = mf.getGraphicTile();
	if (!tileset.isAWallTile(tile)) {
		return;
	}
	const bool human = tileset.isARaceWallTile(tile, true);
	const int dirFlag = GetDirectionFromSurrounding(pos, human, false);
	const unsigned int wallTile = getWallTile(tileset, human, dirFlag, mf.get_value(), tile);

	if (mf.getGraphicTile() != wallTile) {
		mf.setGraphicTile(wallTile);
		UI.get_minimap()->UpdateXY(pos);

		if (mf.player_info->IsTeamVisible(*ThisPlayer)) {
			UI.get_minimap()->UpdateSeenXY(pos);
			Map.MarkSeenTile(mf);
		}
	}
}
*/
//Wyrmgus end

/**
** Correct the surrounding real wall fields.
**
** @param pos Map tile-position.
*/
//Wyrmgus start
/*
static void MapFixWallNeighbors(const Vec2i &pos)
{
	const Vec2i offset[] = {Vec2i(1, 0), Vec2i(-1, 0), Vec2i(0, 1), Vec2i(0, -1)};

	for (unsigned int i = 0; i < sizeof(offset) / sizeof(*offset); ++i) {
		MapFixWallTile(pos + offset[i]);
	}
}
*/
//Wyrmgus end

/**
** Remove wall from the map.
**
** @param pos  Map position.
**
** FIXME: support more walls of different races.
*/
//Wyrmgus start
/*
void CMap::RemoveWall(const Vec2i &pos)
{
	wyrmgus::tile &mf = *Field(pos);

	mf.set_value(0);

	MapFixWallTile(pos);
	mf.Flags &= ~(tile_flag::wall | tile_flag::impassable);
	MapFixWallNeighbors(pos);

	UI.get_minimap()->UpdateXY(pos);

	if (mf.player_info->IsTeamVisible(*ThisPlayer)) {
		UI.get_minimap()->UpdateSeenXY(pos);
		this->MarkSeenTile(mf);
	}
}
*/
//Wyrmgus end

/**
** Set wall onto the map.
**
** @param pos  Map position.
** @param humanwall Flag, if true set a human wall.
**
** @todo FIXME: support for more races.
*/
//Wyrmgus start
/*
void CMap::SetWall(const Vec2i &pos, bool humanwall)
{
	wyrmgus::tile &mf = *Field(pos);

	if (humanwall) {
		const int value = UnitTypeHumanWall->MapDefaultStat.Variables[HP_INDEX].Max;
		mf.setTileIndex(*Tileset, Tileset->getHumanWallTileIndex(0), value);
	} else {
		const int value = UnitTypeOrcWall->MapDefaultStat.Variables[HP_INDEX].Max;
		mf.setTileIndex(*Tileset, Tileset->getOrcWallTileIndex(0), value);
	}

	UI.get_minimap()->UpdateXY(pos);
	MapFixWallTile(pos);
	MapFixWallNeighbors(pos);

	if (mf.player_info->IsTeamVisible(*ThisPlayer)) {
		UI.get_minimap()->UpdateSeenXY(pos);
		this->MarkSeenTile(mf);
	}
}
*/
//Wyrmgus end

/**
** Wall is hit with damage.
**
** @param pos     Map tile-position of wall.
** @param damage  Damage done to wall.
*/
//Wyrmgus start
//void CMap::HitWall(const Vec2i &pos, unsigned damage)
void CMap::HitWall(const Vec2i &pos, unsigned damage, int z)
//Wyrmgus end
{
	//Wyrmgus start
//	const unsigned v = this->Field(pos)->get_value();
	const unsigned v = this->Field(pos, z)->get_value();
	//Wyrmgus end

	if (v <= damage) {
		//Wyrmgus start
//		RemoveWall(pos);
		ClearOverlayTile(pos, z);
		//Wyrmgus end
	} else {
		//Wyrmgus start
//		this->Field(pos)->set_value(v - damage);
		this->Field(pos, z)->set_value(v - damage);
//		MapFixWallTile(pos);
		if (this->Field(pos, z)->get_overlay_terrain()->UnitType && this->Field(pos, z)->get_value() <= this->Field(pos, z)->get_overlay_terrain()->UnitType->MapDefaultStat.Variables[HP_INDEX].Max / 2) {
			this->SetOverlayTerrainDamaged(pos, true, z);
		}
		//Wyrmgus end
	}
}
