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
//      (c) Copyright 2020-2022 by Andrettin
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

#include "map/tile.h"
#include "script/condition/condition.h"
#include "time/time_of_day.h"
#include "util/string_util.h"

namespace wyrmgus {

class time_of_day_condition final : public condition
{
public:
	explicit time_of_day_condition(const std::string &value)
	{
		this->time_of_day = time_of_day::get(value);
	}

	virtual bool check(const CPlayer *player, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return CMap::get()->MapLayers[player->StartMapLayer]->get_tile_time_of_day(player->StartPos) == this->time_of_day;
	}

	virtual bool check(const CUnit *unit, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		if (unit->MapLayer == nullptr) {
			return false;
		}

		const wyrmgus::time_of_day *unit_time_of_day = unit->get_center_tile_time_of_day();
		return this->time_of_day == unit_time_of_day;
	}

	virtual std::string get_string(const size_t indent, const bool links_allowed) const override
	{
		Q_UNUSED(indent)
		Q_UNUSED(links_allowed)

		return string::highlight(this->time_of_day->get_name()) + " time of day";
	}

private:
	const wyrmgus::time_of_day *time_of_day = nullptr;
};

}
