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

namespace wyrmgus {

class sml_data;
enum class sml_operator;

class sml_parser
{
public:
	explicit sml_parser();

	sml_data parse(const std::filesystem::path &filepath);
	sml_data parse(const std::string &sml_string);

private:
	void parse(std::istream &istream, sml_data &sml_data);
	void parse_line(const std::string &line);
	bool parse_escaped_character(std::string &current_string, const char c);
	void parse_tokens();
	void reset();

private:
	std::vector<std::string> tokens;
	sml_data *current_sml_data = nullptr;
	std::string current_key;
	sml_operator current_property_operator;
};

}
