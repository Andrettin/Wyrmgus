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

#include "population/population_type.h"

#include "economy/resource.h"
#include "player/civilization.h"
#include "player/civilization_group.h"
#include "population/population_class.h"

namespace wyrmgus {

void population_type::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "production_efficiency") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			const resource *resource = resource::get(key);
			this->production_efficiency_map[resource] = std::stoi(value);
		});
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void population_type::initialize()
{
	const wyrmgus::population_class *population_class = this->get_population_class();

	if (this->get_population_class() != nullptr) {
		if (population_class != nullptr) {
			if (this->civilization != nullptr) {
				this->civilization->set_class_population_type(population_class, this);
			} else if (this->civilization_group != nullptr) {
				this->civilization_group->set_class_population_type(population_class, this);
			}
		}
	}

	named_data_entry::initialize();
}

bool population_type::is_growable() const
{
	return this->get_population_class()->is_growable();
}

int population_type::get_production_efficiency(const resource *resource) const
{
	const auto find_iterator = this->production_efficiency_map.find(resource);

	if (find_iterator != this->production_efficiency_map.end()) {
		return find_iterator->second;
	}

	return this->get_population_class()->get_production_efficiency(resource);
}

}
