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
//      (c) Copyright 2021-2022 by Andrettin
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

enum class ecological_niche {
	none,
	
	browser,
	grazer,
	predator,
	scavenger
};

inline ecological_niche string_to_ecological_niche(const std::string &str)
{
	if (str == "browser") {
		return ecological_niche::browser;
	} else if (str == "grazer") {
		return ecological_niche::grazer;
	} else if (str == "predator") {
		return ecological_niche::predator;
	} else if (str == "scavenger") {
		return ecological_niche::scavenger;
	}

	throw std::runtime_error("Invalid ecological niche: \"" + str + "\".");
}

inline std::string ecological_niche_to_string(const ecological_niche niche)
{
	switch (niche) {
		case ecological_niche::browser:
			return "browser";
		case ecological_niche::grazer:
			return "grazer";
		case ecological_niche::predator:
			return "predator";
		case ecological_niche::scavenger:
			return "scavenger";
		default:
			break;
	}

	throw std::runtime_error("Invalid ecological niche: \"" + std::to_string(static_cast<int>(niche)) + "\".");
}

}

Q_DECLARE_METATYPE(wyrmgus::ecological_niche)
