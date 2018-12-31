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
//      (c) Copyright 2018 by Andrettin
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

std::vector<CSchoolOfMagic *> CSchoolOfMagic::SchoolsOfMagic;
std::map<std::string, CSchoolOfMagic *> CSchoolOfMagic::SchoolsOfMagicByIdent;
std::map<const CUpgrade *, CSchoolOfMagic *> CSchoolOfMagic::SchoolsOfMagicByUpgrade;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Get a school of magic
**
**	@param	ident		The school of magic's string identifier
**	@param	should_find	Whether it is an error if the school of magic could not be found; this is true by default
**
**	@return	The school of magic if found, or null otherwise
*/
CSchoolOfMagic *CSchoolOfMagic::GetSchoolOfMagic(const std::string &ident, const bool should_find)
{
	std::map<std::string, CSchoolOfMagic *>::const_iterator find_iterator = SchoolsOfMagicByIdent.find(ident);
	
	if (find_iterator != SchoolsOfMagicByIdent.end()) {
		return find_iterator->second;
	}
	
	if (should_find) {
		fprintf(stderr, "Invalid school of magic: \"%s\".\n", ident.c_str());
	}
	
	return nullptr;
}

/**
**	@brief	Get or add a school of magic
**
**	@param	ident	The school of magic's string identifier
**
**	@return	The school of magic if found, or a newly-created one otherwise
*/
CSchoolOfMagic *CSchoolOfMagic::GetOrAddSchoolOfMagic(const std::string &ident)
{
	CSchoolOfMagic *school_of_magic = GetSchoolOfMagic(ident, false);
	
	if (!school_of_magic) {
		school_of_magic = new CSchoolOfMagic;
		school_of_magic->Ident = ident;
		SchoolsOfMagic.push_back(school_of_magic);
		SchoolsOfMagicByIdent[ident] = school_of_magic;
	}
	
	return school_of_magic;
}

/**
**	@brief	Get a school of magic by its respective upgrade
**
**	@param	upgrade	The school of magic's upgrade
**	@param	should_find	Whether it is an error if the school of magic could not be found; this is true by default
**
**	@return	The upgrade's school of magic, if any
*/
CSchoolOfMagic *CSchoolOfMagic::GetSchoolOfMagicByUpgrade(const CUpgrade *upgrade, const bool should_find)
{
	std::map<const CUpgrade *, CSchoolOfMagic *>::const_iterator find_iterator = SchoolsOfMagicByUpgrade.find(upgrade);
	
	if (find_iterator != SchoolsOfMagicByUpgrade.end()) {
		return find_iterator->second;
	}
	
	if (should_find) {
		fprintf(stderr, "No school of magic found for upgrade: \"%s\".\n", upgrade->Ident.c_str());
	}
	
	return nullptr;
}

/**
**	@brief	Remove the existing schools of magic
*/
void CSchoolOfMagic::ClearSchoolsOfMagic()
{
	for (size_t i = 0; i < SchoolsOfMagic.size(); ++i) {
		delete SchoolsOfMagic[i];
	}
	SchoolsOfMagic.clear();
}

/**
**	@brief	Process data provided by a configuration file
**
**	@param	config_data	The configuration data
*/
void CSchoolOfMagic::ProcessConfigData(const CConfigData *config_data)
{
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
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
		} else {
			fprintf(stderr, "Invalid school of magic property: \"%s\".\n", key.c_str());
		}
	}
}
