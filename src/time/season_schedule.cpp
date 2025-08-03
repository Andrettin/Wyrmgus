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
//      (c) Copyright 2018-2022 by Andrettin
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

#include "stratagus.h"

#include "time/season_schedule.h"

#include "time/season.h"

namespace wyrmgus {

season_schedule::season_schedule(const std::string &identifier) : time_period_schedule(identifier)
{
}

season_schedule::~season_schedule()
{
}

void season_schedule::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "scheduled_seasons") {
		scope.for_each_child([&](const gsml_data &child_scope) {
			const std::string &child_tag = child_scope.get_tag();
			const season *season = season::get(child_tag);
			const size_t index = this->scheduled_seasons.size();
			auto scheduled_season = std::make_unique<wyrmgus::scheduled_season>(index, season, this);
			child_scope.process(scheduled_season.get());
			this->scheduled_seasons.push_back(std::move(scheduled_season));
		});
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void season_schedule::initialize()
{
	unsigned long total_hours = 0;
	for (const std::unique_ptr<scheduled_season> &season : this->get_scheduled_seasons()) {
		total_hours += season->get_hours();
	}
	this->set_total_hours(total_hours);

	time_period_schedule::initialize();
}

void season_schedule::check() const
{
	for (const std::unique_ptr<scheduled_season> &season : this->get_scheduled_seasons()) {
		season->check();
	}
}

/**
**	@brief	Get the default total hours for a season schedule
**
**	@return	The default total hours
*/
unsigned long season_schedule::GetDefaultTotalHours() const
{
	return DEFAULT_DAYS_PER_YEAR * DEFAULT_HOURS_PER_DAY;
}

/**
**	@brief	Get the default hour multiplier for a season schedule
**
**	@return	The default hour multiplier
*/
int season_schedule::GetDefaultHourMultiplier() const
{
	return DEFAULT_DAY_MULTIPLIER_PER_YEAR;
}

}
