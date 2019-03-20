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
/**@name season_schedule.h - The season schedule header file. */
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

#ifndef __SEASON_SCHEDULE_H__
#define __SEASON_SCHEDULE_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "time/time_period_schedule.h"

#include <map>
#include <string>
#include <vector>

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CSeason;
class CSeasonSchedule;

class CScheduledSeason
{
public:
	unsigned ID = 0;						/// the scheduled season's ID within the season schedule
	CSeason *Season = nullptr;				/// the season that is scheduled
	unsigned Hours = 0;						/// the amount of hours the scheduled season lasts
	CSeasonSchedule *Schedule = nullptr;	/// the schedule to which this season belongs
};

class CSeasonSchedule : public CTimePeriodSchedule
{
	DATA_TYPE_CLASS(CSeasonSchedule)
	
public:
	~CSeasonSchedule();
	
	static CSeasonSchedule *DefaultSeasonSchedule;
	
	virtual bool ProcessConfigDataProperty(const std::string &key, std::string value) override;
	virtual bool ProcessConfigDataSection(const CConfigData *section) override;
	virtual void Initialize() override;
	
	virtual unsigned long GetDefaultTotalHours() const;
	virtual int GetDefaultHourMultiplier() const;

	std::string Name;									/// Name of the season schedule
	unsigned HoursPerDay = DEFAULT_HOURS_PER_DAY;		/// The hours per each day for this season schedule
	std::vector<CScheduledSeason *> ScheduledSeasons;	/// The seasons that are scheduled
};

#endif
