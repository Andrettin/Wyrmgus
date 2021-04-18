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

#pragma once

namespace wyrmgus {

enum class difficulty {
	none,
	easy,
	normal,
	hard,
	brutal
};

inline difficulty string_to_difficulty(const std::string &str)
{
	if (str == "easy") {
		return difficulty::easy;
	} else if (str == "normal") {
		return difficulty::normal;
	} else if (str == "hard") {
		return difficulty::hard;
	} else if (str == "brutal") {
		return difficulty::brutal;
	}

	throw std::runtime_error("Invalid difficulty: \"" + str + "\".");
}

inline std::string difficulty_to_string(const difficulty difficulty)
{
	switch (difficulty) {
		case difficulty::easy:
			return "easy";
		case difficulty::normal:
			return "normal";
		case difficulty::hard:
			return "hard";
		case difficulty::brutal:
			return "brutal";
		default:
			break;
	}

	throw std::runtime_error("Invalid difficulty: \"" + std::to_string(static_cast<int>(difficulty)) + "\".");
}

inline std::string get_difficulty_name(const difficulty difficulty)
{
	switch (difficulty) {
		case difficulty::easy:
			return "Easy";
		case difficulty::normal:
			return "Normal";
		case difficulty::hard:
			return "Hard";
		case difficulty::brutal:
			return "Brutal";
		default:
			break;
	}

	throw std::runtime_error("Invalid difficulty: \"" + std::to_string(static_cast<int>(difficulty)) + "\".");
}

}

Q_DECLARE_METATYPE(wyrmgus::difficulty)
