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
#include "species/gender.h"
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
**	@brief	Process a property in the data provided by a configuration file
**
**	@param	key		The property's key
**	@param	value	The property's value
**
**	@return	True if the property can be processed, or false otherwise
*/
bool CDeity::ProcessConfigDataProperty(const std::string &key, std::string value)
{
	if (key == "pantheon") {
		this->Pantheon = CPantheon::Get(value);
	} else if (key == "gender") {
		this->Gender = CGender::Get(value);
	} else if (key == "major") {
		this->Major = StringToBool(value);
	} else if (key == "civilization") {
		CCivilization *civilization = CCivilization::Get(value);
		if (civilization != nullptr) {
			this->Civilizations.push_back(civilization);
			civilization->Deities.push_back(this);
		}
	} else if (key == "religion") {
		CReligion *religion = CReligion::Get(value);
		if (religion) {
			this->Religions.push_back(religion);
		}
	} else if (key == "domain") {
		CDeityDomain *deity_domain = CDeityDomain::Get(value);
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
		this->Icon.Icon = nullptr;
		this->Icon.Load();
		this->Icon.Icon->Load();
	} else if (key == "home_plane") {
		CPlane *plane = CPlane::Get(value);
		if (plane) {
			this->HomePlane = plane;
		}
	} else if (key == "deity_upgrade") {
		value = FindAndReplaceString(value, "_", "-");
		CUpgrade *upgrade = CUpgrade::Get(value);
		if (upgrade) {
			this->DeityUpgrade = upgrade;
			CDeity::DeitiesByUpgrade[upgrade] = this;
		} else {
			fprintf(stderr, "Invalid upgrade: \"%s\".\n", value.c_str());
		}
	} else if (key == "character_upgrade") {
		value = FindAndReplaceString(value, "_", "-");
		CUpgrade *upgrade = CUpgrade::Get(value);
		if (upgrade) {
			this->CharacterUpgrade = upgrade;
		} else {
			fprintf(stderr, "Invalid upgrade: \"%s\".\n", value.c_str());
		}
	} else if (key == "holy_order") {
		CFaction *holy_order = CFaction::Get(value);
		if (holy_order) {
			this->HolyOrders.push_back(holy_order);
			holy_order->HolyOrderDeity = this;
		}
	} else {
		return false;
	}
	
	return true;
}

/**
**	@brief	Process a section in the data provided by a configuration file
**
**	@param	section		The section
**
**	@return	True if the section can be processed, or false otherwise
*/
bool CDeity::ProcessConfigDataSection(const CConfigData *section)
{
	if (section->Tag == "cultural_names") {
		for (const CConfigProperty &property : section->Properties) {
			if (property.Operator != CConfigOperator::Assignment) {
				fprintf(stderr, "Wrong operator enumeration index for property \"%s\": %i.\n", property.Key.c_str(), property.Operator);
				continue;
			}
			
			const CCivilization *civilization = CCivilization::Get(property.Key);
			
			if (civilization) {
				this->CulturalNames[civilization] = String(property.Value.c_str());
			}
		}
	} else {
		return false;
	}
	
	return true;
}

/**
**	@brief	Initialize the deity
*/
void CDeity::Initialize()
{
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
	
	this->Initialized = true;
}

/**
**	@brief	Get the cultural name for a given civilization
**
**	@param	civilization	The civilization
**
**	@return	The name if present, or an empty string otherwise
*/
String CDeity::GetCulturalName(const CCivilization *civilization) const
{
	std::map<const CCivilization *, String>::const_iterator find_iterator = this->CulturalNames.find(civilization);
	
	if (find_iterator != this->CulturalNames.end()) {
		return find_iterator->second;
	}
	
	return String();
}

void CDeity::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("get_cultural_name", "civilization"), [](const CDeity *deity, Object *civilization){ return deity->GetCulturalName(static_cast<CCivilization *>(civilization)); });
	ClassDB::bind_method(D_METHOD("is_major"), &CDeity::IsMajor);
	ClassDB::bind_method(D_METHOD("get_gender"), [](const CDeity *deity){ return const_cast<CGender *>(deity->GetGender()); });
	ClassDB::bind_method(D_METHOD("get_domains"), [](const CDeity *deity){ return VectorToGodotArray(deity->GetDomains()); });
}
