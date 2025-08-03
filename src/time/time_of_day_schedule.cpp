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

#include "time/time_of_day_schedule.h"

#include "time/season.h"
#include "time/time_of_day.h"

namespace wyrmgus {

time_of_day_schedule::time_of_day_schedule(const std::string &identifier) : time_period_schedule(identifier)
{
}

time_of_day_schedule::~time_of_day_schedule()
{
}

void time_of_day_schedule::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "scheduled_times_of_day") {
		scope.for_each_child([&](const gsml_data &child_scope) {
			const std::string &child_tag = child_scope.get_tag();
			const time_of_day *time_of_day = time_of_day::get(child_tag);
			const size_t index = this->scheduled_times_of_day.size();
			auto scheduled_season = std::make_unique<wyrmgus::scheduled_time_of_day>(index, time_of_day, this);
			child_scope.process(scheduled_season.get());
			this->scheduled_times_of_day.push_back(std::move(scheduled_season));
		});
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void time_of_day_schedule::initialize()
{
	unsigned long total_hours = 0;
	for (const std::unique_ptr<scheduled_time_of_day> &time_of_day : this->get_scheduled_times_of_day()) {
		total_hours += time_of_day->get_hours(nullptr);
	}
	this->set_total_hours(total_hours);

	time_period_schedule::initialize();
}

void time_of_day_schedule::check() const
{
	for (const std::unique_ptr<scheduled_time_of_day> &time_of_day : this->get_scheduled_times_of_day()) {
		time_of_day->check();
	}
}

/**
**	@brief	Get the default total hours for a time of day schedule
**
**	@return	The default total hours
*/
unsigned long time_of_day_schedule::GetDefaultTotalHours() const
{
	return DEFAULT_HOURS_PER_DAY;
}

/**
**	@brief	Get the default hour multiplier for a time of day schedule
**
**	@return	The default hour multiplier
*/
int time_of_day_schedule::GetDefaultHourMultiplier() const
{
	return 1;
}

void scheduled_time_of_day::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "season_hours") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			const season *season = season::get(key);
			const unsigned hours = std::stoul(value);

			if (season == nullptr) {
				throw std::runtime_error("Season hours for scheduled time of day has no valid season.");
			}

			if (hours == 0) {
				throw std::runtime_error("Season hours for scheduled time of day has no valid amount of hours.");
			}

			this->season_hours[season] = hours;
		});
	} else {
		scheduled_time_period::process_gsml_scope(scope);
	}
}

/**
**	@brief	Get the amount of hours the scheduled time of day lasts
**
**	@param	season	Optional parameter; if given, gets the amount of hours for the given season, if defined
**
**	@return	The amount of hours
*/
unsigned scheduled_time_of_day::get_hours(const wyrmgus::season *season) const
{
	if (season != nullptr) {
		const auto find_iterator = this->season_hours.find(season);
		
		if (find_iterator != this->season_hours.end()) {
			return find_iterator->second;
		}
	}
	
	return scheduled_time_period::get_hours();
}

}
