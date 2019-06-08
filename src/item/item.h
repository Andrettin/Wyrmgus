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
/**@name item.h - The item header file. */
//
//      (c) Copyright 2015-2019 by Andrettin
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

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include <vector>

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CCharacter;
class CConfigData;
class CIcon;
class CUnitType;
class CUpgrade;
class CSpell;
class CVariable;
class UniqueItem;

/*----------------------------------------------------------------------------
--  Definitions
----------------------------------------------------------------------------*/

class CPersistentItem
{
public:
	void ProcessConfigData(const CConfigData *config_data);
	
	String Name;
	bool Bound = false;			/// Whether the item is bound to its owner and can't be dropped
	bool Identified = true;		/// Whether the item has been identified
	const CUnitType *Type = nullptr;	/// Item type of the item
	const CUpgrade *Prefix = nullptr;
	const CUpgrade *Suffix = nullptr;
	const CSpell *Spell = nullptr;
	const CUpgrade *Work = nullptr;
	const CUpgrade *Elixir = nullptr;
	UniqueItem *Unique = nullptr;
	CCharacter *Owner = nullptr;
};

/*----------------------------------------------------------------------------
-- Functions
----------------------------------------------------------------------------*/

extern std::string GetItemEffectsString(const std::string &item_ident);
extern void ItemCclRegister();

#endif
