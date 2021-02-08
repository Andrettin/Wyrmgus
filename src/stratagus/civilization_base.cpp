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
//      (c) Copyright 2020-2021 by Andrettin
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

#include "civilization_base.h"

#include "civilization_group.h"
#include "civilization_history.h"
#include "database/sml_data.h"
#include "database/sml_operator.h"
#include "faction.h"
#include "fallback_name_generator.h"
#include "gender.h"
#include "name_generator.h"
#include "unit/unit_class.h"
#include "util/container_util.h"
#include "util/vector_util.h"

namespace wyrmgus {

civilization_base::civilization_base(const std::string &identifier) : detailed_data_entry(identifier)
{
}

civilization_base::~civilization_base()
{
}

void civilization_base::process_sml_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "personal_names") {
		if (!values.empty()) {
			if (this->personal_name_generators.find(gender::none) == this->personal_name_generators.end()) {
				this->personal_name_generators[gender::none] = std::make_unique<name_generator>();
			}

			this->personal_name_generators[gender::none]->add_names(values);
		}

		scope.for_each_child([&](const sml_data &child_scope) {
			const std::string &tag = child_scope.get_tag();

			const wyrmgus::gender gender = string_to_gender(tag);

			if (this->personal_name_generators.find(gender) == this->personal_name_generators.end()) {
				this->personal_name_generators[gender] = std::make_unique<name_generator>();
			}

			this->personal_name_generators[gender]->add_names(child_scope.get_values());
		});
	} else if (tag == "surnames") {
		if (this->surname_generator == nullptr) {
			this->surname_generator = std::make_unique<name_generator>();
		}

		this->surname_generator->add_names(values);
	} else if (tag == "unit_class_names") {
		scope.for_each_child([&](const sml_data &child_scope) {
			const std::string &tag = child_scope.get_tag();

			const unit_class *unit_class = unit_class::get(tag);

			if (this->unit_class_name_generators.find(unit_class) == this->unit_class_name_generators.end()) {
				this->unit_class_name_generators[unit_class] = std::make_unique<name_generator>();
			}

			this->unit_class_name_generators[unit_class]->add_names(child_scope.get_values());
		});
	} else if (tag == "ship_names") {
		if (this->ship_name_generator == nullptr) {
			this->ship_name_generator = std::make_unique<name_generator>();
		}

		this->ship_name_generator->add_names(values);
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

	fallback_name_generator::get()->add_personal_names(this->personal_name_generators);
	if (this->surname_generator != nullptr) {
		fallback_name_generator::get()->add_surnames(this->surname_generator->get_names());
	}
	fallback_name_generator::get()->add_unit_class_names(this->unit_class_name_generators);
	if (this->ship_name_generator != nullptr) {
		fallback_name_generator::get()->add_ship_names(this->ship_name_generator->get_names());
	}

	name_generator::propagate_ungendered_names(this->personal_name_generators);
	name_generator::propagate_unit_class_names(this->unit_class_name_generators, this->ship_name_generator);

	data_entry::initialize();
}

data_entry_history *civilization_base::get_history_base()
{
	return this->history.get();
}

void civilization_base::reset_history()
{
	this->history = std::make_unique<civilization_history>();
}

bool civilization_base::is_part_of_group(const civilization_group *group) const
{
	if (this->get_group() == nullptr) {
		return false;
	}

	if (this->get_group() == group) {
		return true;
	}

	//not the same group, and has a rank lesser than or equal to that of our group, so it can't be an upper group of ours
	if (group->get_rank() <= this->get_group()->get_rank()) {
		return false;
	}

	return this->get_group()->is_part_of_group(group);
}

const name_generator *civilization_base::get_personal_name_generator(const gender gender) const
{
	const auto find_iterator = this->personal_name_generators.find(gender);
	if (find_iterator != this->personal_name_generators.end()) {
		const size_t name_count = find_iterator->second->get_name_count();
		if (name_count >= name_generator::minimum_name_count) {
			return find_iterator->second.get();
		}
	}

	if (this->get_group() != nullptr) {
		return this->get_group()->get_personal_name_generator(gender);
	}

	if (find_iterator != this->personal_name_generators.end()) {
		return find_iterator->second.get();
	}

	return nullptr;
}

void civilization_base::add_personal_name(const gender gender, const std::string &name)
{
	if (this->personal_name_generators.find(gender) == this->personal_name_generators.end()) {
		this->personal_name_generators[gender] = std::make_unique<name_generator>();
	}

	this->personal_name_generators[gender]->add_name(name);

	if (gender == gender::none) {
		if (this->personal_name_generators.find(gender::male) == this->personal_name_generators.end()) {
			this->personal_name_generators[gender::male] = std::make_unique<name_generator>();
		}
		this->personal_name_generators[gender::male]->add_name(name);

		if (this->personal_name_generators.find(gender::female) == this->personal_name_generators.end()) {
			this->personal_name_generators[gender::female] = std::make_unique<name_generator>();
		}
		this->personal_name_generators[gender::female]->add_name(name);
	}

	if (this->group != nullptr) {
		this->group->add_personal_name(gender, name);
	}
}

