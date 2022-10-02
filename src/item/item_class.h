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

#pragma once

#include "util/enum_converter.h"

namespace wyrmgus {

enum class item_slot;

enum class item_class {
	none = -1,
	dagger,
	sword,
	thrusting_sword,
	axe,
	mace,
	spear,
	bow,
	throwing_axe,
	javelin,
	gun,

	shield,
	horn,
	trinket,

	helmet,
	armor,
	cloak,
	gloves,
	belt,
	boots,

	amulet,
	ring,
	
	arrows,

	food,
	potion,
	scroll,
	book,

	count
};

extern template class enum_converter<item_class>;

extern item_slot get_item_class_slot(const item_class item_class);

inline bool is_thrusting_weapon_item_class(const item_class item_class)
{
	switch (item_class) {
		case item_class::dagger:
		case item_class::sword:
		case item_class::thrusting_sword:
		case item_class::spear:
			return true;
		default:
			return false;
	}
}

inline bool is_shield_incompatible_weapon_item_class(const item_class item_class)
{
	switch (item_class) {
		case item_class::dagger:
		case item_class::throwing_axe:
		case item_class::javelin:
		case item_class::gun:
			return true;
		default:
			return false;
	}
}

inline bool is_consumable_item_class(const item_class item_class)
{
	switch (item_class) {
		case item_class::food:
		case item_class::potion:
		case item_class::scroll:
		case item_class::book:
			return true;
		default:
			return false;
	}
}

inline std::string get_item_class_name(const item_class item_class)
{
	switch (item_class) {
		case item_class::dagger:
			return "Dagger";
		case item_class::sword:
			return "Sword";
		case item_class::thrusting_sword:
			return "Thrusting Sword";
		case item_class::axe:
			return "Axe";
		case item_class::mace:
			return "Mace";
		case item_class::spear:
			return "Spear";
		case item_class::bow:
			return "Bow";
		case item_class::throwing_axe:
			return "Throwing Axe";
		case item_class::javelin:
			return "Javelin";
		case item_class::gun:
			return "Gun";
		case item_class::shield:
			return "Shield";
		case item_class::horn:
			return "Horn";
		case item_class::trinket:
			return "Trinket";
		case item_class::helmet:
			return "Helmet";
		case item_class::armor:
			return "Armor";
		case item_class::cloak:
			return "Cloak";
		case item_class::gloves:
			return "Gloves";
		case item_class::belt:
			return "Belt";
		case item_class::boots:
			return "Boots";
		case item_class::amulet:
			return "Amulet";
		case item_class::ring:
			return "Ring";
		case item_class::arrows:
			return "Arrows";
		case item_class::food:
			return "Food";
		case item_class::potion:
			return "Potion";
		case item_class::scroll:
			return "Scroll";
		case item_class::book:
			return "Book";
		default:
			break;
	}

	throw std::runtime_error("Invalid item class: \"" + std::to_string(static_cast<int>(item_class)) + "\".");
}

}

Q_DECLARE_METATYPE(wyrmgus::item_class)
