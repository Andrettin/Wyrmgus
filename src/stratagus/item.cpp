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
	if (item_class == "dagger") {
		return DaggerItemClass;
	} else if (item_class == "sword") {
		return SwordItemClass;
	} else if (item_class == "rapier") {
		return RapierItemClass;
	} else if (item_class == "axe") {
		return AxeItemClass;
	} else if (item_class == "mace") {
		return MaceItemClass;
	} else if (item_class == "spear") {
		return SpearItemClass;
	} else if (item_class == "bow") {
		return BowItemClass;
	} else if (item_class == "throwing-axe") {
		return ThrowingAxeItemClass;
	} else if (item_class == "javelin") {
		return JavelinItemClass;
	} else if (item_class == "shield") {
		return ShieldItemClass;
	} else if (item_class == "helmet") {
		return HelmetItemClass;
	} else if (item_class == "armor") {
		return ArmorItemClass;
	} else if (item_class == "shoes") {
		return ShoesItemClass;
	} else if (item_class == "amulet") {
		return AmuletItemClass;
	} else if (item_class == "ring") {
		return RingItemClass;
	} else if (item_class == "potion") {
		return PotionItemClass;
	}

	return -1;
}

std::string GetItemClassNameById(int item_class)
{
	if (item_class == DaggerItemClass) {
		return "dagger";
	} else if (item_class == SwordItemClass) {
		return "sword";
	} else if (item_class == RapierItemClass) {
		return "rapier";
	} else if (item_class == AxeItemClass) {
		return "axe";
	} else if (item_class == MaceItemClass) {
		return "mace";
	} else if (item_class == SpearItemClass) {
		return "spear";
	} else if (item_class == BowItemClass) {
		return "bow";
	} else if (item_class == ThrowingAxeItemClass) {
		return "throwing-axe";
	} else if (item_class == JavelinItemClass) {
		return "javelin";
	} else if (item_class == ShieldItemClass) {
		return "shield";
	} else if (item_class == HelmetItemClass) {
		return "helmet";
	} else if (item_class == ArmorItemClass) {
		return "armor";
	} else if (item_class == ShoesItemClass) {
		return "shoes";
	} else if (item_class == AmuletItemClass) {
		return "amulet";
	} else if (item_class == RingItemClass) {
		return "ring";
	} else if (item_class == PotionItemClass) {
		return "potion";
	}

	return "";
}

int GetItemClassSlot(int item_class)
{
	if (
		item_class == DaggerItemClass
		|| item_class == SwordItemClass
		|| item_class == RapierItemClass
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
