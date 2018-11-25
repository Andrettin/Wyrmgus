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
/**@name age.cpp - The age source file. */
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

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "age.h"

#include "config.h"
#include "unittype.h"
#include "upgrade_structs.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

std::vector<CAge *> CAge::Ages;
std::map<std::string, CAge *> CAge::AgesByIdent;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Get an age
**
**	@param	ident	The age's string identifier
**
**	@return	The age if found, null otherwise
*/
CAge *CAge::GetAge(const std::string &ident)
{
	if (AgesByIdent.find(ident) != AgesByIdent.end()) {
		return AgesByIdent.find(ident)->second;
	}
	
	return NULL;
}

/**
**	@brief	Get or add an age
**
**	@param	ident	The age's string identifier
**
**	@return	The age if found, otherwise a new age is created and returned
*/
CAge *CAge::GetOrAddAge(const std::string &ident)
{
	CAge *age = GetAge(ident);
	
	if (!age) {
		age = new CAge;
		age->Ident = ident;
		Ages.push_back(age);
		AgesByIdent[ident] = age;
	}
	
	return age;
}

/**
**	@brief	Remove the existing ages
*/
void CAge::ClearAges()
{
	for (size_t i = 0; i < Ages.size(); ++i) {
		delete Ages[i];
	}
	Ages.clear();
}

/**
**	@brief	Process data provided by a configuration file
**
**	@param	config_data	The configuration data
*/
void CAge::ProcessConfigData(const CConfigData *config_data)
{
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "name") {
			this->Name = value;
		} else if (key == "required_upgrade") {
			value = FindAndReplaceString(value, "_", "-");
			CUpgrade *upgrade = CUpgrade::Get(value);
			if (upgrade) {
				this->RequiredUpgrades.push_back(upgrade);
			} else {
				fprintf(stderr, "Invalid upgrade: \"%s\".\n", value.c_str());
			}
		} else if (key == "required_upgrade_class") {
			value = FindAndReplaceString(value, "_", "-");
			int upgrade_class = GetUpgradeClassIndexByName(value);
			if (upgrade_class != -1) {
				this->RequiredUpgradeClasses.push_back(upgrade_class);
			} else {
				fprintf(stderr, "Invalid upgrade class: \"%s\".\n", value.c_str());
			}
		} else {
			fprintf(stderr, "Invalid age property: \"%s\".\n", key.c_str());
		}
	}
}

//@}
