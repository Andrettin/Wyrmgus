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
/**@name item.cpp - The items. */
//
//      (c) Copyright 2015 by Andrettin
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

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "item.h"

#include <ctype.h>

#include <string>
#include <map>

#include "game.h"
#include "parameters.h"
#include "player.h"
#include "unittype.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

int GetItemClassIdByName(std::string item_class)
{
	if (item_class == "Sword") {
		return SwordItemClass;
	} else if (item_class == "Axe") {
		return AxeItemClass;
	} else if (item_class == "Mace") {
		return MaceItemClass;
	} else if (item_class == "Spear") {
		return SpearItemClass;
	} else if (item_class == "Bow") {
		return BowItemClass;
	} else if (item_class == "Throwing Axe") {
		return ThrowingAxeItemClass;
	} else if (item_class == "Javelin") {
		return JavelinItemClass;
	} else if (item_class == "Shield") {
		return ShieldItemClass;
	} else if (item_class == "Helmet") {
		return HelmetItemClass;
	} else if (item_class == "Armor") {
		return ArmorItemClass;
	} else if (item_class == "Shoes") {
		return ShoesItemClass;
	} else if (item_class == "Amulet") {
		return AmuletItemClass;
	} else if (item_class == "Ring") {
		return RingItemClass;
	} else if (item_class == "Potion") {
		return PotionItemClass;
	}

	return -1;
}

std::string GetItemClassNameById(int item_class)
{
	if (item_class == SwordItemClass) {
		return "Sword";
	} else if (item_class == AxeItemClass) {
		return "Axe";
	} else if (item_class == MaceItemClass) {
		return "Mace";
	} else if (item_class == SpearItemClass) {
		return "Spear";
	} else if (item_class == BowItemClass) {
		return "Bow";
	} else if (item_class == ThrowingAxeItemClass) {
		return "Throwing Axe";
	} else if (item_class == JavelinItemClass) {
		return "Javelin";
	} else if (item_class == ShieldItemClass) {
		return "Shield";
	} else if (item_class == HelmetItemClass) {
		return "Helmet";
	} else if (item_class == ArmorItemClass) {
		return "Armor";
	} else if (item_class == ShoesItemClass) {
		return "Shoes";
	} else if (item_class == AmuletItemClass) {
		return "Amulet";
	} else if (item_class == RingItemClass) {
		return "Ring";
	} else if (item_class == PotionItemClass) {
		return "Potion";
	}

	return "";
}

int GetItemClassSlot(int item_class)
{
	if (
		item_class == SwordItemClass
		|| item_class == AxeItemClass
		|| item_class == MaceItemClass
		|| item_class == SpearItemClass
		|| item_class == BowItemClass
		|| item_class == ThrowingAxeItemClass
		|| item_class == JavelinItemClass
	) {
		return WeaponItemSlot;
	} else if (item_class == ShieldItemClass) {
		return ShieldItemSlot;
	} else if (item_class == HelmetItemClass) {
		return HelmetItemSlot;
	} else if (item_class == ArmorItemClass) {
		return ArmorItemSlot;
	} else if (item_class == ShoesItemClass) {
		return ShoesItemSlot;
	} else if (item_class == AmuletItemClass) {
		return AmuletItemSlot;
	} else if (item_class == RingItemClass) {
		return RingItemSlot;
	}

	return -1;
}

//@}
