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

#include "stratagus.h"

#include "religion/deity.h"

#include "civilization.h"
#include "config.h"
#include "faction.h"
#include "gender.h"
#include "plane.h"
#include "player.h"
#include "province.h"
#include "religion/deity_domain.h"
#include "religion/pantheon.h"
#include "religion/religion.h"
#include "upgrade/upgrade.h"
#include "util/string_util.h"

namespace stratagus {

deity::deity(const std::string &identifier)
	: detailed_data_entry(identifier), CDataType(identifier), gender(gender::none)
{
}

/**
**	@brief	Process data provided by a configuration file
**
**	@param	config_data	The configuration data
*/
void deity::ProcessConfigData(const CConfigData *config_data)
{
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "name") {
			this->set_name(value);
		} else if (key == "pantheon") {
			value = FindAndReplaceString(value, "_", "-");
			this->Pantheon = CPantheon::GetPantheon(value);
		} else if (key == "gender") {
			this->gender = string_to_gender(value);
		} else if (key == "major") {
			this->Major = string::to_bool(value);
		} else if (key == "civilization") {
			civilization *civilization = civilization::get(value);
			this->civilizations.push_back(civilization);
			civilization->Deities.push_back(this);
		} else if (key == "religion") {
			value = FindAndReplaceString(value, "_", "-");
			CReligion *religion = CReligion::GetReligion(value.c_str());
			if (religion) {
				this->Religions.push_back(religion);
			}
		} else if (key == "domain") {
			value = FindAndReplaceString(value, "_", "-");
			CDeityDomain *deity_domain = CDeityDomain::GetDeityDomain(value.c_str());
			if (deity_domain) {
				this->Domains.push_back(deity_domain);
			}
		} else if (key == "description") {
			this->set_description(value);
		} else if (key == "background") {
			this->set_background(value);
		} else if (key == "quote") {
			this->set_quote(value);
		} else if (key == "icon") {
			value = FindAndReplaceString(value, "_", "-");
			this->Icon.Name = value;
			this->Icon.Icon = nullptr;
			this->Icon.Load();
		} else if (key == "home_plane") {
			plane *plane = plane::get(value);
			this->home_plane = plane;
		} else if (key == "deity_upgrade") {
			value = FindAndReplaceString(value, "_", "-");
			CUpgrade *upgrade = CUpgrade::get(value);
			this->DeityUpgrade = upgrade;
			deity::deities_by_upgrade[upgrade] = this;
		} else if (key == "character_upgrade") {
			value = FindAndReplaceString(value, "_", "-");
			CUpgrade *upgrade = CUpgrade::get(value);
			this->CharacterUpgrade = upgrade;
		} else if (key == "holy_order") {
			faction *holy_order = faction::get(value);
			this->HolyOrders.push_back(holy_order);
			holy_order->HolyOrderDeity = this;
		} else {
			fprintf(stderr, "Invalid deity property: \"%s\".\n", key.c_str());
		}
	}
	
	for (const CConfigData *child_config_data : config_data->Children) {
		if (child_config_data->Tag == "cultural_names") {
			for (size_t j = 0; j < child_config_data->Properties.size(); ++j) {
				std::string key = child_config_data->Properties[j].first;
				std::string value = child_config_data->Properties[j].second;
				
				const civilization *civilization = civilization::get(key);
				this->CulturalNames[civilization] = value;
			}
		} else {
			fprintf(stderr, "Invalid deity property: \"%s\".\n", child_config_data->Tag.c_str());
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
std::string deity::GetCulturalName(const civilization *civilization) const
{
	const auto find_iterator = this->CulturalNames.find(civilization);
	
	if (find_iterator != this->CulturalNames.end()) {
		return find_iterator->second;
	}
	
	return std::string();
}

}
