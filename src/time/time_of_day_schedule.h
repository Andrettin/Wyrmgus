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

#ifndef __TIME_OF_DAY_SCHEDULE_H__
#define __TIME_OF_DAY_SCHEDULE_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "data_type.h"
#include "time/time_period_schedule.h"

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
	void ProcessConfigData(const CConfigData *config_data);
	int GetHours(const CSeason *season = nullptr) const;
	
	unsigned ID = 0;							/// the scheduled time of day's ID within the time of day schedule
	CTimeOfDay *TimeOfDay = nullptr;			/// the time of day that is scheduled
private:
	int Hours = 0;								/// the amount of hours the scheduled time of day lasts
public:
	CTimeOfDaySchedule *Schedule = nullptr;		/// the schedule to which this time of day belongs
	std::map<const CSeason *, int> SeasonHours;	/// the amount of hours the scheduled time of day lasts in a given season
};

class CTimeOfDaySchedule : public CTimePeriodSchedule
{
	GDCLASS(CTimeOfDaySchedule, CTimePeriodSchedule)
	DATA_TYPE_CLASS(CTimeOfDaySchedule)
	
public:
	~CTimeOfDaySchedule();
	
	static CTimeOfDaySchedule *DefaultTimeOfDaySchedule;
	
	virtual bool ProcessConfigDataProperty(const std::string &key, std::string value) override;
	virtual bool ProcessConfigDataSection(const CConfigData *section) override;
	virtual void Initialize() override;
	
	/**
	**	@brief	Get the default total hours for a time of day schedule
	**
	**	@return	The default total hours
	*/
	virtual unsigned long GetDefaultTotalHours() const
	{
		return DEFAULT_HOURS_PER_DAY;
	}
	
	/**
	**	@brief	Get the default hour multiplier for a time of day schedule
	**
	**	@return	The default hour multiplier
	*/
	virtual int GetDefaultHourMultiplier() const
	{
		return 1;
	}

	std::string Name;										/// Name of the time of day schedules
	std::vector<CScheduledTimeOfDay *> ScheduledTimesOfDay;	/// The times of day that are scheduled

protected:
	static inline void _bind_methods() {}
};

#endif
