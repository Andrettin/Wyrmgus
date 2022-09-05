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

#include "database/gsml_data.h"
#include "database/gsml_property.h"
#include "population/employment_type.h"
#include "population/population_class.h"
#include "population/population_type.h"
#include "population/population_unit_key.h"
#include "util/assert_util.h"
#include "util/number_util.h"
#include "util/random.h"
#include "util/vector_util.h"

namespace wyrmgus {

int64_t population_unit::calculate_growth_quantity(const int64_t capacity, const int64_t current_population, const bool limit_to_population)
{
	int64_t quantity = std::max(capacity / population_unit::capacity_growth_divisor, population_unit::min_base_growth);
	quantity = std::min(quantity, capacity);
	if (limit_to_population) {
		quantity = std::min(quantity, current_population);
	}
	quantity = random::get()->generate_in_range<int64_t>(1, quantity);
	return quantity;
}

int64_t population_unit::calculate_population_growth_quantity(const int64_t population_growth_capacity, const int64_t current_population)
{
	const int64_t sign = number::sign(population_growth_capacity);
	const bool limit_to_population = population_growth_capacity < 0;

	const int64_t quantity = population_unit::calculate_growth_quantity(std::abs(population_growth_capacity), current_population, limit_to_population) * sign;
	return quantity;
}


bool population_unit::compare(const population_unit *lhs, const population_unit *rhs)
{
	if (lhs->get_population() != rhs->get_population()) {
		return lhs->get_population() > rhs->get_population();
	}

	const population_type *lhs_type = lhs->get_type();
	const population_type *rhs_type = rhs->get_type();

	const population_class *lhs_class = lhs_type->get_population_class();
	const population_class *rhs_class = rhs_type->get_population_class();

	if (lhs_class->promotes_to(rhs_class, true)) {
		return true;
	}

	if (rhs_class->promotes_to(lhs_class, true)) {
		return false;
	}

	return lhs_class->get_identifier() < rhs_class->get_identifier();
}

population_unit::population_unit(const population_unit_key &key, const int64_t population)
	: type(key.type), employment_type(key.employment_type), population(population)
{
	assert_throw(this->get_type() != nullptr);

	if (this->get_employment_type() != nullptr) {
		assert_throw(vector::contains(this->get_employment_type()->get_employees(), this->get_type()->get_population_class()));
	}

	assert_log(this->population >= 0);
	if (this->population < 0) {
		this->population = 0;
	}
}

void population_unit::process_gsml_property(const gsml_property &property)
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

void population_unit::process_gsml_scope(const gsml_data &scope)
{
	throw std::runtime_error("Invalid population unit scope: \"" + scope.get_tag() + "\".");
}

gsml_data population_unit::to_gsml_data() const
{
	gsml_data data;

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
	assert_log(population >= 0);

	this->population = population;

	emit population_changed();
}

const resource *population_unit::get_input_resource() const
{
	if (this->get_employment_type() != nullptr) {
		return this->get_employment_type()->get_input_resource();
	}

	return nullptr;
}

const resource *population_unit::get_output_resource() const
{
	if (this->get_employment_type() != nullptr) {
		return this->get_employment_type()->get_output_resource();
	}

	return this->get_type()->get_population_class()->get_unemployed_output_resource();
}

const centesimal_int &population_unit::get_output_multiplier() const
{
	if (this->get_employment_type() != nullptr) {
		return this->get_employment_type()->get_output_multiplier();
	}

	return this->get_type()->get_population_class()->get_unemployed_output_multiplier();
}

}
