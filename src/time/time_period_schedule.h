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

#include "database/named_data_entry.h"

namespace wyrmgus {

class scheduled_time_period
{
public:
	explicit scheduled_time_period(const size_t index) : index(index)
	{
	}

	virtual ~scheduled_time_period()
	{
	}

	virtual void process_sml_property(const sml_property &property);
	virtual void process_sml_scope(const sml_data &scope);

	virtual void check() const
	{
		if (this->get_hours() == 0) {
			throw std::runtime_error("Scheduled time period has no amount of time defined.");
		}
	}

	size_t get_index() const
	{
		return this->index;
	}

	unsigned get_hours() const
	{
		return this->hours;
	}

private:
	size_t index = 0;
	unsigned hours = 0; //the amount of hours the scheduled time period lasts
};

class time_period_schedule : public named_data_entry
{
public:
	explicit time_period_schedule(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void initialize() override;

	virtual unsigned long GetDefaultTotalHours() const = 0;
	virtual int GetDefaultHourMultiplier() const = 0;
	void CalculateHourMultiplier();

	unsigned long get_total_hours() const
	{
		return this->total_hours;
	}

protected:
	void set_total_hours(const unsigned long hours)
	{
		this->total_hours = hours;
	}

private:
	unsigned long total_hours = 0; //the total amount of hours this time period schedule contains
public:
	int HourMultiplier = 1; //the amount of hours that pass for this schedule for each in-game hour
};

}
