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

#ifndef __SEASON_SCHEDULE_H__
#define __SEASON_SCHEDULE_H__

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
class CSeasonSchedule;

class CScheduledSeason
{
public:
	CScheduledSeason() :
		ID(0), Season(nullptr), Hours(0), Schedule(nullptr)
	{
	}
	
	unsigned ID;				/// the scheduled season's ID within the season schedule
	CSeason *Season;			/// the season that is scheduled
	unsigned Hours;				/// the amount of hours the scheduled season lasts
	CSeasonSchedule *Schedule;	/// the schedule to which this season belongs
};

class CSeasonSchedule
{
public:
	CSeasonSchedule() :
		TotalHours(0), HoursPerDay(DefaultHoursPerDay)
	{
	}
	
	~CSeasonSchedule();
	
	static CSeasonSchedule *GetSeasonSchedule(const std::string &ident, const bool should_find = true);
	static CSeasonSchedule *GetOrAddSeasonSchedule(const std::string &ident);
	static void ClearSeasonSchedules();
	
	static std::vector<CSeasonSchedule *> SeasonSchedules;		/// Season schedules
	static std::map<std::string, CSeasonSchedule *> SeasonSchedulesByIdent;
	static CSeasonSchedule *DefaultSeasonSchedule;
	
	void ProcessConfigData(const CConfigData *config_data);

	std::string Ident;									/// Ident of the season schedules
	std::string Name;									/// Name of the season schedules
	unsigned TotalHours;								/// The total amount of hours this season schedule contains
	unsigned HoursPerDay;								/// The hours per each day for this season schedule
	std::vector<CScheduledSeason *> ScheduledSeasons;	/// The seasons that are scheduled
};

//@}

#endif // !__SEASON_SCHEDULE_H__
