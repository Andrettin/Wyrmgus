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
/**@name item.h - The item headerfile. */
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

#ifndef __ITEM_H__
#define __ITEM_H__

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include <vector>

#ifndef __ICONS_H__
#include "icons.h"
#endif

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CUnitType;
class CUpgrade;

/**
**  Indexes into item type array.
*/
enum ItemClasses {
	SwordItemClass,
	AxeItemClass,
	MaceItemClass,
	SpearItemClass,
	BowItemClass,
	ThrowingAxeItemClass,
	JavelinItemClass,

	ShieldItemClass,

	HelmetItemClass,
	ArmorItemClass,
	ShoesItemClass,

	AmuletItemClass,
	RingItemClass,
	
	PotionItemClass,

	MaxItemClasses
};

class CItemType
{
public:
	CItemType() :
		Class(-1),
		UnitType(NULL)
	{
	}
	
	int Class;					/// Item type's class
	std::string Name;			/// Name of the item type
	IconConfig Icon;			/// Item's icon
	CUnitType *UnitType;		/// Unit type used by the item when it is on the ground
};

class CItem
{
public:
	CItem() :
		ItemType(NULL)
	{
	}
	
	CItemType *ItemType;		/// Item type of the item
};

/*----------------------------------------------------------------------------
-- Variables
----------------------------------------------------------------------------*/

extern std::vector<CItemType *> ItemTypes;
extern std::vector<CItem *> Items;

/*----------------------------------------------------------------------------
-- Functions
----------------------------------------------------------------------------*/

extern int GetItemClassIdByName(std::string item_class);
extern std::string GetItemClassNameById(int item_class);
extern CItemType *GetItemType(std::string item_type_name);
extern void CleanItemTypes();
extern void CleanItems();
extern void ItemCclRegister();

//@}

#endif // !__ITEM_H__
