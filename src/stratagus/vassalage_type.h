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

namespace wyrmgus {

enum class vassalage_type {
	none,
	vassalage,
	personal_union
};

inline vassalage_type string_to_vassalage_type(const std::string &str)
{
	if (str == "none") {
		return vassalage_type::none;
	} else if (str == "vassalage") {
		return vassalage_type::vassalage;
	} else if (str == "personal_union") {
		return vassalage_type::personal_union;
	}

	throw std::runtime_error("Invalid vassalage type: \"" + str + "\".");
}

inline std::string vassalage_type_to_string(const vassalage_type vassalage_type)
{
	switch (vassalage_type) {
		case vassalage_type::none:
			return "none";
		case vassalage_type::vassalage:
			return "vassalage";
		case vassalage_type::personal_union:
			return "personal_union";
		default:
			break;
	}

	throw std::runtime_error("Invalid vassalage type: \"" + std::to_string(static_cast<int>(vassalage_type)) + "\".");
}

}

Q_DECLARE_METATYPE(wyrmgus::vassalage_type)
