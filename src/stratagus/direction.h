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
//      (c) Copyright 2015-2022 by Andrettin
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

enum class direction {
	north,
	northeast,
	east,
	southeast,
	south,
	southwest,
	west,
	northwest
};

inline direction string_to_direction(const std::string &str)
{
	if (str == "north") {
		return direction::north;
	} else if (str == "northeast") {
		return direction::northeast;
	} else if (str == "east") {
		return direction::east;
	} else if (str == "southeast") {
		return direction::southeast;
	} else if (str == "south") {
		return direction::south;
	} else if (str == "southwest") {
		return direction::southwest;
	} else if (str == "west") {
		return direction::west;
	} else if (str == "northwest") {
		return direction::northwest;
	}

	throw std::runtime_error("Invalid direction: \"" + str + "\".");
}

inline std::string direction_to_string(const direction direction)
{
	switch (direction) {
		case direction::north:
			return "north";
		case direction::northeast:
			return "northeast";
		case direction::east:
			return "east";
		case direction::southeast:
			return "southeast";
		case direction::south:
			return "south";
		case direction::southwest:
			return "southwest";
		case direction::west:
			return "west";
		case direction::northwest:
			return "northwest";
		default:
			break;
	}

	throw std::runtime_error("Invalid direction: \"" + std::to_string(static_cast<int>(direction)) + "\".");
}

}

Q_DECLARE_METATYPE(wyrmgus::direction)
