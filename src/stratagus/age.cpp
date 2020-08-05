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
//      (c) Copyright 2018-2020 by Andrettin
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
//

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
#include "video.h"

namespace stratagus {

const age *age::current_age = nullptr;

void age::initialize_all()
{
	data_type::initialize_all();

	age::sort_instances([](age *a, age *b) {
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
	if (this->graphics) {
		CGraphic::Free(this->graphics);
	}
}

void age::process_sml_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "image") {
		std::filesystem::path filepath;
		Vec2i size(0, 0);

		scope.for_each_property([&](const sml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();
			if (key == "file") {
				filepath = database::get_graphics_path(this->get_module()) / value;
			} else if (key == "width") {
				size.x = std::stoi(value);
			} else if (key == "height") {
				size.y = std::stoi(value);
			} else {
				throw std::runtime_error("Invalid image property: \"" + key + "\".");
			}
		});

		if (filepath.empty()) {
			throw std::runtime_error("Image has no file.");
		}

		if (size.x == 0) {
			throw std::runtime_error("Image has no width.");
		}

		if (size.y == 0) {
			throw std::runtime_error("Image has no height.");
		}

		this->graphics = CGraphic::New(filepath.string(), size.x, size.y);
	} else if (tag == "preconditions") {
		this->preconditions = std::make_unique<and_condition>();
		database::process_sml_data(this->preconditions, scope);
	} else if (tag == "conditions") {
		this->conditions = std::make_unique<and_condition>();
		database::process_sml_data(this->conditions, scope);
	} else {
		data_entry::process_sml_scope(scope);
	}
}

void age::initialize()
{
	if (this->graphics != nullptr) {
		this->graphics->Load(false, defines::get()->get_scale_factor());
	}

	data_entry::initialize();
}

}

/**
**	@brief	Set the current overall in-game age
**
**	@param	age_ident	The age's string identifier
*/
void SetCurrentAge(const std::string &age_ident)
{
	stratagus::age *age = stratagus::age::get(age_ident);
	
	if (!age) {
		return;
	}
	
	stratagus::age::current_age = age;
}
