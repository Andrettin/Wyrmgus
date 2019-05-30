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
/**@name item_class.cpp - The item class source file. */
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

#include "item/item_class.h"

#include "item/item_slot.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

void ItemClass::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("set_slot", "slot"), +[](ItemClass *item_class, const String &slot_ident){ item_class->Slot = ItemSlot::Get(slot_ident); });
	ClassDB::bind_method(D_METHOD("get_slot"), +[](const ItemClass *item_class){ return const_cast<ItemSlot *>(item_class->GetSlot()); });
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "slot"), "set_slot", "get_slot");
	
	ClassDB::bind_method(D_METHOD("set_consumable", "consumable"), +[](ItemClass *item_class, const bool consumable){ item_class->Consumable = consumable; });
	ClassDB::bind_method(D_METHOD("is_consumable"), &ItemClass::IsConsumable);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "consumable"), "set_consumable", "is_consumable");
	
	ClassDB::bind_method(D_METHOD("set_two_handed", "two_handed"), +[](ItemClass *item_class, const bool two_handed){ item_class->TwoHanded = two_handed; });
	ClassDB::bind_method(D_METHOD("is_two_handed"), &ItemClass::IsTwoHanded);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "two_handed"), "set_two_handed", "is_two_handed");
	
	ClassDB::bind_method(D_METHOD("set_thrusting_weapon", "thrusting_weapon"), +[](ItemClass *item_class, const bool thrusting_weapon){ item_class->ThrustingWeapon = thrusting_weapon; });
	ClassDB::bind_method(D_METHOD("is_thrusting_weapon"), &ItemClass::IsThrustingWeapon);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "thrusting_weapon"), "set_thrusting_weapon", "is_thrusting_weapon");
	
	ClassDB::bind_method(D_METHOD("set_allow_arrows", "allow_arrows"), +[](ItemClass *item_class, const bool allow_arrows){ item_class->AllowArrows = allow_arrows; });
	ClassDB::bind_method(D_METHOD("allows_arrows"), &ItemClass::AllowsArrows);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "allow_arrows"), "set_allow_arrows", "allows_arrows");
	
	ClassDB::bind_method(D_METHOD("set_shield", "shield"), +[](ItemClass *item_class, const bool shield){ item_class->Shield = shield; });
	ClassDB::bind_method(D_METHOD("is_shield"), &ItemClass::IsShield);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "shield"), "set_shield", "is_shield");
	
	ClassDB::bind_method(D_METHOD("set_special_property_required", "special_property_required"), +[](ItemClass *item_class, const bool special_property_required){ item_class->SpecialPropertyRequired = special_property_required; });
	ClassDB::bind_method(D_METHOD("is_special_property_required"), &ItemClass::IsSpecialPropertyRequired);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "special_property_required"), "set_special_property_required", "is_special_property_required");
	
	ClassDB::bind_method(D_METHOD("set_shield_compatible", "shield_compatible"), +[](ItemClass *item_class, const bool shield_compatible){ item_class->ShieldCompatible = shield_compatible; });
	ClassDB::bind_method(D_METHOD("is_shield_compatible"), &ItemClass::IsShieldCompatible);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "shield_compatible"), "set_shield_compatible", "is_shield_compatible");
}
