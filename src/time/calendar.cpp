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
//      (c) Copyright 2018-2022 by Andrettin
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

#include "time/calendar.h"

#include "config.h"
#include "time/day_of_the_week.h"
#include "time/month.h"

namespace wyrmgus {

calendar::calendar(const std::string &identifier) : named_data_entry(identifier)
{
}

calendar::~calendar()
{
}

void calendar::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "day_of_the_week_names") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			this->day_of_the_week_names[string_to_day_of_the_week(key)] = value;
		});
	} else if (tag == "month_names") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			this->month_names[string_to_month(key)] = value;
		});
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void calendar::initialize()
{
	if (this->base_calendar != nullptr) {
		if (this->base_calendar == this) {
			throw std::runtime_error("The base calendar of \"" + this->get_identifier() + "\" is itself.");
		}

		if (this->base_calendar->is_any_base_calendar(this)) {
			throw std::runtime_error("The base calendar of \"" + this->get_identifier() + "\" is \"" + this->base_calendar->get_identifier() + "\", which creates a circular dependency.");
		}

		if (!this->base_calendar->is_initialized()) {
			this->base_calendar->initialize();
		}

		//calculate the year offset to the Gregorian calendar based on the offset to the base calendar
		this->year_offset += this->base_calendar->get_year_offset();
		this->base_calendar = nullptr;
	}

	data_entry::initialize();
}

}