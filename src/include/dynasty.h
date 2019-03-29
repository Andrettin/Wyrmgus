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
/**@name dynasty.h - The dynasty header file. */
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

#ifndef __DYNASTY_H__
#define __DYNASTY_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "database.h"
#include "data_type.h"
#include "ui/icon_config.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CCivilization;
class CFaction;
class CUpgrade;
class LuaCallback;

/*----------------------------------------------------------------------------
--  Definition
----------------------------------------------------------------------------*/

class CDynasty : public CDataType, public Database<CDynasty>
{
	GDCLASS(CDynasty, CDataType)

public:
	~CDynasty();
	
	static constexpr const char *GetClassIdentifier()
	{
		return "dynasty";
	}
	
	virtual bool ProcessConfigDataProperty(const std::string &key, std::string value) override;
	
	std::string Name;													/// dynasty name
	std::string Description;											/// dynasty description
	std::string Quote;													/// dynasty quote
	std::string Background;												/// dynasty background
	CUpgrade *DynastyUpgrade = nullptr;									/// dynasty upgrade applied when the dynasty is set
	CCivilization *Civilization = nullptr;								/// dynasty civilization
	IconConfig Icon;													/// dynasty's icon
	LuaCallback *Conditions = nullptr;
	std::vector<CFaction *> Factions;									/// to which factions is this dynasty available

protected:
	static inline void _bind_methods() {}
};

#endif
