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

#include "stratagus.h"

#include "name_generator.h"

#include "gender.h"
#include "unit/unit_class.h"
#include "util/vector_util.h"
#include "util/vector_random_util.h"

namespace wyrmgus {

void name_generator::propagate_ungendered_names(const std::map<gender, std::unique_ptr<name_generator>> &source_name_map, std::map<gender, std::unique_ptr<name_generator>> &target_name_map)
{
	const auto find_iterator = source_name_map.find(gender::none);
	if (find_iterator != source_name_map.end()) {
		if (target_name_map.find(gender::male) == target_name_map.end()) {
			target_name_map[gender::male] = std::make_unique<name_generator>();
		}
		target_name_map[gender::male]->add_names(find_iterator->second->get_names());

		if (target_name_map.find(gender::female) == target_name_map.end()) {
			target_name_map[gender::female] = std::make_unique<name_generator>();
		}
		target_name_map[gender::female]->add_names(find_iterator->second->get_names());
	}
}

void name_generator::propagate_unit_class_names(const unit_class_map<std::unique_ptr<name_generator>> &unit_class_name_generators, std::unique_ptr<name_generator> &ship_name_generator)
{
	for (const auto &kv_pair : unit_class_name_generators) {
		const unit_class *unit_class = kv_pair.first;

		if (unit_class->is_ship()) {
			if (ship_name_generator == nullptr) {
				ship_name_generator = std::make_unique<name_generator>();
			}

			ship_name_generator->add_names(kv_pair.second->get_names());
		}
	}
}

bool name_generator::is_name_valid(const std::string &name) const
{
	return vector::contains(this->names, name);
}

void name_generator::add_names(const std::vector<std::string> &names)
{
	vector::merge(this->names, names);
}

std::string name_generator::generate_name() const
{
	return vector::get_random(this->names);
}

}
