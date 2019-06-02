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
/**@name faction_type.h - The faction type header file. */
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

#ifndef __FACTION_TYPE_H__
#define __FACTION_TYPE_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "data_element.h"
#include "data_type.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CUpgrade;

/*----------------------------------------------------------------------------
--  Definition
----------------------------------------------------------------------------*/

class FactionType : public DataElement, public DataType<FactionType>
{
	GDCLASS(FactionType, DataElement)
	
public:
	static constexpr const char *ClassIdentifier = "faction_type";
	
	bool IsNeutral() const
	{
		return this->Neutral;
	}
	
	bool IsReligious() const
	{
		return this->Religious;
	}
	
	bool IsTribal() const
	{
		return this->Tribal;
	}
	
	const CUpgrade *GetUpgrade() const
	{
		return this->Upgrade;
	}
	
private:
	bool Neutral = false;	/// whether the faction type is a neutral one, e.g. a mercenary company
	bool Religious = false;	/// whether the faction type is a religious one
	bool Tribal = false;	/// whether the faction type is a tribal one
	const CUpgrade *Upgrade = nullptr;	/// the upgrade for the faction type
	
protected:
	static void _bind_methods();
};

#endif
