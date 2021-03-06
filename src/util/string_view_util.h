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

namespace wyrmgus::string_view {

inline std::vector<std::string_view> split(const std::string_view &str, const char delimiter)
{
	std::vector<std::string_view> string_list;

	size_t start_pos = 0;
	size_t find_pos = 0;
	while ((find_pos = str.find(delimiter, start_pos)) != std::string::npos) {
		std::string_view string_element = str.substr(start_pos, find_pos - start_pos);

		string_list.push_back(std::move(string_element));

		start_pos = find_pos + 1;
	}

	std::string_view string_element = str.substr(start_pos, str.length() - start_pos);

	string_list.push_back(std::move(string_element));

	return string_list;
}

}
