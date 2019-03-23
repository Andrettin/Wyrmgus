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
/**@name season_schedule.cpp - The season schedule source file. */
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

#include "time/season_schedule.h"

#include "config.h"
#include "config_operator.h"
#include "time/season.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

CSeasonSchedule *CSeasonSchedule::DefaultSeasonSchedule = nullptr;
	
/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Destructor
*/
CSeasonSchedule::~CSeasonSchedule()
{
	for (CScheduledSeason *scheduled_season : this->ScheduledSeasons) {
		delete scheduled_season;
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
bool CSeasonSchedule::ProcessConfigDataProperty(const std::string &key, std::string value)
{
	if (key == "name") {
		this->Name = value;
	} else if (key == "default_schedule") {
		const bool is_default_schedule = StringToBool(value);
		if (is_default_schedule) {
			CSeasonSchedule::DefaultSeasonSchedule = this;
		}
	} else if (key == "hours_per_day") {
		this->HoursPerDay = std::stoi(value);
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
bool CSeasonSchedule::ProcessConfigDataSection(const CConfigData *section)
{
	if (section->Tag == "scheduled_season") {
		CSeason *season = nullptr;
		unsigned hours = 0;
		
		for (const CConfigProperty &property : section->Properties) {
			if (property.Operator != CConfigOperator::Assignment) {
				fprintf(stderr, "Wrong operator enumeration index for property \"%s\": %i.\n", property.Key.c_str(), property.Operator);
				continue;
			}
			
			std::string key = property.Key;
			std::string value = property.Value;
			
			if (key == "season") {
				value = FindAndReplaceString(value, "_", "-");
				season = CSeason::Get(value);
			} else if (key == "days") {
				hours = std::stoi(value) * this->HoursPerDay;
			} else if (key == "hours") {
				hours = std::stoi(value);
			} else {
				fprintf(stderr, "Invalid scheduled season property: \"%s\".\n", key.c_str());
			}
		}
		
		if (!season) {
			fprintf(stderr, "Scheduled season has no season.\n");
			return true;
		}
		
		if (hours <= 0) {
			fprintf(stderr, "Scheduled season has no amount of time defined.\n");
			return true;
		}
		
		CScheduledSeason *scheduled_season = new CScheduledSeason;
		scheduled_season->Season = season;
		scheduled_season->Hours = hours;
		scheduled_season->ID = this->ScheduledSeasons.size();
		scheduled_season->Schedule = this;
		this->ScheduledSeasons.push_back(scheduled_season);
		this->TotalHours += scheduled_season->Hours;
	} else {
		return false;
	}
	
	return true;
}
	
/**
**	@brief	Initialize the season schedule
*/
void CSeasonSchedule::Initialize()
{
	this->CalculateHourMultiplier();
	
	this->Initialized = true;
}

/**
**	@brief	Get the default total hours for a season schedule
**
**	@return	The default total hours
*/
unsigned long CSeasonSchedule::GetDefaultTotalHours() const
{
	return DEFAULT_DAYS_PER_YEAR * DEFAULT_HOURS_PER_DAY;
}

/**
**	@brief	Get the default hour multiplier for a season schedule
**
**	@return	The default hour multiplier
*/
int CSeasonSchedule::GetDefaultHourMultiplier() const
{
	return DEFAULT_DAY_MULTIPLIER_PER_YEAR;
}
