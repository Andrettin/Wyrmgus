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
//      (c) Copyright 2020 by Andrettin
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

#include "species/taxon_base.h"

#include "database/sml_data.h"
#include "gender.h"
#include "species/taxon.h"
#include "util/vector_util.h"

namespace wyrmgus {

void taxon_base::process_sml_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "specimen_names") {
		if (!values.empty()) {
			vector::merge(this->specimen_names[gender::none], values);
		}

		scope.for_each_child([&](const sml_data &child_scope) {
			const std::string &tag = child_scope.get_tag();

			const wyrmgus::gender gender = string_to_gender(tag);
			vector::merge(this->specimen_names[gender], child_scope.get_values());
		});
	} else {
		data_entry::process_sml_scope(scope);
	}
}

void taxon_base::initialize()
{
	if (this->get_supertaxon() != nullptr) {
		if (!this->get_supertaxon()->is_initialized()) {
			this->get_supertaxon()->initialize();
		}

		this->get_supertaxon()->add_specimen_names_from(this);
	}

	data_entry::initialize();
}

const taxon *taxon_base::get_supertaxon_of_rank(const taxonomic_rank rank) const
{
	if (this->get_supertaxon() == nullptr) {
		return nullptr;
	}

	if (this->get_supertaxon()->get_rank() > rank) {
		return nullptr;
	}

	if (this->get_supertaxon()->get_rank() == rank) {
		return this->get_supertaxon();
	}

	return this->get_supertaxon()->get_supertaxon_of_rank(rank);
}

bool taxon_base::is_subtaxon_of(const taxon *other_taxon) const
{
	if (this->get_supertaxon() == nullptr) {
		return false;
	}

	if (other_taxon->get_rank() <= this->get_rank()) {
		return false;
	}

	if (other_taxon == this->get_supertaxon()) {
		return true;
	}

	return this->get_supertaxon()->is_subtaxon_of(other_taxon);
}

std::vector<std::string> taxon_base::get_specimen_names(const gender gender) const
{
	static constexpr size_t minimum_name_count = 10;

	std::vector<std::string> potential_names;

	auto find_iterator = this->specimen_names.find(gender::none);
	if (find_iterator != this->specimen_names.end()) {
		vector::merge(potential_names, find_iterator->second);
	}

	if (gender != gender::none) {
		find_iterator = this->specimen_names.find(gender);
		if (find_iterator != this->specimen_names.end()) {
			vector::merge(potential_names, find_iterator->second);
		}
	}

	if (potential_names.size() < minimum_name_count && this->get_supertaxon() != nullptr) {
		return this->get_supertaxon()->get_specimen_names(gender);
	}

	return potential_names;
}

void taxon_base::add_specimen_name(const gender gender, const std::string &name)
{
	this->specimen_names[gender].push_back(name);

	if (this->get_supertaxon() != nullptr) {
		this->get_supertaxon()->add_specimen_name(gender, name);
	}
}

void taxon_base::add_specimen_names_from(const taxon_base *other)
{
	for (const auto &kv_pair : other->specimen_names) {
		vector::merge(this->specimen_names[kv_pair.first], kv_pair.second);
	}

	if (this->get_supertaxon() != nullptr) {
		this->get_supertaxon()->add_specimen_names_from(other);
	}
}

}