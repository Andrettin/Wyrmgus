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

#include "fallback_name_generator.h"

#include "gendered_name_generator.h"
#include "name_generator.h"
#include "unit/unit_class.h"
#include "util/gender.h"
#include "util/vector_util.h"

namespace wyrmgus {

fallback_name_generator::fallback_name_generator()
{
}

fallback_name_generator::~fallback_name_generator()
{
}

const name_generator *fallback_name_generator::get_specimen_name_generator(const gender gender) const
{
	if (this->specimen_name_generator != nullptr) {
		return this->specimen_name_generator->get_name_generator(gender);
	}

	return nullptr;
}

void fallback_name_generator::add_specimen_names(const std::unique_ptr<gendered_name_generator> &source_name_generator)
{
	if (this->specimen_name_generator == nullptr) {
		this->specimen_name_generator = std::make_unique<gendered_name_generator>();
	}

	this->specimen_name_generator->add_names_from(source_name_generator);
}

const name_generator *fallback_name_generator::get_personal_name_generator(const gender gender) const
{
	if (this->personal_name_generator != nullptr) {
		return this->personal_name_generator->get_name_generator(gender);
	}

	return nullptr;
}

void fallback_name_generator::add_personal_names(const std::unique_ptr<gendered_name_generator> &source_name_generator)
{
	if (this->personal_name_generator == nullptr) {
		this->personal_name_generator = std::make_unique<gendered_name_generator>();
	}

	this->personal_name_generator->add_names_from(source_name_generator);
}

const name_generator *fallback_name_generator::get_surname_generator(const gender gender) const
{
	if (this->surname_generator != nullptr) {
		return this->surname_generator->get_name_generator(gender);
	}

	return nullptr;
}

void fallback_name_generator::add_surnames(const std::unique_ptr<gendered_name_generator> &source_name_generator)
{
	if (this->surname_generator == nullptr) {
		this->surname_generator = std::make_unique<gendered_name_generator>();
	}

	this->surname_generator->add_names_from(source_name_generator);
}

const name_generator *fallback_name_generator::get_unit_class_name_generator(const unit_class *unit_class) const
{
	const auto find_iterator = this->unit_class_name_generators.find(unit_class);
	if (find_iterator != this->unit_class_name_generators.end()) {
		return find_iterator->second.get();
	}

	if (unit_class->is_ship() && this->ship_name_generator != nullptr) {
		return this->ship_name_generator.get();
	}

	return nullptr;
}

void fallback_name_generator::add_unit_class_names(const unit_class_map<std::unique_ptr<name_generator>> &unit_class_names)
{
	for (const auto &kv_pair : unit_class_names) {
		if (this->unit_class_name_generators.find(kv_pair.first) == this->unit_class_name_generators.end()) {
			this->unit_class_name_generators[kv_pair.first] = std::make_unique<name_generator>();
		}

		this->unit_class_name_generators[kv_pair.first]->add_names(kv_pair.second->get_names());
	}

	name_generator::propagate_unit_class_names(unit_class_names, this->ship_name_generator);
}

void fallback_name_generator::add_ship_names(const std::vector<name_variant> &ship_names)
{
	if (this->ship_name_generator == nullptr) {
		this->ship_name_generator = std::make_unique<name_generator>();
	}

	this->ship_name_generator->add_names(ship_names);
}

}
