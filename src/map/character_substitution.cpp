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

#include "map/character_substitution.h"

#include "database/sml_data.h"
#include "database/sml_property.h"
#include "util/container_util.h"
#include "util/exception_util.h"
#include "util/queue_util.h"
#include "util/string_conversion_util.h"
#include "util/vector_random_util.h"
#include "util/vector_util.h"

namespace wyrmgus {

void character_substitution::process_sml_property(const sml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "source_character") {
		this->source_characters.clear();
		this->source_characters.push_back(string::to_character(value));
	} else if (key == "target_character") {
		this->target_characters.clear();
		this->target_characters.push_back(string::to_character(value));
	} else {
		exception::throw_with_trace(std::runtime_error("Invalid character substitution property: \"" + key + "\"."));
	}
}

void character_substitution::process_sml_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "source_characters") {
		for (const std::string &value : values) {
			this->source_characters.push_back(string::to_character(value));
		}
	} else if (tag == "target_characters") {
		for (const std::string &value : values) {
			this->target_characters.push_back(string::to_character(value));
		}

		scope.for_each_property([&](const sml_property &property) {
			const char c = string::to_character(property.get_key());
			const int weight = std::stoi(property.get_value());

			for (int i = 0; i < weight; ++i) {
				this->target_characters.push_back(c);
			}
		});
	} else if (tag == "shuffle_character_sets") {
		for (const std::string &value : values) {
			this->shuffle_character_sets.push_back({ string::to_character(value) });
		}

		scope.for_each_child([&](const sml_data &child_scope) {
			std::vector<char> character_set;

			for (const std::string &value : child_scope.get_values()) {
				character_set.push_back(string::to_character(value));
			}

			this->shuffle_character_sets.push_back(std::move(character_set));
		});
	} else {
		exception::throw_with_trace(std::runtime_error("Invalid character substitution scope: \"" + tag + "\"."));
	}
}

void character_substitution::apply_to_map(character_map &map) const
{
	std::vector<std::vector<char>> shuffled_character_sets;

	vector::process_randomly(this->shuffle_character_sets, [&shuffled_character_sets](std::vector<char> &&character_set) {
		shuffled_character_sets.push_back(std::move(character_set));
	});

	for (int y = 0; y < static_cast<int>(map.size()); ++y) {
		std::vector<char> &row = map[y];

		for (int x = 0; x < static_cast<int>(row.size()); ++x) {
			char &character = row.at(x);

			if (vector::contains(this->source_characters, character)) {
				character = vector::get_random(this->target_characters);
				continue;
			}

			for (size_t i = 0; i < this->shuffle_character_sets.size(); ++i) {
				const std::vector<char> &shuffle_character_set = this->shuffle_character_sets[i];
				const std::optional<size_t> character_index = vector::find_index(shuffle_character_set, character);
				if (!character_index.has_value()) {
					continue;
				}

				character = shuffled_character_sets[i][character_index.value()];
			}
		}
	}
}

}
