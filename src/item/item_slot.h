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

enum class item_slot {
	none = -1,
	weapon,
	shield,
	helmet,
	armor,
	gloves,
	boots,
	belt,
	amulet,
	ring,
	arrows,

	count
};

inline item_slot string_to_item_slot(const std::string &str)
{
	if (str == "weapon") {
		return item_slot::weapon;
	} else if (str == "shield") {
		return item_slot::shield;
	} else if (str == "helmet") {
		return item_slot::helmet;
	} else if (str == "armor") {
		return item_slot::armor;
	} else if (str == "gloves") {
		return item_slot::gloves;
	} else if (str == "boots") {
		return item_slot::boots;
	} else if (str == "belt") {
		return item_slot::belt;
	} else if (str == "amulet") {
		return item_slot::amulet;
	} else if (str == "ring") {
		return item_slot::ring;
	} else if (str == "arrows") {
		return item_slot::arrows;
	}

	throw std::runtime_error("Invalid item slot: \"" + str + "\".");
}

inline std::string item_slot_to_string(const item_slot item_slot)
{
	switch (item_slot) {
		case item_slot::weapon:
			return "weapon";
		case item_slot::shield:
			return "shield";
		case item_slot::helmet:
			return "helmet";
		case item_slot::armor:
			return "armor";
		case item_slot::gloves:
			return "gloves";
		case item_slot::boots:
			return "boots";
		case item_slot::belt:
			return "belt";
		case item_slot::amulet:
			return "amulet";
		case item_slot::ring:
			return "ring";
		case item_slot::arrows:
			return "arrows";
		default:
			break;
	}

	throw std::runtime_error("Invalid item slot: \"" + std::to_string(static_cast<int>(item_slot)) + "\".");
}

}

Q_DECLARE_METATYPE(wyrmgus::item_slot)
