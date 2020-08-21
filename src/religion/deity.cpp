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
#include "util/container_util.h"
#include "util/string_util.h"
#include "util/vector_util.h"

namespace wyrmgus {

deity::deity(const std::string &identifier)
	: detailed_data_entry(identifier), CDataType(identifier), gender(gender::none)
{
}

void deity::ProcessConfigData(const CConfigData *config_data)
{
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "name") {
			this->set_name(value);
		} else if (key == "pantheon") {
			this->pantheon = pantheon::get(value);
		} else if (key == "gender") {
			this->gender = string_to_gender(value);
		} else if (key == "major") {
			this->major = string::to_bool(value);
		} else if (key == "civilization") {
			civilization *civilization = civilization::get(value);
			this->add_civilization(civilization);
		} else if (key == "religion") {
			religion *religion = religion::get(value);
			this->religions.push_back(religion);
		} else if (key == "domain") {
			deity_domain *domain = deity_domain::get(value);
			this->domains.push_back(domain);
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
				this->cultural_names[civilization] = value;
			}
		} else {
			fprintf(stderr, "Invalid deity property: \"%s\".\n", child_config_data->Tag.c_str());
		}
	}
}

void deity::initialize()
{

	if (this->is_major() && this->domains.size() > deity::major_deity_domain_max) { // major deities can only have up to three domains
		this->domains.resize(deity::major_deity_domain_max);
	} else if (!this->is_major() && this->domains.size() > deity::minor_deity_domain_max) { // minor deities can only have one domain
		this->domains.resize(deity::minor_deity_domain_max);
	}

	for (deity_domain *domain : this->get_domains()) {
		for (CUpgrade *ability : domain->Abilities) {
			if (!vector::contains(this->Abilities, ability)) {
				this->Abilities.push_back(ability);
			}
		}
	}

	data_entry::initialize();
}

const std::string &deity::get_cultural_name(const civilization *civilization) const
{
	const auto find_iterator = this->cultural_names.find(civilization);
	
	if (find_iterator != this->cultural_names.end()) {
		return find_iterator->second;
	}
	
	return string::empty_str;
}

QVariantList deity::get_civilizations_qvariant_list() const
{
	return container::to_qvariant_list(this->get_civilizations());
}

void deity::add_civilization(civilization *civilization)
{
	this->civilizations.push_back(civilization);
	civilization->Deities.push_back(this);
}

void deity::remove_civilization(civilization *civilization)
{
	vector::remove(this->civilizations, civilization);
	vector::remove(civilization->Deities, this);
}

QVariantList deity::get_religions_qvariant_list() const
{
	return container::to_qvariant_list(this->get_religions());
}

void deity::remove_religion(religion *religion)
{
	vector::remove(this->religions, religion);
}

QVariantList deity::get_domains_qvariant_list() const
{
	return container::to_qvariant_list(this->get_domains());
}

void deity::remove_domain(deity_domain *domain)
{
	vector::remove(this->domains, domain);
}

}
