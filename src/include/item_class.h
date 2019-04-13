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
/**@name item_class.h - The item class header file. */
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

#ifndef __ITEM_CLASS_H__
#define __ITEM_CLASS_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "data_element.h"
#include "data_type.h"

/*----------------------------------------------------------------------------
--  Definition
----------------------------------------------------------------------------*/

class ItemClass : public DataElement, public DataType<ItemClass>
{
	DATA_TYPE(ItemClass, DataElement)

public:
	static constexpr const char *ClassIdentifier = "item_class";
	
private:
	static inline bool InitializeClass()
	{
		PROPERTY_KEY("consumable", Consumable);
		PROPERTY_KEY("two_handed", TwoHanded);
		PROPERTY_KEY("thrusting_weapon", ThrustingWeapon);
		PROPERTY_KEY("allow_arrows", AllowArrows);
		PROPERTY_KEY("shield", Shield);
		PROPERTY_KEY("special_property_required", SpecialPropertyRequired);
		PROPERTY_KEY("shield_compatible", ShieldCompatible);
		
		return true;
	}
	
	static inline bool ClassInitialized = InitializeClass();
	
public:
	virtual bool ProcessConfigDataProperty(const std::string &key, std::string value) override;
	
	Property<int> Slot = -1;				/// the item slot of the item class
	Property<bool> Consumable = false;		/// whether the item class is consumable
	Property<bool> TwoHanded = false;		/// whether the item class is a two-handed weapon
	Property<bool> ThrustingWeapon = false;	/// whether the item class is a thrusting weapon
	Property<bool> AllowArrows = false;		/// whether the item class allows arrows to be equipped
	Property<bool> Shield = false;			/// whether the item class is a shield
	Property<bool> SpecialPropertyRequired = false;	/// whether items of the item class must always have a special property (e.g. a magic affix)
	Property<bool> ShieldCompatible = false;	/// whether the item class is compatible with shields
	
protected:
	static inline void _bind_methods() {}
};

#endif
