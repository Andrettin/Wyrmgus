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
//      (c) Copyright 2020-2021 by Andrettin
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

#include "network.h"
#include "script/condition/condition.h"
#include "time/month.h"
#include "util/locale_util.h"

namespace wyrmgus {

class real_month_condition final : public condition
{
public:
	explicit real_month_condition(const std::string &value)
	{
		this->month = string_to_month(value);
	}

	virtual bool check(const CPlayer *player, const bool ignore_units) const override
	{
		Q_UNUSED(player)
		Q_UNUSED(ignore_units)

		if (IsNetworkGame()) {
			//always false in multiplayer games, to prevent desyncs if the real month changes during a game
			return false;
		}

		const QDateTime current_date = QDateTime::currentDateTime();
		const int current_month = current_date.date().month();

		return static_cast<int>(this->month) == current_month;
	}

	virtual std::string get_string(const size_t indent) const override
	{
		Q_UNUSED(indent)

		return "The current real month is " + string::highlight(locale::english_locale.standaloneMonthName(static_cast<int>(this->month)).toStdString());
	}

private:
	wyrmgus::month month;
};

}
