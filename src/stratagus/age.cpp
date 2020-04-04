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
/**@name age.cpp - The age source file. */
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

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "age.h"

#include "config.h"
#include "game.h"
#include "mod.h"
#include "player.h"
#include "time/calendar.h"
#include "unit/unittype.h"
#include "upgrade/dependency.h"
#include "upgrade/upgrade_structs.h"
#include "video.h"

CAge *CAge::CurrentAge = nullptr;

void CAge::initialize_all()
{
	data_type::initialize_all();

	CAge::sort_instances([](CAge *a, CAge *b) {
		if (a->priority != b->priority) {
			return a->priority > b->priority;
		} else {
			return a->get_identifier() < b->get_identifier();
		}
	});
}

void CAge::SetCurrentAge(CAge *age)
{
	if (age == CAge::CurrentAge) {
		return;
	}

	CAge::CurrentAge = age;

	if (GameCycle > 0 && !SaveGameLoading) {
		if (CAge::CurrentAge && CAge::CurrentAge->year_boost > 0) {
			//boost the current date's year by a certain amount; this is done so that we can both have a slower passage of years in-game (for seasons and character lifetimes to be more sensible), while still not having overly old dates in later ages
			for (CCalendar *calendar : CCalendar::Calendars) {
				calendar->CurrentDate.AddHours(calendar, (long long) CAge::CurrentAge->year_boost * DEFAULT_DAYS_PER_YEAR * DEFAULT_HOURS_PER_DAY);
			}
		}
	}
}

//check which age fits the current overall situation best, out of the ages each player is in
void CAge::CheckCurrentAge()
{
	CAge *best_age = CAge::CurrentAge;

	for (int p = 0; p < PlayerMax; ++p) {
		if (CPlayer::Players[p]->Age && (!best_age || CPlayer::Players[p]->Age->priority > best_age->priority)) {
			best_age = CPlayer::Players[p]->Age;
		}
	}

	if (best_age != CAge::CurrentAge) {
		CAge::SetCurrentAge(best_age);
	}
}

CAge::~CAge()
{
	if (this->graphics) {
		CGraphic::Free(this->graphics);
	}
}

void CAge::process_sml_scope(const stratagus::sml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "image") {
		std::string file;
		Vec2i size(0, 0);

		scope.for_each_property([&](const stratagus::sml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();
			if (key == "file") {
				file = CMod::GetCurrentModPath() + value;
			} else if (key == "width") {
				size.x = std::stoi(value);
			} else if (key == "height") {
				size.y = std::stoi(value);
			} else {
				throw std::runtime_error("Invalid image property: \"" + key + "\".");
			}
		});

		if (file.empty()) {
			throw std::runtime_error("Image has no file.");
		}

		if (size.x == 0) {
			throw std::runtime_error("Image has no width.");
		}

		if (size.y == 0) {
			throw std::runtime_error("Image has no height.");
		}

		this->graphics = CGraphic::New(file, size.x, size.y);
		this->graphics->Load();
		this->graphics->UseDisplayFormat();
	} else if (tag == "predependencies") {
		this->predependency = new CAndDependency;
		stratagus::database::process_sml_data(this->predependency, scope);
	} else if (tag == "dependencies") {
		this->dependency = new CAndDependency;
		stratagus::database::process_sml_data(this->dependency, scope);
	} else {
		data_entry::process_sml_scope(scope);
	}
}

/**
**	@brief	Set the current overall in-game age
**
**	@param	age_ident	The age's string identifier
*/
void SetCurrentAge(const std::string &age_ident)
{
	CAge *age = CAge::get(age_ident);
	
	if (!age) {
		return;
	}
	
	CAge::CurrentAge = age;
}
