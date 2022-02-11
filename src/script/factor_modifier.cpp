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

#include "script/factor_modifier.h"

#include "database/gsml_data.h"
#include "database/gsml_operator.h"
#include "database/gsml_property.h"
#include "script/condition/and_condition.h"
#include "script/context.h"

namespace wyrmgus {

template <typename scope_type>
factor_modifier<scope_type>::factor_modifier()
{
	this->conditions = std::make_unique<and_condition>();
}

template <typename scope_type>
factor_modifier<scope_type>::~factor_modifier()
{
}

template <typename scope_type>
void factor_modifier<scope_type>::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const gsml_operator gsml_operator = property.get_operator();
	const std::string &value = property.get_value();

	if (key == "factor") {
		if (gsml_operator == gsml_operator::assignment) {
			this->factor = centesimal_int(value);
		} else {
			throw std::runtime_error("Invalid operator for property (\"" + property.get_key() + "\").");
		}
	} else {
		std::unique_ptr<const condition> condition = wyrmgus::condition::from_gsml_property(property);
		this->conditions->add_condition(std::move(condition));
	}
}

template <typename scope_type>
void factor_modifier<scope_type>::process_gsml_scope(const gsml_data &scope)
{
	std::unique_ptr<const condition> condition = wyrmgus::condition::from_gsml_scope(scope);
	this->conditions->add_condition(std::move(condition));
}

template <typename scope_type>
void factor_modifier<scope_type>::check_validity() const
{
	if (this->factor == 0) {
		throw std::runtime_error("Factor modifier has a factor of 0.");
	}

	this->conditions->check_validity();
}

template <typename scope_type>
bool factor_modifier<scope_type>::check_conditions(const scope_type *scope) const
{
	return this->conditions->check(scope, read_only_context::from_scope(scope));
}

template class factor_modifier<CPlayer>;
template class factor_modifier<CUnit>;

}
