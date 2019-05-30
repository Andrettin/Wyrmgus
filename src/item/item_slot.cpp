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
/**@name item_slot.cpp - The item slot source file. */
//
//      (c) Copyright 2019 by Andrettin
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
//

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "item/item_slot.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

void ItemSlot::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("set_quantity", "quantity"), +[](ItemSlot *slot, const int quantity){ slot->Quantity = quantity; });
	ClassDB::bind_method(D_METHOD("get_quantity"), &ItemSlot::GetQuantity);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "quantity"), "set_quantity", "get_quantity");
	
	ClassDB::bind_method(D_METHOD("set_weapon", "weapon"), +[](ItemSlot *slot, const bool weapon){ slot->Weapon = weapon; });
	ClassDB::bind_method(D_METHOD("is_weapon"), &ItemSlot::IsWeapon);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "weapon"), "set_weapon", "is_weapon");
	
	ClassDB::bind_method(D_METHOD("set_shield", "shield"), +[](ItemSlot *slot, const bool shield){ slot->Shield = shield; });
	ClassDB::bind_method(D_METHOD("is_shield"), &ItemSlot::IsShield);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "shield"), "set_shield", "is_shield");

	ClassDB::bind_method(D_METHOD("set_boots", "boots"), +[](ItemSlot *slot, const bool boots){ slot->Boots = boots; });
	ClassDB::bind_method(D_METHOD("is_boots"), &ItemSlot::IsBoots);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "boots"), "set_boots", "is_boots");

	ClassDB::bind_method(D_METHOD("set_arrows", "arrows"), +[](ItemSlot *slot, const bool arrows){ slot->Arrows = arrows; });
	ClassDB::bind_method(D_METHOD("is_arrows"), &ItemSlot::IsArrows);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "arrows"), "set_arrows", "is_arrows");
}
