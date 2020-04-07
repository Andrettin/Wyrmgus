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
/**@name deity_domain.cpp - The deity domain source file. */
//
//      (c) Copyright 2018-2020 by Andrettin
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

#include "religion/deity_domain.h"

#include "config.h"
#include "upgrade/upgrade_structs.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

std::vector<CDeityDomain *> CDeityDomain::DeityDomains;
std::map<std::string, CDeityDomain *> CDeityDomain::DeityDomainsByIdent;
std::map<const CUpgrade *, CDeityDomain *> CDeityDomain::DeityDomainsByUpgrade;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Get a deity domain
**
**	@param	ident		The deity domain's string identifier
**	@param	should_find	Whether it is an error if the deity domain could not be found; this is true by default
**
**	@return	The deity domain if found, or null otherwise
*/
CDeityDomain *CDeityDomain::GetDeityDomain(const std::string &ident, const bool should_find)
{
	std::map<std::string, CDeityDomain *>::const_iterator find_iterator = DeityDomainsByIdent.find(ident);
	
	if (find_iterator != DeityDomainsByIdent.end()) {
		return find_iterator->second;
	}
	
	if (should_find) {
		fprintf(stderr, "Invalid deity domain: \"%s\".\n", ident.c_str());
	}
	
	return nullptr;
}

/**
**	@brief	Get or add a deity domain
**
**	@param	ident	The deity domain's string identifier
**
**	@return	The deity domain if found, or a newly-created one otherwise
*/
CDeityDomain *CDeityDomain::GetOrAddDeityDomain(const std::string &ident)
{
	CDeityDomain *deity_domain = GetDeityDomain(ident, false);
	
	if (!deity_domain) {
		deity_domain = new CDeityDomain;
		deity_domain->Ident = ident;
		DeityDomains.push_back(deity_domain);
		DeityDomainsByIdent[ident] = deity_domain;
	}
	
	return deity_domain;
}

/**
**	@brief	Get a deity domain by its respective upgrade
**
**	@param	upgrade	The deity domain's upgrade
**	@param	should_find	Whether it is an error if the deity domain could not be found; this is true by default
**
**	@return	The upgrade's deity domain, if any
*/
CDeityDomain *CDeityDomain::GetDeityDomainByUpgrade(const CUpgrade *upgrade, const bool should_find)
{
	std::map<const CUpgrade *, CDeityDomain *>::const_iterator find_iterator = DeityDomainsByUpgrade.find(upgrade);
	
	if (find_iterator != DeityDomainsByUpgrade.end()) {
		return find_iterator->second;
	}
	
	if (should_find) {
		fprintf(stderr, "No deity domain found for upgrade: \"%s\".\n", upgrade->Ident.c_str());
	}
	
	return nullptr;
}

/**
**	@brief	Remove the existing deity domains
*/
void CDeityDomain::ClearDeityDomains()
{
	for (size_t i = 0; i < DeityDomains.size(); ++i) {
		delete DeityDomains[i];
	}
	DeityDomains.clear();
}

/**
**	@brief	Process data provided by a configuration file
**
**	@param	config_data	The configuration data
*/
void CDeityDomain::ProcessConfigData(const CConfigData *config_data)
{
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "name") {
			this->Name = value;
		} else if (key == "upgrade") {
			value = FindAndReplaceString(value, "_", "-");
			CUpgrade *upgrade = CUpgrade::get(value);
			this->Upgrade = upgrade;
			CDeityDomain::DeityDomainsByUpgrade[upgrade] = this;
		} else if (key == "ability") {
			value = FindAndReplaceString(value, "_", "-");
			CUpgrade *ability = CUpgrade::get(value);
			this->Abilities.push_back(ability);
			ability->DeityDomains.push_back(this);
		} else if (key == "description") {
			this->Description = value;
		} else if (key == "background") {
			this->Background = value;
		} else if (key == "quote") {
			this->Quote = value;
		} else {
			fprintf(stderr, "Invalid school of magic property: \"%s\".\n", key.c_str());
		}
	}
}
