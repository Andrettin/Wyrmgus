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
//      (c) Copyright 2022 by Andrettin
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

#include "map/dungeon_generation_settings.h"

#include "character.h"
#include "database/database.h"
#include "unit/unit_type.h"

namespace wyrmgus {

void dungeon_generation_settings::process_gsml_property(const gsml_property &property)
{
	database::process_gsml_property_for_object(this, property);
}

void dungeon_generation_settings::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "units") {
		for (const std::string &value : values) {
			this->unit_types.push_back(unit_type::get(value));
		}

		scope.for_each_property([&](const gsml_property &property) {
			const unit_type *unit_type = unit_type::get(property.get_key());
			const int weight = std::stoi(property.get_value());

			for (int i = 0; i < weight; ++i) {
				this->unit_types.push_back(unit_type);
			}
		});
	} else if (tag == "items") {
		for (const std::string &value : values) {
			this->item_unit_types.push_back(unit_type::get(value));
		}

		scope.for_each_property([&](const gsml_property &property) {
			const unit_type *unit_type = unit_type::get(property.get_key());
			const int weight = std::stoi(property.get_value());

			for (int i = 0; i < weight; ++i) {
				this->item_unit_types.push_back(unit_type);
			}
		});
	} else if (tag == "traps") {
		for (const std::string &value : values) {
			this->trap_unit_types.push_back(unit_type::get(value));
		}

		scope.for_each_property([&](const gsml_property &property) {
			const unit_type *unit_type = unit_type::get(property.get_key());
			const int weight = std::stoi(property.get_value());

			for (int i = 0; i < weight; ++i) {
				this->trap_unit_types.push_back(unit_type);
			}
		});
	} else if (tag == "heroes") {
		for (const std::string &value : values) {
			this->heroes.push_back(character::get(value));
		}
	} else {
		database::process_gsml_scope_for_object(this, scope);
	}
}

}
