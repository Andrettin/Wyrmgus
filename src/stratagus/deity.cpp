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
/**@name deity.cpp - The deity source file. */
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

#include "deity.h"

#include "civilization.h"
#include "config.h"
#include "deity_domain.h"
#include "plane.h"
#include "player.h"
#include "province.h"
#include "religion.h"
#include "upgrade.h"

#include <algorithm>

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

std::vector<CDeity *> CDeity::Deities;
std::map<std::string, CDeity *> CDeity::DeitiesByIdent;
std::map<const CUpgrade *, CDeity *> CDeity::DeitiesByUpgrade;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Get a deity
**
**	@param	ident		The deity's string identifier
**	@param	should_find	Whether it is an error if the deity could not be found; this is true by default
**
**	@return	The deity if found, or null otherwise
*/
CDeity *CDeity::GetDeity(const std::string &ident, const bool should_find)
{
	if (DeitiesByIdent.find(ident) != DeitiesByIdent.end()) {
		return DeitiesByIdent.find(ident)->second;
	}
	
	if (should_find) {
		fprintf(stderr, "Invalid deity: \"%s\".\n", ident.c_str());
	}
	
	return NULL;
}

/**
**	@brief	Get or add a deity
**
**	@param	ident	The deity's string identifier
**
**	@return	The deity if found, or a newly-created one otherwise
*/
CDeity *CDeity::GetOrAddDeity(const std::string &ident)
{
	CDeity *deity = GetDeity(ident, false);
	
	if (!deity) {
		deity = new CDeity;
		deity->Ident = ident;
		Deities.push_back(deity);
		DeitiesByIdent[ident] = deity;
	}
	
	return deity;
}

/**
**	@brief	Get a deity by its respective upgrade
**
**	@param	upgrade	The deity's upgrade
**	@param	should_find	Whether it is an error if the deity could not be found; this is true by default
**
**	@return	The upgrade's deity, if any
*/
CDeity *CDeity::GetDeityByUpgrade(const CUpgrade *upgrade, const bool should_find)
{
	if (DeitiesByUpgrade.find(upgrade) != DeitiesByUpgrade.end()) {
		return DeitiesByUpgrade.find(upgrade)->second;
	}
	
	if (should_find) {
		fprintf(stderr, "No deity found for upgrade: \"%s\".\n", upgrade->Ident.c_str());
	}
	
	return NULL;
}

/**
**	@brief	Remove the existing deities
*/
void CDeity::ClearDeities()
{
	for (size_t i = 0; i < Deities.size(); ++i) {
		delete Deities[i];
	}
	Deities.clear();
}

/**
**	@brief	Process data provided by a configuration file
**
**	@param	config_data	The configuration data
*/
void CDeity::ProcessConfigData(const CConfigData *config_data)
{
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "name") {
			this->Name = value;
		} else if (key == "pantheon") {
			this->Pantheon = value;
		} else if (key == "gender") {
			this->Gender = GetGenderIdByName(value);
		} else if (key == "major") {
			this->Major = StringToBool(value);
		} else if (key == "civilization") {
			value = FindAndReplaceString(value, "_", "-");
			CCivilization *civilization = CCivilization::GetCivilization(value);
			if (civilization != NULL) {
				this->Civilizations.push_back(civilization);
				civilization->Deities.push_back(this);
			}
		} else if (key == "religion") {
			value = FindAndReplaceString(value, "_", "-");
			CReligion *religion = CReligion::GetReligion(value.c_str());
			if (religion) {
				this->Religions.push_back(religion);
			} else {
				fprintf(stderr, "Religion \"%s\" doesn't exist.\n", value.c_str());
			}
		} else if (key == "domain") {
			value = FindAndReplaceString(value, "_", "-");
			CDeityDomain *deity_domain = CDeityDomain::GetDeityDomain(value.c_str());
			if (deity_domain) {
				this->Domains.push_back(deity_domain);
			}
		} else if (key == "description") {
			this->Description = value;
		} else if (key == "background") {
			this->Background = value;
		} else if (key == "quote") {
			this->Quote = value;
		} else if (key == "icon") {
			value = FindAndReplaceString(value, "_", "-");
			this->Icon.Name = value;
			this->Icon.Icon = NULL;
			this->Icon.Load();
			this->Icon.Icon->Load();
		} else if (key == "home_plane") {
			value = FindAndReplaceString(value, "_", "-");
			CPlane *plane = CPlane::GetPlane(value);
			if (plane) {
				this->HomePlane = plane;
			} else {
				fprintf(stderr, "Plane \"%s\" does not exist.\n", value.c_str());
			}
		} else {
			fprintf(stderr, "Invalid deity property: \"%s\".\n", key.c_str());
		}
	}
}

//@}
