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
/**@name unit_cache.cpp - The unit cache. */
//
//      Cache to find units faster from position.
//      Sort of trivial implementation, since most queries are on a single tile.
//      Unit is just inserted in a double linked list for every tile it's on.
//
//      (c) Copyright 2004-2007 by Crestez Leonard and Jimmy Salmon
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

#include "stratagus.h"

#include "map/map.h"
#include "map/map_layer.h"
#include "unit/unit.h"
#include "unit/unit_type.h"

#include <algorithm>
#include <string>

/**
**  Insert new unit into cache.
**
**  @param unit  Unit pointer to place in cache.
*/
void CMap::Insert(CUnit &unit)
{
	Assert(!unit.Removed);
	unsigned int index = unit.Offset;
	const int w = unit.GetType()->TileSize.x;
	const int h = unit.GetType()->TileSize.y;
	int j, i = h;

	do {
		CMapField *mf = unit.GetMapLayer()->Field(index);
		j = w;
		do {
			mf->UnitCache.Insert(&unit);
			++mf;
		} while (--j && unit.GetTilePos().x + (j - w) < unit.GetMapLayer()->GetWidth());
		index += unit.GetMapLayer()->GetWidth();
	} while (--i && unit.GetTilePos().y + (i - h) < unit.GetMapLayer()->GetHeight());
}

/**
**  Remove unit from cache.
**
**  @param unit  Unit pointer to remove from cache.
*/
void CMap::Remove(CUnit &unit)
{
	Assert(!unit.Removed);
	unsigned int index = unit.Offset;
	const int w = unit.GetType()->TileSize.x;
	const int h = unit.GetType()->TileSize.y;
	int j, i = h;

	do {
		CMapField *mf = unit.GetMapLayer()->Field(index);
		j = w;
		do {
			mf->UnitCache.Remove(&unit);
			++mf;
		} while (--j && unit.GetTilePos().x + (j - w) < unit.GetMapLayer()->GetWidth());
		index += unit.GetMapLayer()->GetWidth();
	} while (--i && unit.GetTilePos().y + (i - h) < unit.GetMapLayer()->GetHeight());
}

//Wyrmgus start
//void CMap::Clamp(Vec2i &pos) const
void CMap::Clamp(Vec2i &pos, int z) const
//Wyrmgus end
{
	//Wyrmgus start
//	pos.x = std::clamp<short int>(pos.x, 0, this->Info.MapWidth - 1);
//	pos.y = std::clamp<short int>(pos.y, 0, this->Info.MapHeight - 1);
	pos.x = std::clamp<short int>(pos.x, 0, this->Info.MapWidths[z] - 1);
	pos.y = std::clamp<short int>(pos.y, 0, this->Info.MapHeights[z] - 1);
	//Wyrmgus end
}
