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

#include "script/trigger_random_group.h"

#include "script/trigger.h"
#include "script/trigger_type.h"
#include "util/assert_util.h"

namespace wyrmgus {

trigger_random_group::trigger_random_group(const std::string &identifier)
	: named_data_entry(identifier), type(trigger_type::default_trigger)
{
}

void trigger_random_group::initialize()
{
	trigger_random_group::groups_by_type[this->get_type()].push_back(this);

	data_entry::initialize();
}

void trigger_random_group::check() const
{
	assert_throw(this->get_type() != trigger_type::default_trigger);
}

void trigger_random_group::generate_random_offset()
{
	this->random_offset = trigger::generate_random_offset_for_type(this->get_type());
}

}
