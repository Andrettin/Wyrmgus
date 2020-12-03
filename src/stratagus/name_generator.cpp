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

#include "name_generator.h"

#include "gender.h"
#include "util/vector_util.h"

namespace wyrmgus {

void name_generator::propagate_ungendered_names(const std::map<gender, std::vector<std::string>> &source_name_map, std::map<gender, std::vector<std::string>> &target_name_map)
{
	const auto find_iterator = source_name_map.find(gender::none);
	if (find_iterator != source_name_map.end()) {
		vector::merge(target_name_map[gender::male], find_iterator->second);
		vector::merge(target_name_map[gender::female], find_iterator->second);
	}
}

const std::vector<std::string> &name_generator::get_specimen_names(const gender gender) const
{
	const auto find_iterator = this->specimen_names.find(gender);
	if (find_iterator != this->specimen_names.end()) {
		return find_iterator->second;
	}

	return vector::empty_string_vector;
}

void name_generator::add_specimen_names(const std::map<gender, std::vector<std::string>> &specimen_names)
{
	for (const auto &kv_pair : specimen_names) {
		vector::merge(this->specimen_names[kv_pair.first], kv_pair.second);
	}

	name_generator::propagate_ungendered_names(specimen_names, this->specimen_names);
}

const std::vector<std::string> &name_generator::get_personal_names(const gender gender) const
{
	const auto find_iterator = this->personal_names.find(gender);
	if (find_iterator != this->personal_names.end()) {
		return find_iterator->second;
	}

	return vector::empty_string_vector;
}

void name_generator::add_personal_names(const std::map<gender, std::vector<std::string>> &personal_names)
{
	for (const auto &kv_pair : personal_names) {
		vector::merge(this->personal_names[kv_pair.first], kv_pair.second);
	}

	name_generator::propagate_ungendered_names(personal_names, this->personal_names);
}

void name_generator::add_surnames(const std::vector<std::string> &surnames)
{
	vector::merge(this->surnames, surnames);
}

const std::vector<std::string> &name_generator::get_unit_class_names(const unit_class *unit_class) const
{
	const auto find_iterator = this->unit_class_names.find(unit_class);
	if (find_iterator != this->unit_class_names.end()) {
		return find_iterator->second;
	}

	return vector::empty_string_vector;
}

void name_generator::add_unit_class_names(const unit_class_map<std::vector<std::string>> &unit_class_names)
{
	for (const auto &kv_pair : unit_class_names) {
		vector::merge(this->unit_class_names[kv_pair.first], kv_pair.second);
	}
}

void name_generator::add_ship_names(const std::vector<std::string> &ship_names)
{
	vector::merge(this->ship_names, ship_names);
}

}
