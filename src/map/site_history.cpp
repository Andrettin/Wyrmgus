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

#include "map/site_history.h"

#include "database/sml_data.h"
#include "map/site.h"
#include "unit/unit_class.h"
#include "util/container_util.h"
#include "util/vector_util.h"

namespace wyrmgus {

void site_history::process_sml_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "population_groups") {
		scope.for_each_property([&](const sml_property &property) {
			const unit_class *unit_class = unit_class::get(property.get_key());
			this->population_groups[unit_class] = std::stoll(property.get_value());
		});
	} else {
		data_entry_history::process_sml_scope(scope);
	}
}

QVariantList site_history::get_building_classes_qvariant_list() const
{
	return container::to_qvariant_list(this->get_building_classes());
}

void site_history::add_building_class(unit_class *building_class)
{
	if (building_class->is_town_hall()) {
		if (!this->site->is_settlement()) {
			throw std::runtime_error("Tried to add a settlement head building to a non-settlement site.");
		}

		//remove other settlement head buildings (there can be only one at a time for a given settlement)
		for (size_t i = 0; i < this->building_classes.size();) {
			unit_class *other_building_class = this->building_classes[i];

			if (other_building_class->is_town_hall()) {
				this->building_classes.erase(this->building_classes.begin() + i);
			} else {
				i++;
			}
		}
	}

	this->building_classes.push_back(building_class);
}

void site_history::remove_building_class(unit_class *building_class)
{
	if (building_class->is_town_hall()) {
		if (!vector::contains(this->building_classes, building_class)) {
			return;
		}
	}

	vector::remove_one(this->building_classes, building_class);
}

}
