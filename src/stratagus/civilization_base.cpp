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

#include "civilization_base.h"

#include "civilization_group.h"
#include "database/sml_data.h"
#include "database/sml_operator.h"
#include "gender.h"
#include "name_generator.h"
#include "unit/unit_class.h"
#include "util/container_util.h"
#include "util/vector_util.h"

namespace wyrmgus {

void civilization_base::process_sml_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "personal_names") {
		if (!values.empty()) {
			vector::merge(this->personal_names[gender::none], values);
		}

		scope.for_each_child([&](const sml_data &child_scope) {
			const std::string &tag = child_scope.get_tag();

			const wyrmgus::gender gender = string_to_gender(tag);
			vector::merge(this->personal_names[gender], child_scope.get_values());
		});
	} else if (tag == "surnames") {
		vector::merge(this->surnames, values);
	} else if (tag == "unit_class_names") {
		scope.for_each_child([&](const sml_data &child_scope) {
			const std::string &tag = child_scope.get_tag();

			const unit_class *unit_class = unit_class::get(tag);
			vector::merge(this->unit_class_names[unit_class], child_scope.get_values());
		});
	} else if (tag == "ship_names") {
		vector::merge(this->ship_names, values);
	} else {
		data_entry::process_sml_scope(scope);
	}
}

void civilization_base::initialize()
{
	if (this->group != nullptr) {
		if (!this->group->is_initialized()) {
			this->group->initialize();
		}

		if (this->get_species() == nullptr) {
			this->set_species(this->get_group()->species);
		}

		this->group->add_names_from(this);
	}

	name_generator::get()->add_personal_names(this->personal_names);
	name_generator::get()->add_surnames(this->surnames);
	name_generator::get()->add_unit_class_names(this->unit_class_names);
	name_generator::get()->add_ship_names(this->ship_names);

	name_generator::propagate_ungendered_names(this->personal_names);

	data_entry::initialize();
}

const std::vector<std::string> &civilization_base::get_personal_names(const gender gender) const
{
	const auto find_iterator = this->personal_names.find(gender);
	if (find_iterator != this->personal_names.end() && find_iterator->second.size() >= name_generator::minimum_name_count) {
		return find_iterator->second;
	}

	if (this->get_group() != nullptr) {
		return this->get_group()->get_personal_names(gender);
	}

	if (find_iterator != this->personal_names.end()) {
		return find_iterator->second;
	}

	return vector::empty_string_vector;
}

void civilization_base::add_personal_name(const gender gender, const std::string &name)
{
	this->personal_names[gender].push_back(name);

	if (gender == gender::none) {
		this->personal_names[gender::male].push_back(name);
		this->personal_names[gender::female].push_back(name);
	}

	if (this->group != nullptr) {
		this->group->add_personal_name(gender, name);
	}
}

const std::vector<std::string> &civilization_base::get_surnames() const
{
	if (this->surnames.size() >= name_generator::minimum_name_count) {
		return this->surnames;
	}

	if (this->get_group() != nullptr) {
		return this->get_group()->get_surnames();
	}

	return this->surnames;
}

void civilization_base::add_surname(const std::string &surname)
{
	this->surnames.push_back(surname);

	if (this->group != nullptr) {
		this->group->add_surname(surname);
	}
}

const std::vector<std::string> &civilization_base::get_unit_class_names(const unit_class *unit_class) const
{
	const auto find_iterator = this->unit_class_names.find(unit_class);
	if (find_iterator != this->unit_class_names.end() && find_iterator->second.size() >= name_generator::minimum_name_count) {
		return find_iterator->second;
	}

	if (this->get_group() != nullptr) {
		return this->get_group()->get_unit_class_names(unit_class);
	}

	return name_generator::get()->get_unit_class_names(unit_class);
}

void civilization_base::add_unit_class_name(const unit_class *unit_class, const std::string &name)
{
	this->unit_class_names[unit_class].push_back(name);

	if (this->group != nullptr) {
		this->group->add_unit_class_name(unit_class, name);
	}
}

const std::vector<std::string> &civilization_base::get_ship_names() const
{
	if (this->ship_names.size() >= name_generator::minimum_name_count) {
		return this->ship_names;
	}

	if (this->get_group() != nullptr) {
		return this->get_group()->get_ship_names();
	}

	return name_generator::get()->get_ship_names();
}

void civilization_base::add_ship_name(const std::string &ship_name)
{
	this->ship_names.push_back(ship_name);

	if (this->group != nullptr) {
		this->group->add_ship_name(ship_name);
	}
}

void civilization_base::add_names_from(const civilization_base *other)
{
	for (const auto &kv_pair : other->personal_names) {
		vector::merge(this->personal_names[kv_pair.first], kv_pair.second);
	}

	name_generator::propagate_ungendered_names(other->personal_names, this->personal_names);

	vector::merge(this->surnames, other->surnames);

	for (const auto &kv_pair : other->unit_class_names) {
		vector::merge(this->unit_class_names[kv_pair.first], kv_pair.second);
	}

	vector::merge(this->ship_names, other->ship_names);

	if (this->group != nullptr) {
		this->group->add_names_from(other);
	}
}

}
