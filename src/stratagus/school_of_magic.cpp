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
/**@name school_of_magic.cpp - The school of magic source file. */
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

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "school_of_magic.h"

#include "config.h"
#include "upgrade/upgrade_structs.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

std::map<const CUpgrade *, CSchoolOfMagic *> CSchoolOfMagic::SchoolsOfMagicByUpgrade;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Get a school of magic by its respective upgrade
**
**	@param	upgrade	The school of magic's upgrade
**	@param	should_find	Whether it is an error if the school of magic could not be found; this is true by default
**
**	@return	The upgrade's school of magic, if any
*/
CSchoolOfMagic *CSchoolOfMagic::GetByUpgrade(const CUpgrade *upgrade, const bool should_find)
{
	std::map<const CUpgrade *, CSchoolOfMagic *>::const_iterator find_iterator = CSchoolOfMagic::SchoolsOfMagicByUpgrade.find(upgrade);
	
	if (find_iterator != CSchoolOfMagic::SchoolsOfMagicByUpgrade.end()) {
		return find_iterator->second;
	}
	
	if (should_find) {
		fprintf(stderr, "No school of magic found for upgrade: \"%s\".\n", upgrade->Ident.c_str());
	}
	
	return nullptr;
}

/**
**	@brief	Remove a school of magic
**
**	@param	school_of_magic	The school of magic
*/
void CSchoolOfMagic::Remove(CSchoolOfMagic *school_of_magic)
{
	for (std::map<const CUpgrade *, CSchoolOfMagic *>::iterator iterator = CSchoolOfMagic::SchoolsOfMagicByUpgrade.begin(); iterator != CSchoolOfMagic::SchoolsOfMagicByUpgrade.end(); ++iterator) {
		if (iterator->second == school_of_magic) {
			CSchoolOfMagic::SchoolsOfMagicByUpgrade.erase(iterator);
			break;
		}
	}
	
	Database<CSchoolOfMagic>::Remove(school_of_magic);
}

/**
**	@brief	Remove the existing schools of magic
*/
void CSchoolOfMagic::Clear()
{
	CSchoolOfMagic::SchoolsOfMagicByUpgrade.clear();
	
	Database<CSchoolOfMagic>::Clear();
}

/**
**	@brief	Process a property in the data provided by a configuration file
**
**	@param	key		The property's key
**	@param	value	The property's value
**
**	@return	True if the property can be processed, or false otherwise
*/
bool CSchoolOfMagic::ProcessConfigDataProperty(const std::string &key, std::string value)
{
	if (key == "name") {
		this->Name = value;
	} else if (key == "upgrade") {
		value = FindAndReplaceString(value, "_", "-");
		CUpgrade *upgrade = CUpgrade::Get(value);
		if (upgrade) {
			this->Upgrade = upgrade;
			CSchoolOfMagic::SchoolsOfMagicByUpgrade[upgrade] = this;
		} else {
			fprintf(stderr, "Invalid upgrade: \"%s\".\n", value.c_str());
		}
	} else if (key == "ability") {
		value = FindAndReplaceString(value, "_", "-");
		CUpgrade *ability = CUpgrade::Get(value);
		if (ability) {
			this->Abilities.push_back(ability);
			ability->SchoolsOfMagic.push_back(this);
		} else {
			fprintf(stderr, "Invalid upgrade: \"%s\".\n", value.c_str());
		}
	} else if (key == "description") {
		this->Description = value;
	} else if (key == "background") {
		this->Background = value;
	} else if (key == "quote") {
		this->Quote = value;
	} else {
		return false;
	}
	
	return true;
}
