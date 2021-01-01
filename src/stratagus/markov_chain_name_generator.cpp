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

#include "markov_chain_name_generator.h"

#include "util/vector_util.h"
#include "util/vector_random_util.h"

namespace wyrmgus {

bool markov_chain_name_generator::is_name_valid(const std::string &name) const
{
	return vector::contains(this->names, name);
}

void markov_chain_name_generator::add_name(const std::string &name)
{
	for (size_t i = 0; i <= name.size(); ++i) {
		std::string prefix = this->get_letter_prefix(name, i);
		std::string suffix = this->get_letter_suffix(name, i);

		std::vector<std::string> &suffixes = this->letters_by_prefix[std::move(prefix)];

		if (!vector::contains(suffixes, suffix)) {
			//do not add duplicates, as the final generated name list would grow too large
			suffixes.push_back(std::move(suffix));
		}
	}

	if (this->min_name_size > name.size()) {
		this->min_name_size = name.size();
	}

	if (this->max_name_size < name.size()) {
		this->max_name_size = name.size();
	}
}

std::string markov_chain_name_generator::generate_name() const
{
	return vector::get_random(this->names);
}

void markov_chain_name_generator::generate_possible_names(const std::string &current_name)
{
	std::string prefix;
	if (current_name.size() < this->markov_chain_size) {
		prefix = current_name;
	} else {
		prefix = current_name.substr(current_name.size() - this->markov_chain_size, this->markov_chain_size);
	}

	const auto find_iterator = this->letters_by_prefix.find(prefix);
	if (find_iterator != this->letters_by_prefix.end()) {
		for (const std::string &suffix : find_iterator->second) {
			const std::string suffixed_name = current_name + suffix;

			if (suffixed_name.size() > (this->max_name_size + 2)) {
				//prevent infinite recursion; +2 because of the extra start and end characters
				continue;
			}

			this->generate_possible_names(suffixed_name);
		}
	} else {
		std::string name = current_name.substr(1, current_name.size() - 2); //cut off the start and end letters

		const bool name_valid = name.size() >= this->min_name_size && name.size() <= this->max_name_size;

		if (!name_valid) {
			return;
		}

		if (vector::contains(this->names, name)) {
			//do not add duplicates, as the generated name list would grow too large
			return;
		}

		this->names.push_back(std::move(name));
	}
}

}
