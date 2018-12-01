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
/**@name time_of_day_schedule.h - The time of day schedule header file. */
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

#ifndef __TIME_OF_DAY_SCHEDULE_H__
#define __TIME_OF_DAY_SCHEDULE_H__

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include <map>
#include <string>
#include <vector>

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CConfigData;
class CSeason;
class CTimeOfDay;
class CTimeOfDaySchedule;

class CScheduledTimeOfDay
{
public:
	CScheduledTimeOfDay() :
		ID(0), TimeOfDay(nullptr), Hours(0), Schedule(nullptr)
	{
	}
	
	void ProcessConfigData(const CConfigData *config_data);
	int GetHours(const CSeason *season = nullptr) const;
	
	unsigned ID;					/// the scheduled time of day's ID within the time of day schedule
	CTimeOfDay *TimeOfDay;			/// the time of day that is scheduled
private:
	int Hours;						/// the amount of hours the scheduled time of day lasts
public:
	CTimeOfDaySchedule *Schedule;	/// the schedule to which this time of day belongs
	std::map<const CSeason *, int> SeasonHours;	/// the amount of hours the scheduled time of day lasts in a given season
};

class CTimeOfDaySchedule
{
public:
	CTimeOfDaySchedule() :
		TotalHours(0)
	{
	}
	
	~CTimeOfDaySchedule();
	
	static CTimeOfDaySchedule *GetTimeOfDaySchedule(const std::string &ident, const bool should_find = true);
	static CTimeOfDaySchedule *GetOrAddTimeOfDaySchedule(const std::string &ident);
	static void ClearTimeOfDaySchedules();
	
	static std::vector<CTimeOfDaySchedule *> TimeOfDaySchedules;		/// Time of day schedules
	static std::map<std::string, CTimeOfDaySchedule *> TimeOfDaySchedulesByIdent;
	static CTimeOfDaySchedule *DefaultTimeOfDaySchedule;
	
	void ProcessConfigData(const CConfigData *config_data);

	std::string Ident;										/// Ident of the time of day schedules
	std::string Name;										/// Name of the time of day schedules
	unsigned TotalHours;									/// The total amount of hours this time of day schedule contains
	std::vector<CScheduledTimeOfDay *> ScheduledTimesOfDay;	/// The times of day that are scheduled
};

//@}

#endif // !__TIME_OF_DAY_SCHEDULE_H__
