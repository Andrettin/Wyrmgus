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

#include "sound/music_type.h"

namespace archimedes {

template class enum_converter<music_type>;

template <>
const std::string enum_converter<music_type>::property_class_identifier = "wyrmgus::music_type";

template <>
const std::map<std::string, music_type> enum_converter<music_type>::string_to_enum_map = {
	{ "menu", music_type::menu },
	{ "credits", music_type::credits },
	{ "loading", music_type::loading },
	{ "map", music_type::map },
	{ "victory", music_type::victory },
	{ "defeat", music_type::defeat }

};

template <>
const bool enum_converter<music_type>::initialized = enum_converter::initialize();

}
