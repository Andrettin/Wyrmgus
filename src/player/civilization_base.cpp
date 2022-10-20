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

#include "player/civilization_base.h"

#include "database/gsml_data.h"
#include "database/gsml_operator.h"
#include "fallback_name_generator.h"
#include "gendered_name_generator.h"
#include "name_generator.h"
#include "player/civilization_group.h"
#include "player/civilization_history.h"
#include "player/faction.h"
#include "sound/sound.h"
#include "ui/cursor.h"
#include "unit/unit_class.h"
#include "unit/unit_type.h"
#include "upgrade/upgrade_class.h"
#include "upgrade/upgrade_structs.h"
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/gender.h"
#include "util/vector_util.h"

namespace wyrmgus {

civilization_base::civilization_base(const std::string &identifier) : detailed_data_entry(identifier)
{
}

civilization_base::~civilization_base()
{
}

void civilization_base::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "class_unit_types") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			const unit_class *unit_class = unit_class::get(key);
			const unit_type *unit_type = unit_type::get(value);
			this->set_class_unit_type(unit_class, unit_type);
		});
	} else if (tag == "class_upgrades") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			const upgrade_class *upgrade_class = upgrade_class::get(key);
			const CUpgrade *upgrade = CUpgrade::get(value);
			this->set_class_upgrade(upgrade_class, upgrade);
		});
	} else if (tag == "unit_sounds") {
		if (this->unit_sound_set == nullptr) {
			this->unit_sound_set = std::make_unique<wyrmgus::unit_sound_set>();
		}

		database::process_gsml_data(this->unit_sound_set, scope);
	} else if (tag == "not_enough_resource_sounds") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			const resource *resource = resource::get(key);
			const sound *sound = sound::get(value);
			this->not_enough_resource_sounds[resource] = sound;
		});
	} else if (tag == "personal_names") {
		if (this->personal_name_generator == nullptr) {
			this->personal_name_generator = std::make_unique<gendered_name_generator>();
		}

		if (!values.empty()) {
			this->personal_name_generator->add_names(gender::none, values);
		}

		scope.for_each_child([&](const gsml_data &child_scope) {
			const std::string &tag = child_scope.get_tag();

			const gender gender = enum_converter<wyrmgus::gender>::to_enum(tag);

			this->personal_name_generator->add_names(gender, child_scope.get_values());
		});
	} else if (tag == "surnames") {
		if (this->surname_generator == nullptr) {
			this->surname_generator = std::make_unique<gendered_name_generator>();
		}

		if (!values.empty()) {
			this->surname_generator->add_names(gender::none, values);
		}

		scope.for_each_child([&](const gsml_data &child_scope) {
			const std::string &tag = child_scope.get_tag();

			const gender gender = enum_converter<wyrmgus::gender>::to_enum(tag);

			this->surname_generator->add_names(gender, child_scope.get_values());
		});
	} else if (tag == "unit_class_names") {
		scope.for_each_child([&](const gsml_data &child_scope) {
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
		data_entry::process_gsml_scope(scope);
	}
}

void civilization_base::initialize()
{
	if (this->unit_sound_set != nullptr) {
		this->unit_sound_set->map_sounds();
	}

	if (this->group != nullptr) {
		if (!this->group->is_initialized()) {
			this->group->initialize();
		}

		if (this->get_species() == nullptr) {
			this->set_species(this->get_group()->species);
		}

		this->group->add_names_from(this);
	}

	if (this->personal_name_generator != nullptr) {
		fallback_name_generator::get()->add_personal_names(this->personal_name_generator);
		this->personal_name_generator->propagate_ungendered_names();
	}

	if (this->surname_generator != nullptr) {
		fallback_name_generator::get()->add_surnames(this->surname_generator);
		this->surname_generator->propagate_ungendered_names();
	}

	fallback_name_generator::get()->add_unit_class_names(this->unit_class_name_generators);
	name_generator::propagate_unit_class_names(this->unit_class_name_generators, this->ship_name_generator);

	if (this->ship_name_generator != nullptr) {
		fallback_name_generator::get()->add_ship_names(this->ship_name_generator->get_names());
	}

	data_entry::initialize();
}

void civilization_base::check() const
{
	for (const auto &[unit_class, unit_type] : this->class_unit_types) {
		assert_throw(unit_type->get_unit_class() == unit_class);
	}

	for (const auto &[upgrade_class, upgrade] : this->class_upgrades) {
		assert_throw(upgrade->get_upgrade_class() == upgrade_class);
	}

	data_entry::check();
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

const wyrmgus::unit_sound_set *civilization_base::get_unit_sound_set() const
{
	if (this->unit_sound_set != nullptr) {
		return this->unit_sound_set.get();
	}

	if (this->get_group() != nullptr) {
		return this->get_group()->get_unit_sound_set();
	}

	return nullptr;
}

const sound *civilization_base::get_help_town_sound() const
{
	if (this->help_town_sound != nullptr) {
		return this->help_town_sound;
	}

	if (this->get_group() != nullptr) {
		return this->get_group()->get_help_town_sound();
	}

	return nullptr;
}

const sound *civilization_base::get_work_complete_sound() const
{
	if (this->work_complete_sound != nullptr) {
		return this->work_complete_sound;
	}

	if (this->get_group() != nullptr) {
		return this->get_group()->get_work_complete_sound();
	}

	return nullptr;
}

const sound *civilization_base::get_research_complete_sound() const
{
	if (this->research_complete_sound != nullptr) {
		return this->research_complete_sound;
	}

	if (this->work_complete_sound != nullptr) {
		return this->work_complete_sound;
	}

	if (this->get_group() != nullptr) {
		return this->get_group()->get_research_complete_sound();
	}

	return nullptr;
}

const sound *civilization_base::get_not_enough_food_sound() const
{
	if (this->not_enough_food_sound != nullptr) {
		return this->not_enough_food_sound;
	}

	if (this->get_group() != nullptr) {
		return this->get_group()->get_not_enough_food_sound();
	}

	return nullptr;
}

const sound *civilization_base::get_not_enough_resource_sound(const resource *resource) const
{
	const auto find_iterator = this->not_enough_resource_sounds.find(resource);
	if (find_iterator != this->not_enough_resource_sounds.end()) {
		return find_iterator->second;
	}

	if (this->get_group() != nullptr) {
		return this->get_group()->get_not_enough_resource_sound(resource);
	}

	return nullptr;
}

cursor *civilization_base::get_cursor(const cursor_type type) const
{
	const auto find_iterator = this->cursors.find(type);
	if (find_iterator != this->cursors.end()) {
		return find_iterator->second;
	}

	if (this->get_group() != nullptr) {
		return this->get_group()->get_cursor(type);
	}

	return nullptr;
}

const unit_type *civilization_base::get_class_unit_type(const unit_class *unit_class) const
{
	if (unit_class == nullptr) {
		return nullptr;
	}

	const auto find_iterator = this->class_unit_types.find(unit_class);
	if (find_iterator != this->class_unit_types.end()) {
		return find_iterator->second;
	}

	if (this->get_group() != nullptr) {
		return this->get_group()->get_class_unit_type(unit_class);
	}

	return nullptr;
}

const CUpgrade *civilization_base::get_class_upgrade(const upgrade_class *upgrade_class) const
{
	if (upgrade_class == nullptr) {
		return nullptr;
	}

	const auto find_iterator = this->class_upgrades.find(upgrade_class);
	if (find_iterator != this->class_upgrades.end()) {
		return find_iterator->second;
	}

	if (this->get_group() != nullptr) {
		return this->get_group()->get_class_upgrade(upgrade_class);
	}

	return nullptr;
}

const population_type *civilization_base::get_class_population_type(const population_class *population_class) const
{
	if (population_class == nullptr) {
		return nullptr;
	}

	const auto find_iterator = this->class_population_types.find(population_class);
	if (find_iterator != this->class_population_types.end()) {
		return find_iterator->second;
	}

	if (this->get_group() != nullptr) {
		return this->get_group()->get_class_population_type(population_class);
	}

	return nullptr;
}

const name_generator *civilization_base::get_personal_name_generator(const gender gender) const
{
	const name_generator *name_generator = nullptr;

	if (this->personal_name_generator != nullptr) {
		name_generator = this->personal_name_generator->get_name_generator(gender);
	}

	if (name_generator != nullptr) {
		const size_t name_count = name_generator->get_name_count();
		if (name_count >= name_generator::minimum_name_count) {
			return name_generator;
		}
	}

	if (this->get_group() != nullptr) {
		return this->get_group()->get_personal_name_generator(gender);
	}

	return name_generator;
}

void civilization_base::add_personal_name(const gender gender, const name_variant &name)
{
	if (this->personal_name_generator == nullptr) {
		this->personal_name_generator = std::make_unique<gendered_name_generator>();
	}

	this->personal_name_generator->add_name(gender, name);

	if (gender == gender::none) {
		this->personal_name_generator->add_name(gender::male, name);
		this->personal_name_generator->add_name(gender::female, name);
	}

	if (this->group != nullptr) {
		this->group->add_personal_name(gender, name);
	}
}

const name_generator *civilization_base::get_surname_generator(const gender gender) const
{
	const name_generator *name_generator = nullptr;

	if (this->surname_generator != nullptr) {
		name_generator = this->surname_generator->get_name_generator(gender);
	}

	if (name_generator != nullptr) {
		const size_t name_count = name_generator->get_name_count();
		if (name_count >= name_generator::minimum_name_count) {
			return name_generator;
		}
	}

	if (this->get_group() != nullptr) {
		return this->get_group()->get_surname_generator(gender);
	}

	return name_generator;
}

void civilization_base::add_surname(const gender gender, const name_variant &surname)
{
	if (this->surname_generator == nullptr) {
		this->surname_generator = std::make_unique<gendered_name_generator>();
	}

	this->surname_generator->add_name(gender, surname);

	if (gender == gender::none) {
		this->surname_generator->add_name(gender::male, surname);
		this->surname_generator->add_name(gender::female, surname);
	}

	if (this->group != nullptr) {
		this->group->add_surname(gender, surname);
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

void civilization_base::add_unit_class_name(const unit_class *unit_class, const name_variant &name)
{
	if (this->unit_class_name_generators.find(unit_class) == this->unit_class_name_generators.end()) {
		this->unit_class_name_generators[unit_class] = std::make_unique<name_generator>();
	}

	this->unit_class_name_generators[unit_class]->add_name(name);

	if (this->group != nullptr) {
		this->group->add_unit_class_name(unit_class, name);
	}
}

void civilization_base::add_ship_name(const name_variant &ship_name)
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
	if (other->personal_name_generator != nullptr) {
		if (this->personal_name_generator == nullptr) {
			this->personal_name_generator = std::make_unique<gendered_name_generator>();
		}

		this->personal_name_generator->add_names_from(other->personal_name_generator);
	}

	if (other->surname_generator != nullptr) {
		if (this->surname_generator == nullptr) {
			this->surname_generator = std::make_unique<gendered_name_generator>();
		}

		this->surname_generator->add_names_from(other->surname_generator);
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
