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

#include "database/sml_data.h"
#include "database/sml_operator.h"
#include "gender.h"
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
	} else if (tag == "unit_class_names") {
		scope.for_each_child([&](const sml_data &child_scope) {
			const std::string &tag = child_scope.get_tag();

			const unit_class *unit_class = unit_class::get(tag);
			vector::merge(this->unit_class_names[unit_class], child_scope.get_values());
		});
	} else if (tag == "surnames") {
		vector::merge(this->surnames, values);
	} else {
		data_entry::process_sml_scope(scope);
	}
}

const std::vector<std::string> &civilization_base::get_personal_names(const gender gender) const
{
	auto find_iterator = this->personal_names.find(gender);
	if (find_iterator != this->personal_names.end()) {
		return find_iterator->second;
	}

	return vector::empty_string_vector;
}

const std::vector<std::string> &civilization_base::get_unit_class_names(const unit_class *unit_class) const
{
	auto find_iterator = this->unit_class_names.find(unit_class);
	if (find_iterator != this->unit_class_names.end() && !find_iterator->second.empty()) {
		return find_iterator->second;
	}

	return vector::empty_string_vector;
}

QStringList civilization_base::get_ship_names_qstring_list() const
{
	return container::to_qstring_list(this->get_ship_names());
}

void civilization_base::remove_ship_name(const std::string &ship_name)
{
	vector::remove_one(this->ship_names, ship_name);
}

void civilization_base::add_names_from(const civilization_base *other)
{
	for (const auto &kv_pair : other->personal_names) {
		vector::merge(this->personal_names[kv_pair.first], kv_pair.second);
	}

	vector::merge(this->surnames, other->surnames);

	for (const auto &kv_pair : other->unit_class_names) {
		vector::merge(this->unit_class_names[kv_pair.first], kv_pair.second);
	}

	vector::merge(this->ship_names, other->ship_names);
}

}
