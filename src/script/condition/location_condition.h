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
//      (c) Copyright 2020 by Andrettin
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

#include "map/map_template.h"
#include "script/condition/condition.h"
#include "unit/unit.h"

namespace wyrmgus {

class location_condition final : public condition
{
public:
	location_condition()
	{
	}

	virtual void process_sml_property(const sml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "map_template") {
			this->map_template = map_template::get(value);
		} else {
			throw std::runtime_error("Invalid location condition property: \"" + property.get_key() + "\".");
		}
	}

	virtual bool check(const CPlayer *player, const bool ignore_units) const override
	{
		Q_UNUSED(player)
		Q_UNUSED(ignore_units)

		return true;
	}

	virtual bool check(const CUnit *unit, const bool ignore_units) const override
	{
		Q_UNUSED(ignore_units)

		return unit->is_in_subtemplate_area(this->map_template);
	}

	virtual std::string get_string(const std::string &prefix = "") const override
	{
		return prefix;
	}

private:
	const wyrmgus::map_template *map_template = nullptr;
};

}
