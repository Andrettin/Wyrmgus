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

class scheduled_time_of_day final : public scheduled_time_period
{
public:
	explicit scheduled_time_of_day(const size_t index, const wyrmgus::time_of_day *time_of_day, const time_of_day_schedule *schedule)
		: scheduled_time_period(index), time_of_day(time_of_day), schedule(schedule)
	{
	}

	virtual void process_sml_scope(const sml_data &scope) override;

	virtual void check() const override
	{
		if (this->get_time_of_day() == nullptr) {
			throw std::runtime_error("Scheduled time of day has no time of day.");
		}

		scheduled_time_period::check();
	}

	const wyrmgus::time_of_day *get_time_of_day() const
	{
		return this->time_of_day;
	}

	unsigned get_hours(const wyrmgus::season *season) const;

private:
	const wyrmgus::time_of_day *time_of_day = nullptr;
	const time_of_day_schedule *schedule = nullptr;
	std::map<const wyrmgus::season *, unsigned> season_hours; //the amount of hours the scheduled time of day lasts in a given season
};

class time_of_day_schedule final : public time_period_schedule, public data_type<time_of_day_schedule>
{
	Q_OBJECT

public:
	static constexpr const char *class_identifier = "time_of_day_schedule";
	static constexpr const char *database_folder = "time_of_day_schedules";

	explicit time_of_day_schedule(const std::string &identifier);
	~time_of_day_schedule();

	virtual void process_sml_scope(const sml_data &scope) override;
	virtual void initialize() override;
	virtual void check() const override;

	virtual unsigned long GetDefaultTotalHours() const override;
	virtual int GetDefaultHourMultiplier() const override;

	const std::vector<std::unique_ptr<scheduled_time_of_day>> &get_scheduled_times_of_day() const
	{
		return this->scheduled_times_of_day;
	}

private:
	std::vector<std::unique_ptr<scheduled_time_of_day>> scheduled_times_of_day;
};

}
