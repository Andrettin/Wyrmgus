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
//      (c) Copyright 1998-2021 by Lutz Sammer, Jimmy Salmon and Andrettin
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

enum class unit_domain {
	none = -1,
	land, //can only move on land
	water, //can only move on water
	air, //can move on land and water, and over non-passable terrain like rocks or trees
	air_low, //flying unit, but it flies low, so that it can be attacked by melee land units and cannot fly over rocks or trees
	space  //can move in space, as well as over any other kind of terrain that flying units can
};


inline unit_domain string_to_unit_domain(const std::string &str)
{
	if (str == "land") {
		return unit_domain::land;
	} else if (str == "water") {
		return unit_domain::water;
	} else if (str == "air") {
		return unit_domain::air;
	} else if (str == "air_low") {
		return unit_domain::air_low;
	} else if (str == "space") {
		return unit_domain::space;
	}

	throw std::runtime_error("Invalid unit domain: \"" + str + "\".");
}

inline std::string unit_domain_to_string(const unit_domain domain)
{
	switch (domain) {
		case unit_domain::land:
			return "land";
		case unit_domain::water:
			return "water";
		case unit_domain::air:
			return "air";
		case unit_domain::air_low:
			return "air_low";
		case unit_domain::space:
			return "space";
		default:
			break;
	}

	throw std::runtime_error("Invalid unit domain: \"" + std::to_string(static_cast<int>(domain)) + "\".");
}

}

Q_DECLARE_METATYPE(wyrmgus::unit_domain)
