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
//      (c) Copyright 2023 by Andrettin
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

#include "script/conditional_string.h"

#include "database/gsml_property.h"
#include "script/condition/and_condition.h"
#include "util/assert_util.h"

namespace wyrmgus {

template <typename scope_type>
conditional_string<scope_type>::conditional_string()
{
	this->conditions = std::make_unique<and_condition<scope_type>>();
}

template <typename scope_type>
conditional_string<scope_type>::~conditional_string()
{
}

template <typename scope_type>
void conditional_string<scope_type>::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "text") {
		this->text = value;
	} else {
		this->conditions->process_gsml_property(property);
	}
}

template <typename scope_type>
void conditional_string<scope_type>::process_gsml_scope(const gsml_data &scope)
{
	this->conditions->process_gsml_scope(scope);
}

template <typename scope_type>
void conditional_string<scope_type>::check() const
{
	assert_throw(this->get_conditions() != nullptr);
	this->get_conditions()->check_validity();
}

template class conditional_string<CPlayer>;
template class conditional_string<CUnit>;

}
