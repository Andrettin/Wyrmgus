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

#include "population/population_class.h"

#include "economy/resource.h"
#include "util/assert_util.h"
#include "util/vector_util.h"

namespace wyrmgus {

void population_class::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "promotion_targets") {
		for (const std::string &value : values) {
			population_class *other_class = population_class::get(value);
			this->promotion_targets.push_back(other_class);
			other_class->demotion_targets.push_back(this);
		}
	} else if (tag == "production_efficiency") {
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

void population_class::check() const
{
	assert_throw(this->get_resource_icon() != nullptr);

	for (const population_class *promotion_target : this->get_promotion_targets()) {
		if (vector::contains(promotion_target->get_promotion_targets(), this)) {
			throw std::runtime_error("Population classes \"" + this->get_identifier() + "\" and \"" + promotion_target->get_identifier() + "\" are both set as promotion targets of each other.");
		}
	}

	for (const population_class *demotion_target : this->get_demotion_targets()) {
		if (vector::contains(demotion_target->get_demotion_targets(), this)) {
			throw std::runtime_error("Population classes \"" + this->get_identifier() + "\" and \"" + demotion_target->get_identifier() + "\" are both set as demotion targets of each other.");
		}
	}
}

bool population_class::promotes_to(const population_class *other, const bool include_indirectly) const
{
	if (vector::contains(this->get_promotion_targets(), other)) {
		return true;
	}

	if (include_indirectly) {
		for (const population_class *promotion_target : this->get_promotion_targets()) {
			if (promotion_target->promotes_to(other, include_indirectly)) {
				return true;
			}
		}
	}

	return false;
}

}
