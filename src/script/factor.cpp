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
//      (c) Copyright 2021-2022 by Andrettin
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

#include "script/factor.h"

#include "database/database.h"
#include "database/gsml_data.h"
#include "database/gsml_operator.h"
#include "database/gsml_property.h"
#include "script/factor_modifier.h"

namespace wyrmgus {

template <typename scope_type>
factor<scope_type>::factor()
{
}

template <typename scope_type>
factor<scope_type>::factor(const int base_value) : base_value(base_value)
{
}

template <typename scope_type>
factor<scope_type>::~factor()
{
}

template <typename scope_type>
void factor<scope_type>::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const gsml_operator gsml_operator = property.get_operator();
	const std::string &value = property.get_value();

	if (key == "base_value") {
		if (gsml_operator == gsml_operator::assignment) {
			this->base_value = std::stoi(value);
		} else {
			throw std::runtime_error("Invalid operator for property \"" + key + "\".");
		}
	} else {
		throw std::runtime_error("Invalid factor property: \"" + key + "\".");
	}
}

template <typename scope_type>
void factor<scope_type>::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "modifier") {
		auto modifier = std::make_unique<factor_modifier<scope_type>>();
		database::process_gsml_data(modifier, scope);
		this->modifiers.push_back(std::move(modifier));
	} else {
		throw std::runtime_error("Invalid factor scope: \"" + scope.get_tag() + "\".");
	}
}

template <typename scope_type>
void factor<scope_type>::check() const
{
	if (this->base_value == 0) {
		throw std::runtime_error("Factor has a base value of 0.");
	}

	for (const std::unique_ptr<factor_modifier<scope_type>> &modifier : this->modifiers) {
		modifier->check_validity();
	}
}

template <typename scope_type>
int factor<scope_type>::calculate(const scope_type *scope) const
{
	int value = this->base_value;

	if (scope != nullptr) {
		for (const std::unique_ptr<factor_modifier<scope_type>> &modifier : this->modifiers) {
			if (modifier->check_conditions(scope)) {
				value = (value * modifier->get_factor()).to_int();
			}
		}
	}

	return value;
}

template class factor<CPlayer>;
template class factor<CUnit>;

}
