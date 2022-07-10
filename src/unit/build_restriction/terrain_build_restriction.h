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
//      (c) Copyright 1998-2022 by Lutz Sammer, Jimmy Salmon, Rafal Bursig
//		and Andrettin
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

#pragma once

#include "map/map.h"
#include "map/map_info.h"
#include "map/terrain_type.h"
#include "unit/build_restriction/build_restriction.h"
#include "util/assert_util.h"

namespace wyrmgus {

class terrain_build_restriction final : public build_restriction
{
public:
	terrain_build_restriction()
	{
	}

	explicit terrain_build_restriction(const std::string &value)
	{
		this->terrain = terrain_type::get(value);
	}

	virtual std::unique_ptr<build_restriction> duplicate() const override
	{
		auto b = std::make_unique<terrain_build_restriction>();
		b->terrain = this->terrain;
		return b;
	}

	virtual bool Check(const CUnit *builder, const unit_type &type, const QPoint &pos, CUnit *&ontoptarget, const int z) const override
	{
		Q_UNUSED(ontoptarget);
		Q_UNUSED(builder)

		assert_throw(CMap::get()->Info->IsPointOnMap(pos, z));

		for (int x = pos.x(); x < pos.x() + type.get_tile_width(); ++x) {
			for (int y = pos.y(); y < pos.y() + type.get_tile_height(); ++y) {
				if (!CMap::get()->Info->IsPointOnMap(x, y, z)) {
					continue;
				}

				const QPoint tile_pos(x, y);
				const terrain_type *tile_terrain = CMap::get()->GetTileTerrain(tile_pos, this->terrain->is_overlay(), z);

				if (this->terrain == tile_terrain) {
					return true;
				}
			}
		}

		return false;
	}

private:
	const terrain_type *terrain = nullptr;
};

}
