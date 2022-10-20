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
//      (c) Copyright 2020-2022 by Andrettin
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

#include "species/taxon_base.h"

#include "database/gsml_data.h"
#include "fallback_name_generator.h"
#include "gendered_name_generator.h"
#include "name_generator.h"
#include "species/taxon.h"
#include "util/gender.h"
#include "util/vector_util.h"

namespace wyrmgus {

taxon_base::taxon_base(const std::string &identifier) : detailed_data_entry(identifier)
{
}

taxon_base::~taxon_base()
{
}

void taxon_base::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "specimen_names") {
		if (this->specimen_name_generator == nullptr) {
			this->specimen_name_generator = std::make_unique<gendered_name_generator>();
		}

		if (!values.empty()) {
			this->specimen_name_generator->add_names(gender::none, values);
		}

		scope.for_each_child([&](const gsml_data &child_scope) {
			const std::string &tag = child_scope.get_tag();

			const gender gender = enum_converter<wyrmgus::gender>::to_enum(tag);

			this->specimen_name_generator->add_names(gender, child_scope.get_values());
		});
	} else {
		data_entry::process_gsml_scope(scope);
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

	if (this->specimen_name_generator != nullptr) {
		fallback_name_generator::get()->add_specimen_names(this->specimen_name_generator);
		this->specimen_name_generator->propagate_ungendered_names();
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

bool taxon_base::is_ethereal() const
{
	if (this->ethereal) {
		return true;
	}

	if (this->get_supertaxon() != nullptr) {
		return this->get_supertaxon()->is_ethereal();
	}

	return false;
}

const name_generator *taxon_base::get_specimen_name_generator(const gender gender) const
{
	const name_generator *name_generator = nullptr;

	if (this->specimen_name_generator != nullptr) {
		name_generator = this->specimen_name_generator->get_name_generator(gender);
	}

	if (name_generator != nullptr && name_generator->get_name_count() >= name_generator::minimum_name_count) {
		return name_generator;
	}

	if (this->get_supertaxon() != nullptr) {
		return this->get_supertaxon()->get_specimen_name_generator(gender);
	}

	return name_generator;
}

void taxon_base::add_specimen_name(const gender gender, const name_variant &name)
{
	if (this->specimen_name_generator == nullptr) {
		this->specimen_name_generator = std::make_unique<gendered_name_generator>();
	}

	this->specimen_name_generator->add_name(gender, name);

	if (gender == gender::none) {
		this->specimen_name_generator->add_name(gender::male, name);
		this->specimen_name_generator->add_name(gender::female, name);
	}

	if (this->get_supertaxon() != nullptr) {
		this->get_supertaxon()->add_specimen_name(gender, name);
	}
}

void taxon_base::add_specimen_names_from(const taxon_base *other)
{
	if (other->specimen_name_generator != nullptr) {
		if (this->specimen_name_generator == nullptr) {
			this->specimen_name_generator = std::make_unique<gendered_name_generator>();
		}

		this->specimen_name_generator->add_names_from(other->specimen_name_generator);
	}

	if (this->get_supertaxon() != nullptr) {
		this->get_supertaxon()->add_specimen_names_from(other);
	}
}

}
