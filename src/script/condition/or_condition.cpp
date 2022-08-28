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

#include "script/condition/or_condition.h"

#include "script/condition/and_condition.h"
#include "script/condition/upgrade_condition.h"

namespace wyrmgus {

template <typename scope_type>
or_condition<scope_type>::or_condition(const gsml_operator condition_operator)
	: condition<scope_type>(condition_operator)
{
}

template <typename scope_type>
or_condition<scope_type>::or_condition(std::vector<std::unique_ptr<const condition<scope_type>>> &&conditions)
	: condition<scope_type>(gsml_operator::assignment), conditions(std::move(conditions))
{
}

template <typename scope_type>
void or_condition<scope_type>::ProcessConfigDataSection(const CConfigData *section)
{
	std::unique_ptr<condition<scope_type>> condition = nullptr;

	if (section->Tag == "and") {
		condition = std::make_unique<and_condition<scope_type>>(gsml_operator::assignment);
	} else if (section->Tag == "or") {
		condition = std::make_unique<or_condition<scope_type>>(gsml_operator::assignment);
	} else if (section->Tag == "upgrade") {
		condition = std::make_unique<upgrade_condition<scope_type>>(gsml_operator::assignment);
	} else {
		fprintf(stderr, "Invalid or condition property: \"%s\".\n", section->Tag.c_str());
		return;
	}
	condition->ProcessConfigData(section);
	this->conditions.push_back(std::move(condition));
}

}
