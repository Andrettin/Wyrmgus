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

#include "time/time_of_day_schedule.h"

#include "config.h"
#include "time/season.h"
#include "time/time_of_day.h"
#include "util/string_conversion_util.h"

namespace wyrmgus {

time_of_day_schedule *time_of_day_schedule::DefaultTimeOfDaySchedule = nullptr;

time_of_day_schedule::time_of_day_schedule(const std::string &identifier) : time_period_schedule(identifier)
{
}

time_of_day_schedule::~time_of_day_schedule()
{
	for (size_t i = 0; i < this->ScheduledTimesOfDay.size(); ++i) {
		delete this->ScheduledTimesOfDay[i];
	}
}

/**
**	@brief	Process data provided by a configuration file
**
**	@param	config_data	The configuration data
*/
void time_of_day_schedule::ProcessConfigData(const CConfigData *config_data)
{
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "name") {
			this->set_name(value);
		} else if (key == "default_schedule") {
			const bool is_default_schedule = wyrmgus::string::to_bool(value);
			if (is_default_schedule) {
				time_of_day_schedule::DefaultTimeOfDaySchedule = this;
			}
		} else {
			fprintf(stderr, "Invalid time of day schedule property: \"%s\".\n", key.c_str());
		}
	}
	
	for (const CConfigData *child_config_data : config_data->Children) {
		if (child_config_data->Tag == "scheduled_time_of_day") {
			scheduled_time_of_day *scheduled_time_of_day = new wyrmgus::scheduled_time_of_day;
			scheduled_time_of_day->ID = this->ScheduledTimesOfDay.size();
			scheduled_time_of_day->Schedule = this;
			this->ScheduledTimesOfDay.push_back(scheduled_time_of_day);
			
			scheduled_time_of_day->ProcessConfigData(child_config_data);
		} else {
			fprintf(stderr, "Invalid time of day schedule property: \"%s\".\n", child_config_data->Tag.c_str());
		}
	}
	
	this->CalculateHourMultiplier();
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

/**
**	@brief	Process data provided by a configuration file
**
**	@param	config_data	The configuration data
*/
void scheduled_time_of_day::ProcessConfigData(const CConfigData *config_data)
{
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "time_of_day") {
			this->TimeOfDay = wyrmgus::time_of_day::get(value);
		} else if (key == "hours") {
			this->Schedule->TotalHours -= this->Hours; //remove old amount of hours from the schedule, if it has already been defined before
			this->Hours = std::stoi(value);
			this->Schedule->TotalHours += this->Hours;
		} else {
			fprintf(stderr, "Invalid scheduled time of day property: \"%s\".\n", key.c_str());
		}
	}
	
	for (const CConfigData *child_config_data : config_data->Children) {
		if (child_config_data->Tag == "season_hours") {
			wyrmgus::season *season = nullptr;
			int season_hours = 0;
			
			for (size_t j = 0; j < child_config_data->Properties.size(); ++j) {
				std::string key = child_config_data->Properties[j].first;
				std::string value = child_config_data->Properties[j].second;
				
				if (key == "season") {
					season = wyrmgus::season::get(value);
				} else if (key == "hours") {
					season_hours = std::stoi(value);
				} else {
					fprintf(stderr, "Invalid season hours for scheduled time of day property: \"%s\".\n", key.c_str());
				}
			}
			
			if (!season) {
				fprintf(stderr, "Season hours for scheduled time of day has no valid season.\n");
				continue;
			}
			
			if (season_hours <= 0) {
				fprintf(stderr, "Season hours for scheduled time of day has no valid amount of hours.\n");
				continue;
			}
			
			this->SeasonHours[season] = season_hours;
		} else {
			fprintf(stderr, "Invalid scheduled time of day property: \"%s\".\n", child_config_data->Tag.c_str());
		}
	}
	
	if (!this->TimeOfDay) {
		fprintf(stderr, "Scheduled time of day has no valid time of day.\n");
	}
	
	if (this->Hours <= 0) {
		fprintf(stderr, "Scheduled time of day has no valid amount of hours.\n");
	}
}

/**
**	@brief	Get the amount of hours the scheduled time of day lasts
**
**	@param	season	Optional parameter; if given, gets the amount of hours for the given season, if defined
**
**	@return	The amount of hours
*/
int scheduled_time_of_day::GetHours(const wyrmgus::season *season) const
{
	if (season) {
		auto find_iterator = this->SeasonHours.find(season);
		
		if (find_iterator != this->SeasonHours.end()) {
			return find_iterator->second;
		}
	}
	
	return this->Hours;
}

}
