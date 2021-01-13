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
//

#pragma once

#include "data_type.h"
#include "time/time_period_schedule.h"

class CConfigData;
class CTimeOfDaySchedule;

namespace wyrmgus {
	class season;
	class time_of_day;
}

class CScheduledTimeOfDay
{
public:
	void ProcessConfigData(const CConfigData *config_data);
	int GetHours(const wyrmgus::season *season = nullptr) const;
	
	unsigned ID = 0;							/// the scheduled time of day's ID within the time of day schedule
	wyrmgus::time_of_day *TimeOfDay = nullptr;	/// the time of day that is scheduled
private:
	int Hours = 0;								/// the amount of hours the scheduled time of day lasts
public:
	CTimeOfDaySchedule *Schedule = nullptr;		/// the schedule to which this time of day belongs
	std::map<const wyrmgus::season *, int> SeasonHours;	/// the amount of hours the scheduled time of day lasts in a given season
};

class CTimeOfDaySchedule : public CTimePeriodSchedule
{
public:
	~CTimeOfDaySchedule();
	
	static CTimeOfDaySchedule *GetTimeOfDaySchedule(const std::string &ident, const bool should_find = true);
	static CTimeOfDaySchedule *GetOrAddTimeOfDaySchedule(const std::string &ident);
	static void ClearTimeOfDaySchedules();
	
	static std::vector<CTimeOfDaySchedule *> TimeOfDaySchedules;		/// Time of day schedules
	static std::map<std::string, CTimeOfDaySchedule *> TimeOfDaySchedulesByIdent;
	static CTimeOfDaySchedule *DefaultTimeOfDaySchedule;
	
	virtual void ProcessConfigData(const CConfigData *config_data) override;
	virtual unsigned long GetDefaultTotalHours() const;
	virtual int GetDefaultHourMultiplier() const;

	std::string Name;										/// Name of the time of day schedules
	std::vector<CScheduledTimeOfDay *> ScheduledTimesOfDay;	/// The times of day that are scheduled
};
