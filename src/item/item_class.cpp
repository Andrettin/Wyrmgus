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

#include "stratagus.h"

#include "item/item_class.h"

#include "item/item_slot.h"

namespace wyrmgus {

item_slot get_item_class_slot(const item_class item_class)
{
	switch (item_class) {
		case item_class::dagger:
		case item_class::sword:
		case item_class::thrusting_sword:
		case item_class::axe:
		case item_class::mace:
		case item_class::spear:
		case item_class::bow:
		case item_class::throwing_axe:
		case item_class::javelin:
		case item_class::gun:
			return item_slot::weapon;
		case item_class::shield:
		case item_class::horn:
		case item_class::trinket:
			return item_slot::shield;
		case item_class::helmet:
			return item_slot::helmet;
		case item_class::armor:
		case item_class::cloak:
			return item_slot::armor;
		case item_class::gloves:
			return item_slot::gloves;
		case item_class::boots:
			return item_slot::boots;
		case item_class::belt:
			return item_slot::belt;
		case item_class::amulet:
			return item_slot::amulet;
		case item_class::ring:
			return item_slot::ring;
		case item_class::arrows:
			return item_slot::arrows;
		default:
			return item_slot::none;
	}
}

}
