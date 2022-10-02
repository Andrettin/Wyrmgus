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
//      (c) Copyright 2015-2022 by Andrettin
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

#include "item/item_slot.h"

namespace wyrmgus {

template class enum_converter<item_slot>;

template <>
const std::string enum_converter<item_slot>::property_class_identifier = "wyrmgus::item_slot";

template <>
const std::map<std::string, item_slot> enum_converter<item_slot>::string_to_enum_map = {
	{ "weapon", item_slot::weapon },
	{ "shield", item_slot::shield },
	{ "helmet", item_slot::helmet },
	{ "armor", item_slot::armor },
	{ "gloves", item_slot::gloves },
	{ "boots", item_slot::boots },
	{ "belt", item_slot::belt },
	{ "amulet", item_slot::amulet },
	{ "ring", item_slot::ring },
	{ "arrows", item_slot::arrows }
};

template <>
const bool enum_converter<item_slot>::initialized = enum_converter::initialize();

}
