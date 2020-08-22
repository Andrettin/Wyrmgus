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

#include "character.h"
#include "civilization.h"
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
	: detailed_data_entry(identifier), gender(gender::none)
{
}

void deity::process_sml_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "cultural_names") {
		scope.for_each_property([&](const sml_property &property) {
			const civilization *civilization = civilization::get(property.get_key());
			this->cultural_names[civilization] = property.get_value();
		});
	} else {
		data_entry::process_sml_scope(scope);
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

const icon *deity::get_icon() const
{
	if (this->icon != nullptr) {
		return this->icon;
	}

	if (this->get_character() != nullptr) {
		return this->get_character()->get_icon();
	}

	return nullptr;
}

gender deity::get_gender() const
{
	if (this->gender != gender::none) {
		return this->gender;
	}

	if (this->get_character() != nullptr) {
		return this->get_character()->get_gender();
	}

	return gender::none;
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
