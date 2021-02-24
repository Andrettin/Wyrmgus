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
//      (c) Copyright 2018-2021 by Andrettin
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

#include "age.h"

#include "config.h"
#include "database/defines.h"
#include "game.h"
#include "mod.h"
#include "player.h"
#include "script/condition/and_condition.h"
#include "time/calendar.h"
#include "unit/unit_type.h"
#include "upgrade/upgrade_structs.h"
#include "video/video.h"

namespace wyrmgus {

const age *age::current_age = nullptr;

void age::initialize_all()
{
	data_type::initialize_all();

	age::sort_instances([](const age *a, const age *b) {
		if (a->priority != b->priority) {
			return a->priority > b->priority;
		} else {
			return a->get_identifier() < b->get_identifier();
		}
	});
}

void age::set_current_age(const age *age)
{
	if (age == age::current_age) {
		return;
	}

	age::current_age = age;
}

//check which age fits the current overall situation best, out of the ages each player is in
void age::check_current_age()
{
	const age *best_age = age::current_age;

	for (int p = 0; p < PlayerMax; ++p) {
		const age *age = CPlayer::Players[p]->get_age();

		if (age == nullptr) {
			continue;
		}

		if (!best_age || age->priority > best_age->priority) {
			best_age = age;
		}
	}

	if (best_age != age::current_age) {
		age::set_current_age(best_age);
	}
}

age::age(const std::string &identifier) : named_data_entry(identifier)
{
}

age::~age()
{
}

void age::process_sml_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "preconditions") {
		this->preconditions = std::make_unique<and_condition>();
		database::process_sml_data(this->preconditions, scope);
	} else if (tag == "conditions") {
		this->conditions = std::make_unique<and_condition>();
		database::process_sml_data(this->conditions, scope);
	} else {
		data_entry::process_sml_scope(scope);
	}
}

void age::check() const
{
	if (this->get_icon() == nullptr) {
		throw std::runtime_error("Age \"" + this->get_identifier() + "\" has no icon.");
	}

	if (this->get_preconditions() != nullptr) {
		this->get_preconditions()->check_validity();
	}

	if (this->get_conditions() != nullptr) {
		this->get_conditions()->check_validity();
	}
}


}

/**
**	@brief	Set the current overall in-game age
**
**	@param	age_ident	The age's string identifier
*/
void SetCurrentAge(const std::string &age_ident)
{
	wyrmgus::age *age = wyrmgus::age::get(age_ident);
	
	if (!age) {
		return;
	}
	
	wyrmgus::age::current_age = age;
}
