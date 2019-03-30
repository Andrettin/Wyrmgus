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
/**@name school_of_magic.h - The school of magic header file. */
//
//      (c) Copyright 2018-2019 by Andrettin
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

#ifndef __SCHOOL_OF_MAGIC_H__
#define __SCHOOL_OF_MAGIC_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "database.h"
#include "data_element.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CUpgrade;

/*----------------------------------------------------------------------------
--  Definition
----------------------------------------------------------------------------*/

class CSchoolOfMagic : public DataElement, public Database<CSchoolOfMagic>
{
	GDCLASS(CSchoolOfMagic, DataElement)

public:
	static constexpr const char *GetClassIdentifier()
	{
		return "school_of_magic";
	}
	
	static CSchoolOfMagic *GetByUpgrade(const CUpgrade *upgrade, const bool should_find = true);
	static void Remove(CSchoolOfMagic *school_of_magic);
	static void Clear();
	
private:
	static std::map<const CUpgrade *, CSchoolOfMagic *> SchoolsOfMagicByUpgrade;

public:
	virtual bool ProcessConfigDataProperty(const std::string &key, std::string value) override;
	
	std::string Name;									/// Name of the school of magic
	std::string Description;							/// Description of the school of magic from an in-game universe perspective
	std::string Background;								/// Description of the school of magic from a perspective outside of the game's universe
	std::string Quote;									/// A quote relating to the school of magic
	CUpgrade *Upgrade = nullptr;						/// Upgrade corresponding to the school of magic
	std::vector<CUpgrade *> Abilities;					/// Abilities linked to this school of magic

protected:
	static inline void _bind_methods() {}
};

#endif
