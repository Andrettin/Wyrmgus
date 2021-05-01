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
//      (c) Copyright 2021 by Andrettin
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

namespace wyrmgus {

enum class day_of_the_week {
	monday = 1,
	tuesday = 2,
	wednesday = 3,
	thursday = 4,
	friday = 5,
	saturday = 6,
	sunday = 7
};

inline day_of_the_week string_to_day_of_the_week(const std::string &str)
{
	if (str == "monday") {
		return day_of_the_week::monday;
	} else if (str == "tuesday") {
		return day_of_the_week::tuesday;
	} else if (str == "wednesday") {
		return day_of_the_week::wednesday;
	} else if (str == "thursday") {
		return day_of_the_week::thursday;
	} else if (str == "friday") {
		return day_of_the_week::friday;
	} else if (str == "saturday") {
		return day_of_the_week::saturday;
	} else if (str == "sunday") {
		return day_of_the_week::sunday;
	}

	throw std::runtime_error("Invalid day of the week: \"" + str + "\".");
}

}
