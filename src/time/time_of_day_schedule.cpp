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
//      (c) Copyright 2018-2019 by Andrettin
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

#include "time/time_of_day_schedule.h"

#include "config.h"
#include "config_operator.h"
#include "time/season.h"
#include "time/time_of_day.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

CTimeOfDaySchedule *CTimeOfDaySchedule::DefaultTimeOfDaySchedule = nullptr;
	
/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Destructor
*/
CTimeOfDaySchedule::~CTimeOfDaySchedule()
{
	for (CScheduledTimeOfDay *scheduled_time_of_day : this->ScheduledTimesOfDay) {
		delete scheduled_time_of_day;
	}
}

/**
**	@brief	Process a property in the data provided by a configuration file
**
**	@param	key		The property's key
**	@param	value	The property's value
**
**	@return	True if the property can be processed, or false otherwise
*/
bool CTimeOfDaySchedule::ProcessConfigDataProperty(const String &key, String value)
{
	if (key == "default_schedule") {
		const bool is_default_schedule = StringToBool(value);
		if (is_default_schedule) {
			CTimeOfDaySchedule::DefaultTimeOfDaySchedule = this;
		}
	} else {
		return false;
	}
	
	return true;
}

/**
**	@brief	Process a section in the data provided by a configuration file
**
**	@param	section		The section
**
**	@return	True if the section can be processed, or false otherwise
*/
bool CTimeOfDaySchedule::ProcessConfigDataSection(const CConfigData *section)
{
	if (section->Tag == "scheduled_time_of_day") {
		CScheduledTimeOfDay *scheduled_time_of_day = new CScheduledTimeOfDay;
		scheduled_time_of_day->ID = this->ScheduledTimesOfDay.size();
		scheduled_time_of_day->Schedule = this;
		this->ScheduledTimesOfDay.push_back(scheduled_time_of_day);
		
		scheduled_time_of_day->ProcessConfigData(section);
	} else {
		return false;
	}
	
	return true;
}

/**
**	@brief	Initialize the time of day schedule
*/
void CTimeOfDaySchedule::Initialize()
{
	this->CalculateHourMultiplier();
	
	this->Initialized = true;
}

/**
**	@brief	Process data provided by a configuration file
**
**	@param	config_data	The configuration data
*/
void CScheduledTimeOfDay::ProcessConfigData(const CConfigData *config_data)
{
	for (const CConfigProperty &property : config_data->Properties) {
		if (property.Operator != CConfigOperator::Assignment) {
			fprintf(stderr, "Wrong operator enumeration index for property \"%s\": %i.\n", property.Key.utf8().get_data(), property.Operator);
			continue;
		}
		
		String key = property.Key;
		String value = property.Value;
		
		if (key == "time_of_day") {
			this->TimeOfDay = CTimeOfDay::Get(value);
		} else if (key == "hours") {
			this->Schedule->TotalHours -= this->Hours; //remove old amount of hours from the schedule, if it has already been defined before
			this->Hours = value.to_int();
			this->Schedule->TotalHours += this->Hours;
		} else {
			fprintf(stderr, "Invalid scheduled time of day property: \"%s\".\n", key.utf8().get_data());
		}
	}
	
	for (const CConfigData *section : config_data->Sections) {
		if (section->Tag == "season_hours") {
			CSeason *season = nullptr;
			int season_hours = 0;
			
			for (const CConfigProperty &property : section->Properties) {
				if (property.Operator != CConfigOperator::Assignment) {
					fprintf(stderr, "Wrong operator enumeration index for property \"%s\": %i.\n", property.Key.utf8().get_data(), property.Operator);
					continue;
				}
				
				String key = property.Key;
				String value = property.Value;
				
				if (key == "season") {
					season = CSeason::Get(value);
				} else if (key == "hours") {
					season_hours = value.to_int();
				} else {
					fprintf(stderr, "Invalid season hours for scheduled time of day property: \"%s\".\n", key.utf8().get_data());
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
			fprintf(stderr, "Invalid scheduled time of day property: \"%s\".\n", section->Tag.utf8().get_data());
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
int CScheduledTimeOfDay::GetHours(const CSeason *season) const
{
	if (season) {
		std::map<const CSeason *, int>::const_iterator find_iterator = this->SeasonHours.find(season);
		
		if (find_iterator != this->SeasonHours.end()) {
			return find_iterator->second;
		}
	}
	
	return this->Hours;
}
