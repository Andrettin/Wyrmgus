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
//      (c) Copyright 2021-2022 by Andrettin
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

#include "network/network.h"
#include "script/condition/condition.h"
#include "time/day_of_the_week.h"
#include "util/locale_util.h"

namespace wyrmgus {

template <typename scope_type>
class real_day_of_the_week_condition final : public condition<scope_type>
{
public:
	explicit real_day_of_the_week_condition(const std::string &value)
	{
		this->day_of_the_week = string_to_day_of_the_week(value);
	}

	virtual bool check(const scope_type *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(scope);
		Q_UNUSED(ctx);

		if (IsNetworkGame()) {
			//always false in multiplayer games, to prevent desyncs if the real day of the week changes during a game
			return false;
		}

		const QDateTime current_date = QDateTime::currentDateTime();
		const int current_day_of_the_week = current_date.date().dayOfWeek();

		return static_cast<int>(this->day_of_the_week) == current_day_of_the_week;
	}

	virtual std::string get_string(const size_t indent, const bool links_allowed) const override
	{
		Q_UNUSED(indent)
		Q_UNUSED(links_allowed)

		return "The current real day of the week is " + string::highlight(locale::english_locale.dayName(static_cast<int>(this->day_of_the_week)).toStdString());
	}

private:
	wyrmgus::day_of_the_week day_of_the_week;
};

}
