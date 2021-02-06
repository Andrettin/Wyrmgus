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

#pragma once

#include "database/data_type.h"
#include "time/time_period_schedule.h"

class CConfigData;

namespace wyrmgus {

class season;
class time_of_day;
class time_of_day_schedule;

class scheduled_time_of_day final
{
public:
	void ProcessConfigData(const CConfigData *config_data);
	int GetHours(const wyrmgus::season *season = nullptr) const;
	
	unsigned ID = 0;							/// the scheduled time of day's ID within the time of day schedule
	wyrmgus::time_of_day *TimeOfDay = nullptr;	/// the time of day that is scheduled
private:
	int Hours = 0;								/// the amount of hours the scheduled time of day lasts
public:
	time_of_day_schedule *Schedule = nullptr;		/// the schedule to which this time of day belongs
	std::map<const wyrmgus::season *, int> SeasonHours;	/// the amount of hours the scheduled time of day lasts in a given season
};

class time_of_day_schedule final : public time_period_schedule, public data_type<time_of_day_schedule>
{
	Q_OBJECT

public:
	static constexpr const char *class_identifier = "time_of_day_schedule";
	static constexpr const char *database_folder = "time_of_day_schedules";

	static time_of_day_schedule *DefaultTimeOfDaySchedule;

	explicit time_of_day_schedule(const std::string &identifier);
	~time_of_day_schedule();

	virtual void initialize() override;
	
	virtual void ProcessConfigData(const CConfigData *config_data) override;
	virtual unsigned long GetDefaultTotalHours() const;
	virtual int GetDefaultHourMultiplier() const;

	std::vector<scheduled_time_of_day *> ScheduledTimesOfDay;	/// The times of day that are scheduled
};

}
