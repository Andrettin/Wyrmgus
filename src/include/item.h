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
//      (c) Copyright 2015-2020 by Andrettin
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

#pragma once

#include "ui/icon.h"

class CConfigData;
class CUpgrade;
class CSpell;

namespace stratagus {
	class character;
	class unit_type;
}

class CUniqueItem
{
public:
	CUniqueItem() :
		ResourcesHeld(0), Type(nullptr), Prefix(nullptr), Suffix(nullptr), Set(nullptr), Spell(nullptr), Work(nullptr), Elixir(nullptr)
	{
	}
	
	bool CanDrop() const;				/// Check whether this unique item can drop
	int GetMagicLevel() const;			/// Get this unique item's magic level
	IconConfig GetIcon() const;

	int ResourcesHeld;
	std::string Ident;
	std::string Name;
	std::string Description;
	std::string Background;
	std::string Quote;
	IconConfig Icon;			/// Unique item's icon (if it differs from that of its type)
	stratagus::unit_type *Type;			/// Item type of the item
	CUpgrade *Prefix;
	CUpgrade *Suffix;
	CUpgrade *Set;
	CSpell *Spell;
	CUpgrade *Work;
	CUpgrade *Elixir;
};

class CPersistentItem
{
public:
	void ProcessConfigData(const CConfigData *config_data);
	
	std::string Name;
	bool Bound = false;			/// Whether the item is bound to its owner and can't be dropped
	bool Identified = true;		/// Whether the item has been identified
	stratagus::unit_type *Type = nullptr;	/// Item type of the item
	CUpgrade *Prefix = nullptr;
	CUpgrade *Suffix = nullptr;
	CSpell *Spell = nullptr;
	CUpgrade *Work = nullptr;
	CUpgrade *Elixir = nullptr;
	CUniqueItem *Unique = nullptr;
	stratagus::character *Owner = nullptr;
};

/*----------------------------------------------------------------------------
-- Variables
----------------------------------------------------------------------------*/

extern std::vector<CUniqueItem *> UniqueItems;

/*----------------------------------------------------------------------------
-- Functions
----------------------------------------------------------------------------*/

extern void CleanUniqueItems();
extern CUniqueItem *GetUniqueItem(const std::string &item_ident);
extern std::string GetItemEffectsString(const std::string &item_ident);
extern std::string GetUniqueItemEffectsString(const std::string &item_ident);
extern void ItemCclRegister();
