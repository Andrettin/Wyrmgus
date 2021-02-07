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

#include "ai/ai_force_template.h"

#include "ai/ai_force_type.h"
#include "database/sml_data.h"
#include "database/sml_property.h"
#include "unit/unit_class.h"

namespace wyrmgus {

ai_force_template::ai_force_template()
	: force_type(ai_force_type::none)
{
}

void ai_force_template::process_sml_property(const sml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "force_type") {
		this->force_type = string_to_ai_force_type(value);
	} else if (key == "priority") {
		this->priority = std::stoi(value);
	} else if (key == "weight") {
		this->weight = std::stoi(value);
	} else {
		throw std::runtime_error("Invalid AI force template property: \"" + key + "\".");
	}
}

void ai_force_template::process_sml_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "units") {
		scope.for_each_property([&](const sml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			const unit_class *unit_class = unit_class::get(key);
			const int unit_quantity = std::stoi(value);
			this->add_unit(unit_class, unit_quantity);
		});
	} else {
		throw std::runtime_error("Invalid AI force template scope: \"" + tag + "\".");
	}
}

void ai_force_template::check() const
{
	if (this->get_force_type() == ai_force_type::none) {
		throw std::runtime_error("AI force template has \"none\" as its force type.");
	}
}

}
