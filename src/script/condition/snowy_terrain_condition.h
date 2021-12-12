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
//      (c) Copyright 2021 by Andrettin
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

#include "map/terrain_type.h"
#include "map/tile.h"
#include "script/condition/condition.h"
#include "time/season.h"
#include "util/string_conversion_util.h"

namespace wyrmgus {

class snowy_terrain_condition final : public condition
{
public:
	explicit snowy_terrain_condition(const std::string &value)
	{
		this->value = string::to_bool(value);
	}

	virtual bool check(const CPlayer *player, const read_only_context &ctx, const bool ignore_units) const override
	{
		Q_UNUSED(player)
		Q_UNUSED(ctx)
		Q_UNUSED(ignore_units)

		return true;
	}

	virtual bool check(const CUnit *unit, const read_only_context &ctx, const bool ignore_units) const override
	{
		Q_UNUSED(ctx)
		Q_UNUSED(ignore_units)

		if (unit->MapLayer == nullptr) {
			return false;
		}

		const tile *center_tile = unit->get_center_tile();
		if (center_tile == nullptr) {
			return false;
		}

		const season *season = unit->MapLayer->get_tile_season(unit->get_center_tile_pos());
		const terrain_type *tile_terrain = center_tile->get_terrain();

		return tile_terrain != nullptr && tile_terrain->is_snowy(season);
	}

	virtual std::string get_string(const size_t indent, const bool links_allowed) const override
	{
		Q_UNUSED(indent)
		Q_UNUSED(links_allowed)

		return "Is on snowy terrain";
	}

private:
	bool value = false;
};

}
