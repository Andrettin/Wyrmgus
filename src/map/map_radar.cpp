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
/**@name map_radar.cpp - The map radar handling. */
//
//      (c) Copyright 2004-2006 by Russell Smith and Jimmy Salmon.
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

#include "map/map.h"

#include "map/map_layer.h"
#include "player.h"
#include "unit/unit.h"
#include "unit/unittype.h"


/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

static inline unsigned char
IsTileRadarVisible(const CPlayer &pradar, const CPlayer &punit, const CMapFieldPlayerInfo &mfp)
{
	if (mfp.RadarJammer[punit.Index]) {
		return 0;
	}

	int p = pradar.Index;
	if (pradar.IsVisionSharing()) {
		const unsigned char *const radar = mfp.Radar;
		const unsigned char *const jamming = mfp.RadarJammer;
		unsigned char radarvision = 0;
		// Check jamming first, if we are jammed, exit
		for (int i = 0; i < PlayerMax; ++i) {
			if (i != p) {
				if (jamming[i] > 0 && punit.IsBothSharedVision(*CPlayer::Players[i])) {
					// We are jammed, return nothing
					return 0;
				}
				if (radar[i] > 0 && pradar.IsBothSharedVision(*CPlayer::Players[i])) {
					radarvision |= radar[i];
				}
			}
		}
		// Can't exit until the end, as we might be jammed
		return (radarvision | mfp.Radar[p]);
	}
	return mfp.Radar[p];
}


bool CUnit::IsVisibleOnRadar(const CPlayer &pradar) const
{
	const int x_max = Type->TileSize.x;
	unsigned int index = Offset;
	int j = Type->TileSize.y;
	do {
		const CMapField *mf = this->MapLayer->Field(index);

		int i = x_max;
		do {
			if (IsTileRadarVisible(pradar, *Player, mf->playerInfo) != 0) {
				return true;
			}
			++mf;
		} while (--i);
		index += this->MapLayer->GetWidth();
	} while (--j);

	// Can't exit till the end, as we might be be able to see a different tile
	return false;
}


/**
**  Mark Radar Vision for a tile
**
**  @param player  The player you are marking for
**  @param x       the X tile to mark.
**  @param y       the Y tile to mark.
*/
void MapMarkTileRadar(const CPlayer &player, const unsigned int index, int z)
{
	Assert(CMap::Map.Field(index, z)->playerInfo.Radar[player.Index] != 255);
	CMap::Map.Field(index, z)->playerInfo.Radar[player.Index]++;
}

void MapMarkTileRadar(const CPlayer &player, int x, int y, int z)
{
	Assert(CMap::Map.Info.IsPointOnMap(x, y, z));
	MapMarkTileRadar(player, CMap::Map.getIndex(x, y, z), z);
}


/**
**  Unmark Radar Vision for a tile
**
**  @param player  The player you are marking for
**  @param x       the X tile to mark.
**  @param y       the Y tile to mark.
*/
void MapUnmarkTileRadar(const CPlayer &player, const unsigned int index, int z)
{
	// Reduce radar coverage if it exists.
	unsigned char *v = &(CMap::Map.Field(index, z)->playerInfo.Radar[player.Index]);
	if (*v) {
		--*v;
	}
}

void MapUnmarkTileRadar(const CPlayer &player, int x, int y, int z)
{
	Assert(CMap::Map.Info.IsPointOnMap(x, y, z));
	MapUnmarkTileRadar(player, CMap::Map.getIndex(x, y, z), z);
}


/**
**  Mark Radar Jamming Vision for a tile
**
**  @param player  The player you are marking for
**  @param x       the X tile to mark.
**  @param y       the Y tile to mark.
*/
//Wyrmgus start
//void MapMarkTileRadarJammer(const CPlayer &player, const unsigned int index)
void MapMarkTileRadarJammer(const CPlayer &player, const unsigned int index, int z)
//Wyrmgus end
{
	Assert(CMap::Map.Field(index, z)->playerInfo.RadarJammer[player.Index] != 255);
	CMap::Map.Field(index, z)->playerInfo.RadarJammer[player.Index]++;
}

void MapMarkTileRadarJammer(const CPlayer &player, int x, int y, int z)
{
	Assert(CMap::Map.Info.IsPointOnMap(x, y, z));
	MapMarkTileRadarJammer(player, CMap::Map.getIndex(x, y, z), z);
}

/**
**  Unmark Radar Vision for a tile
**
**  @param player  The player you are marking for
**  @param x       the X tile to mark.
**  @param y       the Y tile to mark.
*/
//Wyrmgus start
//void MapUnmarkTileRadarJammer(const CPlayer &player, const unsigned int index)
void MapUnmarkTileRadarJammer(const CPlayer &player, const unsigned int index, int z)
//Wyrmgus end
{
	// Reduce radar coverage if it exists.
	unsigned char *v = &(CMap::Map.Field(index, z)->playerInfo.RadarJammer[player.Index]);
	if (*v) {
		--*v;
	}
}

void MapUnmarkTileRadarJammer(const CPlayer &player, int x, int y, int z)
{
	Assert(CMap::Map.Info.IsPointOnMap(x, y, z));
	MapUnmarkTileRadarJammer(player, CMap::Map.getIndex(x, y, z), z);
}

void MapMarkTileOwnership(const CPlayer &player, const unsigned int index, int z)
{
}

void MapMarkTileOwnership(const CPlayer &player, int x, int y, int z)
{
	Assert(CMap::Map.Info.IsPointOnMap(x, y, z));
	MapMarkTileOwnership(player, CMap::Map.getIndex(x, y, z), z);
}

void MapUnmarkTileOwnership(const CPlayer &player, const unsigned int index, int z)
{
}

void MapUnmarkTileOwnership(const CPlayer &player, int x, int y, int z)
{
	Assert(CMap::Map.Info.IsPointOnMap(x, y, z));
	MapUnmarkTileOwnership(player, CMap::Map.getIndex(x, y, z), z);
}
