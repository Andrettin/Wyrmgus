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
//      (c) Copyright 2019-2022 by Andrettin
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

namespace wyrmgus::string {

inline bool to_bool(const std::string &str)
{
	if (str == "true" || str == "yes" || str == "1") {
		return true;
	} else if (str == "false" || str == "no" || str == "0") {
		return false;
	}

	throw std::runtime_error("Invalid string used for conversion to boolean: \"" + str + "\".");
}

inline std::string from_bool(const bool value)
{
	if (value) {
		return "true";
	} else {
		return "false";
	}
}

inline char to_character(const std::string &str)
{
	if (str.size() != 1) {
		throw std::runtime_error("Character string \"" + str + "\" has a string size different than 1.");
	}

	return str.front();
}

extern QDateTime to_date(const std::string &date_str);
extern QTime to_time(const std::string &time_str);

}