const name_generator *civilization_base::get_surname_generator() const
{
	if (this->surname_generator != nullptr && this->surname_generator->get_name_count() >= name_generator::minimum_name_count) {
		return this->surname_generator.get();
	}

	if (this->get_group() != nullptr) {
		return this->get_group()->get_surname_generator();
	}

	return nullptr;
}

void civilization_base::add_surname(const std::string &surname)
{
	if (this->surname_generator == nullptr) {
		this->surname_generator = std::make_unique<name_generator>();
	}

	this->surname_generator->add_name(surname);

	if (this->group != nullptr) {
		this->group->add_surname(surname);
	}
}

const name_generator *civilization_base::get_unit_class_name_generator(const unit_class *unit_class) const
{
	const auto find_iterator = this->unit_class_name_generators.find(unit_class);
	if (find_iterator != this->unit_class_name_generators.end() && find_iterator->second->get_name_count() >= name_generator::minimum_name_count) {
		return find_iterator->second.get();
	}

	if (unit_class->is_ship() && this->ship_name_generator != nullptr && this->ship_name_generator->get_name_count() >= name_generator::minimum_name_count) {
		return this->ship_name_generator.get();
	}

	if (this->get_group() != nullptr) {
		return this->get_group()->get_unit_class_name_generator(unit_class);
	}

	return fallback_name_generator::get()->get_unit_class_name_generator(unit_class);
}

void civilization_base::add_unit_class_name(const unit_class *unit_class, const std::string &name)
{
	if (this->unit_class_name_generators.find(unit_class) == this->unit_class_name_generators.end()) {
		this->unit_class_name_generators[unit_class] = std::make_unique<name_generator>();
	}

	this->unit_class_name_generators[unit_class]->add_name(name);

	if (this->group != nullptr) {
		this->group->add_unit_class_name(unit_class, name);
	}
}

void civilization_base::add_ship_name(const std::string &ship_name)
{
	if (this->ship_name_generator == nullptr) {
		this->ship_name_generator = std::make_unique<name_generator>();
	}

	this->ship_name_generator->add_name(ship_name);

	if (this->group != nullptr) {
		this->group->add_ship_name(ship_name);
	}
}

void civilization_base::add_names_from(const civilization_base *other)
{
	for (const auto &kv_pair : other->personal_name_generators) {
		if (this->personal_name_generators.find(kv_pair.first) == this->personal_name_generators.end()) {
			this->personal_name_generators[kv_pair.first] = std::make_unique<name_generator>();
		}

		this->personal_name_generators[kv_pair.first]->add_names(kv_pair.second->get_names());
	}

	name_generator::propagate_ungendered_names(other->personal_name_generators, this->personal_name_generators);

	if (other->surname_generator != nullptr) {
		if (this->surname_generator == nullptr) {
			this->surname_generator = std::make_unique<name_generator>();
		}

		this->surname_generator->add_names(other->surname_generator->get_names());
	}

	for (const auto &kv_pair : other->unit_class_name_generators) {
		if (this->unit_class_name_generators.find(kv_pair.first) == this->unit_class_name_generators.end()) {
			this->unit_class_name_generators[kv_pair.first] = std::make_unique<name_generator>();
		}

		this->unit_class_name_generators[kv_pair.first]->add_names(kv_pair.second->get_names());
	}

	name_generator::propagate_unit_class_names(other->unit_class_name_generators, this->ship_name_generator);

	if (other->ship_name_generator != nullptr) {
		if (this->ship_name_generator == nullptr) {
			this->ship_name_generator = std::make_unique<name_generator>();
		}

		this->ship_name_generator->add_names(other->ship_name_generator->get_names());
	}

	if (this->group != nullptr) {
		this->group->add_names_from(other);
	}
}

void civilization_base::add_names_from(const faction *faction)
{
	for (const auto &kv_pair : faction->get_unit_class_name_generators()) {
		if (this->unit_class_name_generators.find(kv_pair.first) == this->unit_class_name_generators.end()) {
			this->unit_class_name_generators[kv_pair.first] = std::make_unique<name_generator>();
		}

		this->unit_class_name_generators[kv_pair.first]->add_names(kv_pair.second->get_names());
	}

	name_generator::propagate_unit_class_names(faction->get_unit_class_name_generators(), this->ship_name_generator);

	if (faction->get_ship_name_generator() != nullptr) {
		if (this->ship_name_generator == nullptr) {
			this->ship_name_generator = std::make_unique<name_generator>();
		}

		this->ship_name_generator->add_names(faction->get_ship_name_generator()->get_names());
	}

	if (this->group != nullptr) {
		this->group->add_names_from(faction);
	}
}

}
