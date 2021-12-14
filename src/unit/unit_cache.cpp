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

#include "stratagus.h"

#include "unit/unit_cache.h"

#include "map/map.h"
#include "map/map_info.h"
#include "map/map_layer.h"
#include "map/tile.h"
#include "unit/unit.h"
#include "unit/unit_type.h"
#include "util/assert_util.h"

CUnit *CUnitCache::Remove(const unsigned int index)
{
	const size_t size = Units.size();
	assert_throw(index < size);
	CUnit *tmp = Units[index];
	if (size > 1) {
		Units[index] = Units[size - 1];
	}
	Units.pop_back();
	return tmp;
}

/**
**  Insert new unit into cache.
**
**  @param unit  Unit pointer to place in cache.
*/
void CMap::Insert(CUnit &unit)
{
	assert_throw(!unit.Removed);
	unsigned int index = unit.Offset;
	const int w = unit.Type->get_tile_width();
	const int h = unit.Type->get_tile_height();
	int j, i = h;

	do {
		wyrmgus::tile *mf = unit.MapLayer->Field(index);
		j = w;
		do {
			mf->UnitCache.Insert(&unit);
			++mf;
		} while (--j && unit.tilePos.x + (j - w) < unit.MapLayer->get_width());
		index += unit.MapLayer->get_width();
	} while (--i && unit.tilePos.y + (i - h) < unit.MapLayer->get_height());
}

/**
**  Remove unit from cache.
**
**  @param unit  Unit pointer to remove from cache.
*/
void CMap::Remove(CUnit &unit)
{
	assert_throw(!unit.Removed);
	unsigned int index = unit.Offset;
	const int w = unit.Type->get_tile_width();
	const int h = unit.Type->get_tile_height();
	int j, i = h;

	do {
		wyrmgus::tile *mf = unit.MapLayer->Field(index);
		j = w;
		do {
			mf->UnitCache.Remove(&unit);
			++mf;
		} while (--j && unit.tilePos.x + (j - w) < unit.MapLayer->get_width());
		index += unit.MapLayer->get_width();
	} while (--i && unit.tilePos.y + (i - h) < unit.MapLayer->get_height());
}
