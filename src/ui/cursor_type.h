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
//      (c) Copyright 1998-2021 by Lutz Sammer and Andrettin
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

namespace wyrmgus {

enum class cursor_type {
	point,
	magnifying_glass,
	cross,
	green_hair,
	yellow_hair,
	red_hair,
	scroll,
	arrow_east,
	arrow_northeast,
	arrow_north,
	arrow_northwest,
	arrow_west,
	arrow_southwest,
	arrow_south,
	arrow_southeast,

	count
};

inline cursor_type string_to_cursor_type(const std::string &str)
{
	if (str == "point") {
		return cursor_type::point;
	} else if (str == "magnifying_glass") {
		return cursor_type::magnifying_glass;
	} else if (str == "cross") {
		return cursor_type::cross;
	} else if (str == "green_hair") {
		return cursor_type::green_hair;
	} else if (str == "yellow_hair") {
		return cursor_type::yellow_hair;
	} else if (str == "red_hair") {
		return cursor_type::red_hair;
	} else if (str == "scroll") {
		return cursor_type::scroll;
	} else if (str == "arrow_east") {
		return cursor_type::arrow_east;
	} else if (str == "arrow_northeast") {
		return cursor_type::arrow_northeast;
	} else if (str == "arrow_north") {
		return cursor_type::arrow_north;
	} else if (str == "arrow_northwest") {
		return cursor_type::arrow_northwest;
	} else if (str == "arrow_west") {
		return cursor_type::arrow_west;
	} else if (str == "arrow_southwest") {
		return cursor_type::arrow_southwest;
	} else if (str == "arrow_south") {
		return cursor_type::arrow_south;
	} else if (str == "arrow_southeast") {
		return cursor_type::arrow_southeast;
	}
	
	throw std::runtime_error("Invalid cursor type: \"" + str + "\".");
}

}

Q_DECLARE_METATYPE(wyrmgus::cursor_type)
