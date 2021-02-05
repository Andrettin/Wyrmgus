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

#include "stratagus.h"

#include "map/landmass.h"

#include "database/sml_data.h"
#include "map/map.h"
#include "util/vector_util.h"
#include "world.h"

namespace wyrmgus {

void landmass::process_sml_property(const sml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "world") {
		this->world = world::get(value);
	} else {
		throw std::runtime_error("Invalid landmass property: \"" + key + "\".");
	}
}

void landmass::process_sml_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "border_landmasses") {
		for (const std::string &value : values) {
			const size_t index = std::stoul(value);
			const landmass *border_landmass = CMap::get()->get_landmasses()[index].get();
			this->add_border_landmass(border_landmass);
		}
	} else {
		throw std::runtime_error("Invalid landmass scope: \"" + scope.get_tag() + "\".");
	}
}

sml_data landmass::to_sml_data() const
{
	sml_data data;

	if (this->get_world() != nullptr) {
		data.add_property("world", this->get_world()->get_identifier());
	}

	if (!this->get_border_landmasses().empty()) {
		sml_data border_landmasses_data("border_landmasses");
		for (const landmass *border_landmass : this->get_border_landmasses()) {
			border_landmasses_data.add_value(std::to_string(border_landmass->get_index()));
		}
		data.add_child(std::move(border_landmasses_data));
	}

	return data;
}

bool landmass::borders_landmass(const landmass *landmass) const
{
	return vector::contains(this->border_landmasses, landmass);
}

}
