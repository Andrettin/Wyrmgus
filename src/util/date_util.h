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
//      (c) Copyright 2019-2021 by Andrettin
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

namespace wyrmgus::date {

static constexpr int months_per_year = 12;
static constexpr int days_per_year = 365;
static constexpr int hours_per_day = 24;

extern std::string year_to_string(const int year);

inline std::string to_string(const QDate &date)
{
	if (!date.isValid()) {
		throw std::runtime_error("Date is not valid, and cannot be converted to a string.");
	}

	return date::year_to_string(date.year()) + '.' + std::to_string(date.month()) + '.' + std::to_string(date.day());
}

inline std::string to_string(const QDateTime &date_time)
{
	if (!date_time.isValid()) {
		throw std::runtime_error("Date time is not valid, and cannot be converted to a string.");
	}

	const QDate date = date_time.date();
	const QTime time = date_time.time();

	return date::to_string(date) + '.' + std::to_string(time.hour());
}

inline int get_days_in_month(const int month, const int year)
{
	return QDate(year, month, 1).daysInMonth();
}

}
