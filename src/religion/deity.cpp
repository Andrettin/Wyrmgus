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

#include "religion/deity.h"

#include "civilization.h"
#include "config.h"
#include "config_operator.h"
#include "faction.h"
#include "religion/deity_domain.h"
#include "religion/pantheon.h"
#include "religion/religion.h"
#include "ui/icon.h"
#include "upgrade/upgrade.h"
#include "world/plane.h"
#include "world/province.h"

#include <algorithm>

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

std::map<const CUpgrade *, CDeity *> CDeity::DeitiesByUpgrade;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Get a deity by its respective upgrade
**
**	@param	upgrade	The deity's upgrade
**	@param	should_find	Whether it is an error if the deity could not be found; this is true by default
**
**	@return	The upgrade's deity, if any
*/
CDeity *CDeity::GetByUpgrade(const CUpgrade *upgrade, const bool should_find)
{
	if (DeitiesByUpgrade.find(upgrade) != DeitiesByUpgrade.end()) {
		return DeitiesByUpgrade.find(upgrade)->second;
	}
	
	if (should_find) {
		fprintf(stderr, "No deity found for upgrade: \"%s\".\n", upgrade->Ident.c_str());
	}
	
	return nullptr;
}

/**
**	@brief	Remove a deity
**
**	@param	deity	The deity
*/
void CDeity::Remove(CDeity *deity)
{
	for (std::map<const CUpgrade *, CDeity *>::iterator iterator = CDeity::DeitiesByUpgrade.begin(); iterator != CDeity::DeitiesByUpgrade.end(); ++iterator) {
		if (iterator->second == deity) {
			CDeity::DeitiesByUpgrade.erase(iterator);
			break;
		}
	}
	
	DataType<CDeity>::Remove(deity);
}

/**
**	@brief	Remove the existing deities
*/
void CDeity::Clear()
{
	CDeity::DeitiesByUpgrade.clear();
	
	DataType<CDeity>::Clear();
}

/**
**	@brief	Process data provided by a configuration file
**
**	@param	config_data	The configuration data
*/
void CDeity::ProcessConfigData(const CConfigData *config_data)
{
	for (const CConfigProperty &property : config_data->Properties) {
		if (property.Operator != CConfigOperator::Assignment) {
			fprintf(stderr, "Wrong operator enumeration index for property \"%s\": %i.\n", property.Key.c_str(), property.Operator);
			continue;
		}
		
		if (property.Key == "pantheon") {
			std::string value = FindAndReplaceString(property.Value, "_", "-");
			this->Pantheon = CPantheon::Get(value);
		} else if (property.Key == "gender") {
			this->Gender = GetGenderIdByName(property.Value);
		} else if (property.Key == "major") {
			this->Major = StringToBool(property.Value);
		} else if (property.Key == "civilization") {
			std::string value = FindAndReplaceString(property.Value, "_", "-");
			CCivilization *civilization = CCivilization::Get(value);
			if (civilization != nullptr) {
				this->Civilizations.push_back(civilization);
				civilization->Deities.push_back(this);
			}
		} else if (property.Key == "religion") {
			std::string value = FindAndReplaceString(property.Value, "_", "-");
			CReligion *religion = CReligion::Get(value);
			if (religion) {
				this->Religions.push_back(religion);
			}
		} else if (property.Key == "domain") {
			std::string value = FindAndReplaceString(property.Value, "_", "-");
			CDeityDomain *deity_domain = CDeityDomain::Get(value);
			if (deity_domain) {
				this->Domains.push_back(deity_domain);
			}
		} else if (property.Key == "description") {
			this->Description = property.Value;
		} else if (property.Key == "background") {
			this->Background = property.Value;
		} else if (property.Key == "quote") {
			this->Quote = property.Value;
		} else if (property.Key == "icon") {
			std::string value = FindAndReplaceString(property.Value, "_", "-");
			this->Icon.Name = value;
			this->Icon.Icon = nullptr;
			this->Icon.Load();
			this->Icon.Icon->Load();
		} else if (property.Key == "home_plane") {
			std::string value = FindAndReplaceString(property.Value, "_", "-");
			CPlane *plane = CPlane::Get(value);
			if (plane) {
				this->HomePlane = plane;
			}
		} else if (property.Key == "deity_upgrade") {
			std::string value = FindAndReplaceString(property.Value, "_", "-");
			CUpgrade *upgrade = CUpgrade::Get(value);
			if (upgrade) {
				this->DeityUpgrade = upgrade;
				CDeity::DeitiesByUpgrade[upgrade] = this;
			} else {
				fprintf(stderr, "Invalid upgrade: \"%s\".\n", value.c_str());
			}
		} else if (property.Key == "character_upgrade") {
			std::string value = FindAndReplaceString(property.Value, "_", "-");
			CUpgrade *upgrade = CUpgrade::Get(value);
			if (upgrade) {
				this->CharacterUpgrade = upgrade;
			} else {
				fprintf(stderr, "Invalid upgrade: \"%s\".\n", value.c_str());
			}
		} else if (property.Key == "holy_order") {
			std::string value = FindAndReplaceString(property.Value, "_", "-");
			CFaction *holy_order = CFaction::Get(value);
			if (holy_order) {
				this->HolyOrders.push_back(holy_order);
				holy_order->HolyOrderDeity = this;
			}
		} else {
			fprintf(stderr, "Invalid deity property: \"%s\".\n", property.Key.c_str());
		}
	}
	
	for (const CConfigData *section : config_data->Sections) {
		if (section->Tag == "cultural_names") {
			for (const CConfigProperty &property : section->Properties) {
				if (property.Operator != CConfigOperator::Assignment) {
					fprintf(stderr, "Wrong operator enumeration index for property \"%s\": %i.\n", property.Key.c_str(), property.Operator);
					continue;
				}
				
				std::string key = property.Key;
				key = FindAndReplaceString(key, "_", "-");
				
				const CCivilization *civilization = CCivilization::Get(key);
				
				if (civilization) {
					this->CulturalNames[civilization] = property.Value;
				}
			}
		} else {
			fprintf(stderr, "Invalid deity property: \"%s\".\n", section->Tag.c_str());
		}
	}
	
	if (this->Major && this->Domains.size() > MAJOR_DEITY_DOMAIN_MAX) { // major deities can only have up to three domains
		this->Domains.resize(MAJOR_DEITY_DOMAIN_MAX);
	} else if (!this->Major && this->Domains.size() > MINOR_DEITY_DOMAIN_MAX) { // minor deities can only have one domain
		this->Domains.resize(MINOR_DEITY_DOMAIN_MAX);
	}
	
	for (CDeityDomain *domain : this->Domains) {
		for (CUpgrade *ability : domain->Abilities) {
			if (std::find(this->Abilities.begin(), this->Abilities.end(), ability) == this->Abilities.end()) {
				this->Abilities.push_back(ability);
			}
		}
	}
}

/**
**	@brief	Get the cultural name for a given civilization
**
**	@param	civilization	The civilization
**
**	@return	The name if present, or an empty string otherwise
*/
std::string CDeity::GetCulturalName(const CCivilization *civilization) const
{
	std::map<const CCivilization *, std::string>::const_iterator find_iterator = this->CulturalNames.find(civilization);
	
	if (find_iterator != this->CulturalNames.end()) {
		return find_iterator->second;
	}
	
	return std::string();
}
