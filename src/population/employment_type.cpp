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

#include "population/employment_type.h"

#include "population/population_class.h"
#include "util/assert_util.h"
#include "util/vector_util.h"

namespace wyrmgus {

void employment_type::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "employees") {
		for (const std::string &value : values) {
			this->employees.push_back(population_class::get(value));
		}
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

bool employment_type::can_employ(const population_class *population_class) const
{
	if (vector::contains(this->get_employees(), population_class)) {
		return true;
	}

	for (const wyrmgus::population_class *promotion_target : population_class->get_promotion_targets()) {
		if (vector::contains(this->get_employees(), promotion_target)) {
			return true;
		}
	}

	return false;
}

}
