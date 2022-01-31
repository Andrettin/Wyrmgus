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

#include "population/population_unit.h"

#include "database/sml_data.h"
#include "database/sml_property.h"
#include "population/employment_type.h"
#include "population/population_type.h"
#include "population/population_unit_key.h"
#include "util/assert_util.h"

namespace wyrmgus {

population_unit::population_unit(const population_unit_key &key, const int64_t population)
	: type(key.type), employment_type(key.employment_type), population(population)
{
}

void population_unit::process_sml_property(const sml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "type") {
		this->type = population_type::get(value);
	} else if (key == "employment_type") {
		this->employment_type = employment_type::get(value);
	} else if (key == "population") {
		this->set_population(std::stoll(value));
	} else {
		throw std::runtime_error("Invalid population unit property: \"" + key + "\".");
	}
}

void population_unit::process_sml_scope(const sml_data &scope)
{
	throw std::runtime_error("Invalid population unit scope: \"" + scope.get_tag() + "\".");
}

sml_data population_unit::to_sml_data() const
{
	sml_data data;

	if (this->get_type() != nullptr) {
		data.add_property("type", this->get_type()->get_identifier());
	}

	if (this->get_employment_type() != nullptr) {
		data.add_property("employment_type", this->get_employment_type()->get_identifier());
	}

	if (this->get_population() != 0) {
		data.add_property("population", std::to_string(this->get_population()));
	}

	return data;
}

population_unit_key population_unit::get_key() const
{
	return population_unit_key(this->get_type(), this->get_employment_type());
}

void population_unit::set_population(const int64_t population)
{
	std::unique_lock<std::shared_mutex> lock(this->mutex);

	assert_log(population >= 0);

	this->population = population;

	emit population_changed();
}

}
