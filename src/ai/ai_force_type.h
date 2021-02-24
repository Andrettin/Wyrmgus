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
//      (c) Copyright 2015-2021 by Andrettin
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

enum class ai_force_type {
	none = -1,
	land,
	naval,
	air,
	space,

	count
};

inline ai_force_type string_to_ai_force_type(const std::string &str)
{
	if (str == "land") {
		return ai_force_type::land;
	} else if (str == "naval") {
		return ai_force_type::naval;
	} else if (str == "air") {
		return ai_force_type::air;
	} else if (str == "space") {
		return ai_force_type::space;
	}

	throw std::runtime_error("Invalid AI force type: \"" + str + "\".");
}

}

Q_DECLARE_METATYPE(wyrmgus::ai_force_type)
