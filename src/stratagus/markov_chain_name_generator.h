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

#pragma once

namespace wyrmgus {

class markov_chain_name_generator final
{
public:
	static constexpr size_t default_chain_size = 2;
	static inline const std::string start_letter = "(";
	static inline const std::string end_letter = ")";

	explicit markov_chain_name_generator(const size_t markov_chain_size) : markov_chain_size(markov_chain_size)
	{
	}

	bool is_initialized() const
	{
		return this->initialized;
	}

	void initialize()
	{
		this->generate_possible_names();
		this->initialized = true;
	}

	size_t get_name_count() const
	{
		return this->names.size();
	}

	std::string get_letter_prefix(const std::string &name, const size_t letter_index) const
	{
		if (letter_index < this->markov_chain_size) {
			if (letter_index == 0) {
				return markov_chain_name_generator::start_letter;
			} else {
				return markov_chain_name_generator::start_letter + name.substr(0, letter_index);
			}
		} else {
			return name.substr(letter_index - this->markov_chain_size, this->markov_chain_size);
		}
	}

	std::string get_letter_suffix(const std::string &name, const size_t letter_index) const
	{
		if (letter_index == name.size()) {
			return markov_chain_name_generator::end_letter;
		} else {
			return name.substr(letter_index, 1);
		}
	}

	bool is_name_valid(const std::string &name) const;

	void add_name(const std::string &name);

	std::string generate_name() const;

	void generate_possible_names()
	{
		//generate all possible names
		this->generate_possible_names(markov_chain_name_generator::start_letter);
	}

	void generate_possible_names(const std::string &current_name);

private:
	bool initialized = false;
	const size_t markov_chain_size = 0;
	std::map<std::string, std::vector<std::string>> letters_by_prefix; //letters mapped to their markov chain prefix
	size_t min_name_size = 0; //the minimum name size to be generated, based on the size of the smallest input name
	size_t max_name_size = 0; //the maximum name size to be generated, based on the size of the largest input name
	std::vector<std::string> names; //the possible names, pre-generated
};

}
