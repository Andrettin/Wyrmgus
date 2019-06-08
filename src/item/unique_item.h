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
/**@name unique_item.h - The unique item header file. */
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

#ifndef __UNIQUE_ITEM_H__
#define __UNIQUE_ITEM_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "data_type.h"
#include "detailed_data_element.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CIcon;
class CUnitType;
class CUpgrade;
class CSpell;
struct lua_State;

/*----------------------------------------------------------------------------
--  Definition
----------------------------------------------------------------------------*/

class UniqueItem : public DetailedDataElement, public DataType<UniqueItem>
{
	GDCLASS(UniqueItem, DetailedDataElement)

public:
	static constexpr const char *ClassIdentifier = "unique_item";
	
	bool CanDrop() const;				/// Check whether this unique item can drop
	int GetMagicLevel() const;			/// Get this unique item's magic level
	virtual CIcon *GetIcon() const override;
	
	CIcon *GetSpecificIcon() const
	{
		return this->Icon;
	}

	int ResourcesHeld = 0;
	CUnitType *Type = nullptr;	/// Item type of the item
	CUpgrade *Prefix = nullptr;
	CUpgrade *Suffix = nullptr;
	CUpgrade *Set = nullptr;
	CSpell *Spell = nullptr;
	CUpgrade *Work = nullptr;
	CUpgrade *Elixir = nullptr;
	
	friend int CclDefineUniqueItem(lua_State *l);
	
protected:
	static inline void _bind_methods() {}
};

/*----------------------------------------------------------------------------
-- Functions
----------------------------------------------------------------------------*/

extern std::string GetUniqueItemEffectsString(const std::string &item_ident);

#endif
