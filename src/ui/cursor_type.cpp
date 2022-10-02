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
//      (c) Copyright 2022 by Andrettin
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

#include "stratagus.h"

#include "ui/cursor_type.h"

namespace wyrmgus {

template class enum_converter<cursor_type>;

template <>
const std::string enum_converter<cursor_type>::property_class_identifier = "wyrmgus::cursor_type";

template <>
const std::map<std::string, cursor_type> enum_converter<cursor_type>::string_to_enum_map = {
	{ "point", cursor_type::point },
	{ "magnifying_glass", cursor_type::magnifying_glass },
	{ "cross", cursor_type::cross },
	{ "green_hair", cursor_type::green_hair },
	{ "yellow_hair", cursor_type::yellow_hair },
	{ "red_hair", cursor_type::red_hair },
	{ "scroll", cursor_type::scroll },
	{ "arrow_east", cursor_type::arrow_east },
	{ "arrow_northeast", cursor_type::arrow_northeast },
	{ "arrow_north", cursor_type::arrow_north },
	{ "arrow_northwest", cursor_type::arrow_northwest },
	{ "arrow_west", cursor_type::arrow_west },
	{ "arrow_southwest", cursor_type::arrow_southwest },
	{ "arrow_south", cursor_type::arrow_south },
	{ "arrow_southeast", cursor_type::arrow_southeast }
};

template <>
const bool enum_converter<cursor_type>::initialized = enum_converter::initialize();

}
