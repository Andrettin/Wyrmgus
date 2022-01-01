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

#include "stratagus.h"

#include "map/character_unit.h"

#include "database/sml_data.h"
#include "database/sml_property.h"
#include "player/player.h"
#include "unit/unit.h"
#include "unit/unit_type.h"
#include "util/string_conversion_util.h"
#include "util/vector_random_util.h"

namespace wyrmgus {

void character_unit::process_sml_property(const sml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "unit_type") {
		this->unit_types.clear();
		this->unit_types.push_back(unit_type::get(value));
	} else if (key == "ai_active") {
		this->ai_active = string::to_bool(value);
	} else {
		throw std::runtime_error("Invalid character unit property: \"" + key + "\".");
	}
}

void character_unit::process_sml_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "unit_types") {
		for (const std::string &value : values) {
			this->unit_types.push_back(unit_type::get(value));
		}

		scope.for_each_property([&](const sml_property &property) {
			const unit_type *unit_type = unit_type::get(property.get_key());
			const int weight = std::stoi(property.get_value());

			for (int i = 0; i < weight; ++i) {
				this->unit_types.push_back(unit_type);
			}
		});
	} else {
		throw std::runtime_error("Invalid character unit scope: \"" + tag + "\".");
	}
}

void character_unit::create_at(const QPoint &pos, const int z) const
{
	const unit_type *unit_type = vector::get_random(this->unit_types);

	if (unit_type == nullptr) {
		//no unit type means the unit doesn't get generated (this is valid if "none" was given for the unit type)
		return;
	}

	CUnit *unit = CreateUnit(pos - unit_type->get_tile_center_pos_offset(), *unit_type, CPlayer::get_neutral_player(), z);
	unit->Active = this->ai_active;
}

}
