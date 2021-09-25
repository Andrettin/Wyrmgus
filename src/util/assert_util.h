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

inline std::string get_source_relative_filepath_string(const std::string &filepath)
{
	size_t pos = filepath.find("src");
	if (pos != std::string::npos) {
		pos += 4;
		return filepath.substr(pos, filepath.size() - pos);
	}

	return filepath;
}

#ifdef __clang__
inline void assert_throw(const bool check)
#else
inline void assert_throw(const bool check, const std::source_location &location = std::source_location::current())
#endif
{
	if (check) {
		return;
	}

#ifdef __clang__
	throw std::runtime_error("Assert failed.");
#else
	throw std::runtime_error("Assert failed at " + get_source_relative_filepath_string(location.file_name()) + ": " + std::to_string(location.line()) + ", " + location.function_name() + ".");
#endif
}

}