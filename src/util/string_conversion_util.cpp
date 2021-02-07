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

#include "util/string_conversion_util.h"

#include "util/string_util.h"

namespace wyrmgus::string {

QDateTime to_date(const std::string &date_str)
{
	const std::vector<std::string> date_string_list = string::split(date_str, '.');

	int years = 0;
	int months = 1;
	int days = 1;
	int hours = 12;

	if (date_string_list.size() >= 1) {
		years = std::stoi(date_string_list[0]);

		if (date_string_list.size() >= 2) {
			months = std::stoi(date_string_list[1]);

			if (date_string_list.size() >= 3) {
				days = std::stoi(date_string_list[2]);

				if (date_string_list.size() >= 4) {
					hours = std::stoi(date_string_list[3]);
				}
			}
		}
	}

	QDateTime date(QDate(years, months, days), QTime(hours, 0), Qt::UTC);

	if (!date.isValid()) {
		throw std::runtime_error("Date \"" + date_str + "\" is not a valid date.");
	}

	return date;
}

QTime to_time(const std::string &time_str)
{
	const std::vector<std::string> time_string_list = string::split(time_str, '.');

	int hours = 0;
	int minutes = 0;
	int seconds = 0;
	int milliseconds = 0;

	if (time_string_list.size() >= 1) {
		hours = std::stoi(time_string_list[0]);

		if (time_string_list.size() >= 2) {
			minutes = std::stoi(time_string_list[1]);

			if (time_string_list.size() >= 3) {
				seconds = std::stoi(time_string_list[2]);

				if (time_string_list.size() >= 4) {
					milliseconds = std::stoi(time_string_list[3]);
				}
			}
		}
	}

	QTime time(hours, minutes, seconds, milliseconds);

	if (!time.isValid()) {
		throw std::runtime_error("Time \"" + time_str + "\" is not a valid time.");
	}

	return time;
}

}
