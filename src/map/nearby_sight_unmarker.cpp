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
//      (c) Copyright 2021-2022 by Andrettin
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

#include "map/nearby_sight_unmarker.h"

#include "unit/unit.h"
#include "unit/unit_find.h"
#include "util/assert_util.h"

namespace wyrmgus {

nearby_sight_unmarker::nearby_sight_unmarker(const CUnit *unit)
{
	assert_throw(unit != nullptr);

	SelectAroundUnit(*unit, CUnit::max_sight_range, this->nearby_units);

	this->unmark_units();
}

nearby_sight_unmarker::nearby_sight_unmarker(const QPoint &tile_pos, const int z)
{
	static constexpr QPoint range(CUnit::max_sight_range, CUnit::max_sight_range);

	Select(tile_pos - range, tile_pos + range, this->nearby_units, z);
}

nearby_sight_unmarker::~nearby_sight_unmarker()
{
	for (CUnit *unit : this->nearby_units) {
		MapMarkUnitSight(*unit);
	}
}

void nearby_sight_unmarker::unmark_units()
{
	for (CUnit *unit : this->nearby_units) {
		MapUnmarkUnitSight(*unit);
	}
}

}
