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

inline item_class string_to_item_class(const std::string &str)
{
	if (str == "dagger") {
		return item_class::dagger;
	} else if (str == "sword") {
		return item_class::sword;
	} else if (str == "thrusting_sword") {
		return item_class::thrusting_sword;
	} else if (str == "axe") {
		return item_class::axe;
	} else if (str == "mace") {
		return item_class::mace;
	} else if (str == "spear") {
		return item_class::spear;
	} else if (str == "bow") {
		return item_class::bow;
	} else if (str == "throwing_axe") {
		return item_class::throwing_axe;
	} else if (str == "javelin") {
		return item_class::javelin;
	} else if (str == "gun") {
		return item_class::gun;
	} else if (str == "shield") {
		return item_class::shield;
	} else if (str == "horn") {
		return item_class::horn;
	} else if (str == "trinket") {
		return item_class::trinket;
	} else if (str == "helmet") {
		return item_class::helmet;
	} else if (str == "armor") {
		return item_class::armor;
	} else if (str == "cloak") {
		return item_class::cloak;
	} else if (str == "gloves") {
		return item_class::gloves;
	} else if (str == "belt") {
		return item_class::belt;
	} else if (str == "boots") {
		return item_class::boots;
	} else if (str == "amulet") {
		return item_class::amulet;
	} else if (str == "ring") {
		return item_class::ring;
	} else if (str == "arrows") {
		return item_class::arrows;
	} else if (str == "food") {
		return item_class::food;
	} else if (str == "potion") {
		return item_class::potion;
	} else if (str == "scroll") {
		return item_class::scroll;
	} else if (str == "book") {
		return item_class::book;
	}

	throw std::runtime_error("Invalid item class: \"" + str + "\".");
}

inline std::string item_class_to_string(const item_class item_class)
{
	switch (item_class) {
		case item_class::dagger:
			return "dagger";
		case item_class::sword:
			return "sword";
		case item_class::thrusting_sword:
			return "thrusting_sword";
		case item_class::axe:
			return "axe";
		case item_class::mace:
			return "mace";
		case item_class::spear:
			return "spear";
		case item_class::bow:
			return "bow";
		case item_class::throwing_axe:
			return "throwing_axe";
		case item_class::javelin:
			return "javelin";
		case item_class::gun:
			return "gun";
		case item_class::shield:
			return "shield";
		case item_class::horn:
			return "horn";
		case item_class::trinket:
			return "trinket";
		case item_class::helmet:
			return "helmet";
		case item_class::armor:
			return "armor";
		case item_class::cloak:
			return "cloak";
		case item_class::gloves:
			return "gloves";
		case item_class::belt:
			return "belt";
		case item_class::boots:
			return "boots";
		case item_class::amulet:
			return "amulet";
		case item_class::ring:
			return "ring";
		case item_class::arrows:
			return "arrows";
		case item_class::food:
			return "food";
		case item_class::potion:
			return "potion";
		case item_class::scroll:
			return "scroll";
		case item_class::book:
			return "book";
		default:
			break;
	}

	throw std::runtime_error("Invalid item class: \"" + std::to_string(static_cast<int>(item_class)) + "\".");
}

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
