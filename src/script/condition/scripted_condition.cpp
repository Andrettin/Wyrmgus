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

#include "script/condition/scripted_condition.h"

#include "script/condition/and_condition.h"

namespace wyrmgus {

scripted_condition::scripted_condition(const std::string &identifier) : data_entry(identifier)
{
	this->conditions = std::make_unique<and_condition>();
}

scripted_condition::~scripted_condition()
{
}

void scripted_condition::process_sml_property(const sml_property &property)
{
	this->conditions->process_sml_property(property);
}

void scripted_condition::process_sml_scope(const sml_data &scope)
{
	this->conditions->process_sml_scope(scope);
}

void scripted_condition::check() const
{
	this->get_conditions()->check_validity();
}

}
