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

#include "database/data_type.h"
#include "time/time_period_schedule.h"

namespace wyrmgus {

class season;
class season_schedule;

class scheduled_season final
{
public:
	unsigned ID = 0;						/// the scheduled season's ID within the season schedule
	wyrmgus::season *Season = nullptr;				/// the season that is scheduled
	unsigned Hours = 0;						/// the amount of hours the scheduled season lasts
	season_schedule *Schedule = nullptr;	/// the schedule to which this season belongs
};

class season_schedule final : public time_period_schedule, public data_type<season_schedule>
{
	Q_OBJECT

public:
	static constexpr const char *class_identifier = "season_schedule";
	static constexpr const char *database_folder = "season_schedules";

	explicit season_schedule(const std::string &identifier);
	~season_schedule();


	virtual void initialize() override;

	virtual void ProcessConfigData(const CConfigData *config_data) override;
	virtual unsigned long GetDefaultTotalHours() const;
	virtual int GetDefaultHourMultiplier() const;

	unsigned HoursPerDay = DEFAULT_HOURS_PER_DAY;		/// The hours per each day for this season schedule
	std::vector<scheduled_season *> ScheduledSeasons;	/// The seasons that are scheduled
};

}
