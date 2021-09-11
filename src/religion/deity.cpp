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
//      (c) Copyright 2018-2021 by Andrettin
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

#include "stratagus.h"

#include "religion/deity.h"

#include "character.h"
#include "gender.h"
#include "magic_domain.h"
#include "map/world.h"
#include "player/civilization.h"
#include "player/faction.h"
#include "player/player.h"
#include "province.h"
#include "spell/spell.h"
#include "religion/pantheon.h"
#include "religion/religion.h"
#include "upgrade/upgrade.h"
#include "util/container_util.h"
#include "util/string_util.h"
#include "util/vector_util.h"

namespace wyrmgus {

deity *deity::add(const std::string &identifier, const wyrmgus::data_module *data_module)
{
	deity *deity = data_type::add(identifier, data_module);

	//add a character with the same identifier as the deity for it
	wyrmgus::character *character = character::add(identifier, data_module);
	character->set_deity(deity);
	deity->character = character;

	return deity;
}

bool deity::compare_encyclopedia_entries(const deity *lhs, const deity *rhs)
{
	if (lhs->get_pantheon() != rhs->get_pantheon()) {
		if (lhs->get_pantheon() == nullptr || rhs->get_pantheon() == nullptr) {
			return lhs->get_pantheon() != nullptr;
		}

		return lhs->get_pantheon()->get_name() < rhs->get_pantheon()->get_name();
	}

	if (lhs->is_major() != rhs->is_major()) {
		return lhs->is_major();
	}

	return named_data_entry::compare_encyclopedia_entries(lhs, rhs);
}

deity::deity(const std::string &identifier)
	: detailed_data_entry(identifier)
{
}

void deity::process_sml_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "character") {
		database::process_sml_data(this->get_character(), scope);
	} else if (tag == "cultural_names") {
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

	for (const magic_domain *domain : this->get_domains()) {
		for (const spell *spell : domain->get_spells()) {
			if (!vector::contains(this->spells, spell)) {
				this->spells.push_back(spell);
			}
		}
	}

	data_entry::initialize();
}

std::string deity::get_encyclopedia_text() const
{
	std::string text;

	if (this->get_pantheon() != nullptr) {
		named_data_entry::concatenate_encyclopedia_text(text, "Pantheon: " + this->get_pantheon()->get_name());
	}

	if (this->get_homeworld() != nullptr) {
		named_data_entry::concatenate_encyclopedia_text(text, "Homeworld: " + this->get_homeworld()->get_link_string());
	}

	named_data_entry::concatenate_encyclopedia_text(text, "Rank: " + std::string(this->is_major() ? "Major" : "Minor"));

	std::vector<magic_domain *> domains = this->get_domains();
	std::sort(domains.begin(), domains.end(), named_data_entry::compare_encyclopedia_entries);

	if (!domains.empty()) {
		std::string domains_text;
		for (const magic_domain *domain : domains) {
			if (!domains_text.empty()) {
				domains_text += ", ";
			}

			domains_text += domain->get_name();
		}

		named_data_entry::concatenate_encyclopedia_text(text, "Domains: " + domains_text);
	}

	if (this->get_character() != nullptr) {
		named_data_entry::concatenate_encyclopedia_text(text, this->get_character()->get_encyclopedia_genealogical_text());
	}

	std::vector<civilization *> civilizations = this->get_civilizations();
	std::sort(civilizations.begin(), civilizations.end(), named_data_entry::compare_encyclopedia_entries);

	if (!civilizations.empty()) {
		std::string civilizations_text;
		for (const civilization *civilization : civilizations) {
			if (!civilizations_text.empty()) {
				civilizations_text += ", ";
			}

			civilizations_text += civilization->get_link_string();
		}

		named_data_entry::concatenate_encyclopedia_text(text, "Civilizations: " + civilizations_text);
	}

	std::vector<std::string> cultural_names;
	for (const auto &[civilization, cultural_name] : this->cultural_names) {
		cultural_names.push_back(cultural_name + " (" + civilization->get_name() + ")");
	}
	std::sort(cultural_names.begin(), cultural_names.end());

	if (!cultural_names.empty()) {
		std::string cultural_names_text;
		for (const std::string &cultural_name : cultural_names) {
			if (!cultural_names_text.empty()) {
				cultural_names_text += ", ";
			}

			cultural_names_text += cultural_name;
		}

		named_data_entry::concatenate_encyclopedia_text(text, "Cultural Names: " + cultural_names_text);
	}

	std::vector<const spell *> spells = this->get_spells();
	std::sort(spells.begin(), spells.end(), named_data_entry::compare_encyclopedia_entries);

	if (!spells.empty()) {
		std::string spells_text;
		for (const spell *spell : spells) {
			if (!spells_text.empty()) {
				spells_text += ", ";
			}

			spells_text += spell->get_name();
		}

		named_data_entry::concatenate_encyclopedia_text(text, "Spells: " + spells_text);
	}

	named_data_entry::concatenate_encyclopedia_text(text, detailed_data_entry::get_encyclopedia_text());

	return text;
}

const std::string &deity::get_cultural_name(const civilization *civilization) const
{
	const auto find_iterator = this->cultural_names.find(civilization);
	
	if (find_iterator != this->cultural_names.end()) {
		return find_iterator->second;
	}
	
	return string::empty_str;
}

icon *deity::get_icon() const
{
	return this->get_character()->get_base_icon();
}

void deity::set_icon(icon *icon)
{
	this->get_character()->set_base_icon(icon);
}

gender deity::get_gender() const
{
	return this->get_character()->get_gender();
}

void deity::set_gender(const gender gender)
{
	this->get_character()->set_gender(gender);
}

character *deity::get_father() const
{
	return this->get_character()->get_father();
}

void deity::set_father(wyrmgus::character *character)
{
	this->get_character()->set_father(character);
}

character *deity::get_mother() const
{
	return this->get_character()->get_mother();
}

void deity::set_mother(wyrmgus::character *character)
{
	this->get_character()->set_mother(character);
}

void deity::set_upgrade(CUpgrade *upgrade)
{
	if (upgrade == this->get_upgrade()) {
		return;
	}

	this->upgrade = upgrade;
	upgrade->set_deity(this);
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

void deity::remove_domain(magic_domain *domain)
{
	vector::remove(this->domains, domain);
}

}
