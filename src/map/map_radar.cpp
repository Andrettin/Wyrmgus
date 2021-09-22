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

#include "stratagus.h"

#include "map/map.h"

#include "map/map_info.h"
#include "map/map_layer.h"
#include "map/tile.h"
#include "player/player.h"
#include "unit/unit.h"
#include "unit/unit_type.h"
#include "util/assert_util.h"

static inline unsigned char IsTileRadarVisible(const CPlayer &pradar, const CPlayer &punit, const wyrmgus::tile_player_info &mfp)
{
	if (mfp.RadarJammer[punit.get_index()]) {
		return 0;
	}

	const int p = pradar.get_index();
	if (pradar.is_vision_sharing()) {
		const std::array<unsigned char, PlayerMax> &radar = mfp.Radar;
		const std::array<unsigned char, PlayerMax> &jamming = mfp.RadarJammer;
		unsigned char radarvision = 0;

		// Check jamming first, if we are jammed, exit
		for (const int i : punit.get_shared_vision()) {
			if (i == p) {
				continue;
			}

			if (jamming[i] > 0) {
				if (CPlayer::Players[i]->has_shared_vision_with(&punit)) { //if the shared vision is mutual
					// We are jammed, return nothing
					return 0;
				}
			}
		}

		for (const int i : pradar.get_shared_vision()) {
			if (i == p) {
				continue;
			}

			if (radar[i] > 0) {
				if (CPlayer::Players[i]->has_shared_vision_with(p)) { //if the shared vision is mutual
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
	const int x_max = Type->get_tile_width();
	unsigned int index = Offset;
	int j = Type->get_tile_height();
	do {
		const wyrmgus::tile *mf = this->MapLayer->Field(index);

		int i = x_max;
		do {
			if (IsTileRadarVisible(pradar, *Player, *mf->player_info) != 0) {
				return true;
			}
			++mf;
		} while (--i);
		index += this->MapLayer->get_width();
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
	assert_throw(CMap::get()->Field(index, z)->player_info->Radar[player.get_index()] != 255);
	CMap::get()->Field(index, z)->player_info->Radar[player.get_index()]++;
}

void MapMarkTileRadar(const CPlayer &player, int x, int y, int z)
{
	assert_throw(CMap::get()->Info->IsPointOnMap(x, y, z));
	MapMarkTileRadar(player, CMap::get()->get_pos_index(x, y, z), z);
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
	//Wyrmgus start
//	unsigned char *v = &(CMap::get()->Field(index)->player_info->Radar[player.get_index()]);
	unsigned char *v = &(CMap::get()->Field(index, z)->player_info->Radar[player.get_index()]);
	//Wyrmgus end
	if (*v) {
		--*v;
	}
}

void MapUnmarkTileRadar(const CPlayer &player, int x, int y, int z)
{
	assert_throw(CMap::get()->Info->IsPointOnMap(x, y, z));
	MapUnmarkTileRadar(player, CMap::get()->get_pos_index(x, y, z), z);
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
	//Wyrmgus start
//	assert_throw(CMap::get()->Field(index)->player_info->RadarJammer[player.Index] != 255);
//	CMap::get()->Field(index)->player_info->RadarJammer[player.Index]++;
	assert_throw(CMap::get()->Field(index, z)->player_info->RadarJammer[player.get_index()] != 255);
	CMap::get()->Field(index, z)->player_info->RadarJammer[player.get_index()]++;
	//Wyrmgus end
}

void MapMarkTileRadarJammer(const CPlayer &player, int x, int y, int z)
{
	assert_throw(CMap::get()->Info->IsPointOnMap(x, y, z));
	MapMarkTileRadarJammer(player, CMap::get()->get_pos_index(x, y, z), z);
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
	//Wyrmgus start
//	unsigned char *v = &(CMap::get()->Field(index)->player_info->RadarJammer[player.get_index()]);
	unsigned char *v = &(CMap::get()->Field(index, z)->player_info->RadarJammer[player.get_index()]);
	//Wyrmgus end
	if (*v) {
		--*v;
	}
}

void MapUnmarkTileRadarJammer(const CPlayer &player, int x, int y, int z)
{
	assert_throw(CMap::get()->Info->IsPointOnMap(x, y, z));
	MapUnmarkTileRadarJammer(player, CMap::get()->get_pos_index(x, y, z), z);
}
