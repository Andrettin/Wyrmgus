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
//      (c) Copyright 1999-2015 by Vladi Shabanski and Andrettin
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
-- Includes
----------------------------------------------------------------------------*/

#include <cstdio>

#include "stratagus.h"
#include "map/map.h"
#include "map/terrain_type.h"
#include "map/tileset.h"
#include "ui/ui.h"
#include "player.h"
#include "settings.h"
#include "unit/unit_find.h"
#include "unit/unit_type.h"

/*----------------------------------------------------------------------------
-- Functions
----------------------------------------------------------------------------*/

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
static int GetDirectionFromSurrounding(const Vector2i &pos, bool human, bool seen)
{
	const Vector2i offsets[4] = {Vector2i(0, -1), Vector2i(1, 0), Vector2i(0, 1), Vector2i(-1, 0)};
	int dirFlag = 0;

	for (int i = 0; i != 4; ++i) {
		const Vector2i newpos = pos + offsets[i];

		if (!CMap::Map.Info.IsPointOnMap(newpos)) {
			dirFlag |= 1 << i;
		} else {
			const CMapField &mf = *CMap::Map.Field(newpos);
			const unsigned int tile = seen ? mf.playerInfo.SeenTile : mf.getGraphicTile();

			if (CMap::Map.Tileset->isARaceWallTile(tile, human)) {
				dirFlag |= 1 << i;
			}
		}
	}
	return dirFlag;
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
static void MapFixWallNeighbors(const Vector2i &pos)
{
	const Vector2i offset[] = {Vector2i(1, 0), Vector2i(-1, 0), Vector2i(0, 1), Vector2i(0, -1)};

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
void CMap::RemoveWall(const Vector2i &pos)
{
	CMapField &mf = *Field(pos);

	mf.Value = 0;

	MapFixWallTile(pos);
	mf.Flags &= ~(MapFieldWall | MapFieldUnpassable);
	MapFixWallNeighbors(pos);

	UI.Minimap.UpdateXY(pos);

	if (mf.playerInfo.IsTeamVisible(*CPlayer::GetThisPlayer())) {
		UI.Minimap.UpdateSeenXY(pos);
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
//void CMap::HitWall(const Vector2i &pos, unsigned damage)
void CMap::HitWall(const Vector2i &pos, unsigned damage, int z)
//Wyrmgus end
{
	//Wyrmgus start
//	const unsigned v = this->Field(pos)->Value;
	const unsigned v = this->Field(pos, z)->Value;
	//Wyrmgus end

	if (v <= damage) {
		//Wyrmgus start
//		RemoveWall(pos);
		ClearOverlayTile(pos, z);
		//Wyrmgus end
	} else {
		//Wyrmgus start
//		this->Field(pos)->Value = v - damage;
		this->Field(pos, z)->Value = v - damage;
//		MapFixWallTile(pos);
		if (this->Field(pos, z)->OverlayTerrain->UnitType && this->Field(pos, z)->Value <= this->Field(pos, z)->OverlayTerrain->UnitType->MapDefaultStat.Variables[HP_INDEX].Max / 2) {
			this->SetOverlayTerrainDamaged(pos, true, z);
		}
		//Wyrmgus end
	}
}
