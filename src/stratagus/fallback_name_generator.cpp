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

#include "fallback_name_generator.h"

#include "gender.h"
#include "name_generator.h"
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
	const auto find_iterator = this->specimen_name_generators.find(gender);
	if (find_iterator != this->specimen_name_generators.end()) {
		return find_iterator->second.get();
	}

	return nullptr;
}

void fallback_name_generator::add_specimen_names(const std::map<gender, std::unique_ptr<name_generator>> &specimen_names)
{
	for (const auto &kv_pair : specimen_names) {
		if (this->specimen_name_generators.find(kv_pair.first) == this->specimen_name_generators.end()) {
			this->specimen_name_generators[kv_pair.first] = std::make_unique<name_generator>();
		}

		this->specimen_name_generators[kv_pair.first]->add_names(kv_pair.second->get_names());
	}

	name_generator::propagate_ungendered_names(specimen_names, this->specimen_name_generators);
}

const name_generator *fallback_name_generator::get_personal_name_generator(const gender gender) const
{
	const auto find_iterator = this->personal_name_generators.find(gender);
	if (find_iterator != this->personal_name_generators.end()) {
		return find_iterator->second.get();
	}

	return nullptr;
}

void fallback_name_generator::add_personal_names(const std::map<gender, std::unique_ptr<name_generator>> &personal_names)
{
	for (const auto &kv_pair : personal_names) {
		if (this->personal_name_generators.find(kv_pair.first) == this->personal_name_generators.end()) {
			this->personal_name_generators[kv_pair.first] = std::make_unique<name_generator>();
		}

		this->personal_name_generators[kv_pair.first]->add_names(kv_pair.second->get_names());
	}

	name_generator::propagate_ungendered_names(personal_names, this->personal_name_generators);
}

void fallback_name_generator::add_surnames(const std::vector<std::string> &surnames)
{
	if (this->surname_generator == nullptr) {
		this->surname_generator = std::make_unique<name_generator>();
	}

	this->surname_generator->add_names(surnames);
}

const name_generator *fallback_name_generator::get_unit_class_name_generator(const unit_class *unit_class) const
{
	const auto find_iterator = this->unit_class_name_generators.find(unit_class);
	if (find_iterator != this->unit_class_name_generators.end()) {
		return find_iterator->second.get();
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
}

void fallback_name_generator::add_ship_names(const std::vector<std::string> &ship_names)
{
	if (this->ship_name_generator == nullptr) {
		this->ship_name_generator = std::make_unique<name_generator>();
	}

	this->ship_name_generator->add_names(ship_names);
}

}
