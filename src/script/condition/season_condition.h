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
//      (c) Copyright 2020-2021 by Andrettin
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

#pragma once

#include "script/condition/condition.h"
#include "time/season.h"
#include "unit/unit.h"
#include "util/string_util.h"

namespace wyrmgus {

class season;

class season_condition final : public condition
{
public:
	explicit season_condition(const std::string &value)
	{
		this->season = season::get(value);
	}

	virtual bool check(const CPlayer *player, const bool ignore_units) const override
	{
		Q_UNUSED(ignore_units)

		return CMap::Map.MapLayers[player->StartMapLayer]->get_tile_season(player->StartPos) == this->season;
	}

	virtual bool check(const CUnit *unit, const bool ignore_units) const override
	{
		Q_UNUSED(ignore_units)

		if (unit->MapLayer == nullptr) {
			return false;
		}

		const QPoint center_tile_pos = unit->get_center_tile_pos();

		return unit->MapLayer->get_tile_season(center_tile_pos) == this->season;
	}

	virtual std::string get_string(const size_t indent) const override
	{
		Q_UNUSED(indent)

		return string::highlight(this->season->get_name()) + " season";
	}

private:
	const wyrmgus::season *season = nullptr;
};

}
