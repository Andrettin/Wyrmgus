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
/**@name time_of_day_schedule.cpp - The time of day schedule source file. */
//
//      (c) Copyright 2018 by Andrettin
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

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "time_of_day_schedule.h"

#include "config.h"
#include "time_of_day.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

std::vector<CTimeOfDaySchedule *> CTimeOfDaySchedule::TimeOfDaySchedules;
std::map<std::string, CTimeOfDaySchedule *> CTimeOfDaySchedule::TimeOfDaySchedulesByIdent;
	
/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Get a time of day schedule
**
**	@param	ident		The time of day schedule's string identifier
**	@param	should_find	Whether it is an error if the time of day schedule could not be found; this is true by default
**
**	@return	The time of day schedule if found, or null otherwise
*/
CTimeOfDaySchedule *CTimeOfDaySchedule::GetTimeOfDaySchedule(const std::string &ident, const bool should_find)
{
	std::map<std::string, CTimeOfDaySchedule *>::const_iterator find_iterator = TimeOfDaySchedulesByIdent.find(ident);
	
	if (find_iterator != TimeOfDaySchedulesByIdent.end()) {
		return find_iterator->second;
	}
	
	if (should_find) {
		fprintf(stderr, "Invalid time of day schedule: \"%s\".\n", ident.c_str());
	}
	
	return nullptr;
}

/**
**	@brief	Get or add a time of day schedule
**
**	@param	ident	The time of day schedule's string identifier
**
**	@return	The time of day schedule if found, or a newly-created one otherwise
*/
CTimeOfDaySchedule *CTimeOfDaySchedule::GetOrAddTimeOfDaySchedule(const std::string &ident)
{
	CTimeOfDaySchedule *time_of_day_schedule = GetTimeOfDaySchedule(ident, false);
	
	if (!time_of_day_schedule) {
		time_of_day_schedule = new CTimeOfDaySchedule;
		time_of_day_schedule->Ident = ident;
		TimeOfDaySchedules.push_back(time_of_day_schedule);
		TimeOfDaySchedulesByIdent[ident] = time_of_day_schedule;
	}
	
	return time_of_day_schedule;
}

/**
**	@brief	Remove the existing time of day schedules
*/
void CTimeOfDaySchedule::ClearTimeOfDaySchedules()
{
	for (size_t i = 0; i < TimeOfDaySchedules.size(); ++i) {
		delete TimeOfDaySchedules[i];
	}
	TimeOfDaySchedules.clear();
}

/**
**	@brief	Destructor
*/
CTimeOfDaySchedule::~CTimeOfDaySchedule()
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
void CTimeOfDaySchedule::ProcessConfigData(const CConfigData *config_data)
{
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "name") {
			this->Name = value;
		} else {
			fprintf(stderr, "Invalid time of day schedule property: \"%s\".\n", key.c_str());
		}
	}
	
	
	for (size_t i = 0; i < config_data->Children.size(); ++i) {
		CConfigData *child_config_data = config_data->Children[i];
		
		if (child_config_data->Tag == "scheduled_time_of_day") {
			CTimeOfDay *time_of_day = nullptr;
			int hours = 0;
				
			for (size_t j = 0; j < child_config_data->Properties.size(); ++j) {
				std::string key = child_config_data->Properties[j].first;
				std::string value = child_config_data->Properties[j].second;
				
				if (key == "scheduled_time_of_day") {
					value = FindAndReplaceString(value, "_", "-");
					time_of_day = CTimeOfDay::GetTimeOfDay(value);
				} else if (key == "hours") {
					hours = std::stoi(value);
				} else {
					fprintf(stderr, "Invalid scheduled time of day property: \"%s\".\n", key.c_str());
				}
			}
			
			if (!time_of_day) {
				fprintf(stderr, "Scheduled time of day has no time of day.\n");
				continue;
			}
			
			if (hours <= 0) {
				fprintf(stderr, "Scheduled time of day has no hours defined.\n");
				continue;
			}
			
			CScheduledTimeOfDay *scheduled_time_of_day = new CScheduledTimeOfDay;
			scheduled_time_of_day->TimeOfDay = time_of_day;
			scheduled_time_of_day->Hours = hours;
			
			this->ScheduledTimesOfDay.push_back(scheduled_time_of_day);
		} else {
			fprintf(stderr, "Invalid time of day schedule property: \"%s\".\n", child_config_data->Tag.c_str());
		}
	}
}

//@}
