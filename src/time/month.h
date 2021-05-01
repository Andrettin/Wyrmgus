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
//

#pragma once

namespace wyrmgus {

enum class month {
	january = 1,
	february = 2,
	march = 3,
	april = 4,
	may = 5,
	june = 6,
	july = 7,
	august = 8,
	september = 9,
	october = 10,
	november = 11,
	december = 12
};

inline month string_to_month(const std::string &str)
{
	if (str == "january") {
		return month::january;
	} else if (str == "february") {
		return month::february;
	} else if (str == "march") {
		return month::march;
	} else if (str == "april") {
		return month::april;
	} else if (str == "may") {
		return month::may;
	} else if (str == "june") {
		return month::june;
	} else if (str == "july") {
		return month::july;
	} else if (str == "august") {
		return month::august;
	} else if (str == "september") {
		return month::september;
	} else if (str == "october") {
		return month::october;
	} else if (str == "november") {
		return month::november;
	} else if (str == "december") {
		return month::december;
	}

	throw std::runtime_error("Invalid month: \"" + str + "\".");
}

}
