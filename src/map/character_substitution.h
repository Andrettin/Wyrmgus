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
//      Permission is hereby granted, free of charge, to any person obtaining a
//      copy of this software and associated documentation files (the
//      "Software"), to deal in the Software without restriction, including
//      without limitation the rights to use, copy, modify, merge, publish,
//      distribute, sublicense, and/or sell copies of the Software, and to
//      permit persons to whom the Software is furnished to do so, subject to
//      the following conditions:
//
//      The above copyright notice and this permission notice shall be included
//      in all copies or substantial portions of the Software.
//
//      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
//      OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
//      MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
//      IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
//      CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
//      TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
//      SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#pragma once

namespace wyrmgus {

class sml_data;
class sml_property;

class character_substitution final
{
public:
	using character_map = std::vector<std::vector<char>>;

	void process_sml_property(const sml_property &property);
	void process_sml_scope(const sml_data &scope);

	void check() const
	{
		if (!this->shuffle_character_sets.empty()) {
			const size_t front_set_size = this->shuffle_character_sets.front().size();
			for (size_t i = 1; i < this->shuffle_character_sets.size(); ++i) {
				const std::vector<char> &character_set = this->shuffle_character_sets[i];

				if (character_set.size() != front_set_size) {
					throw std::runtime_error("Shuffle character set at index " + std::to_string(i) + " has a different size (" + std::to_string(character_set.size()) + ") than the front set (" + std::to_string(front_set_size) + ").");
				}
			}
		}
	}

	void apply_to_map(character_map &map) const;

private:
	std::vector<char> source_characters;
	std::vector<char> target_characters;

	//characters to be shuffled, i.e. replaced with another character with the same index in another set
	std::vector<std::vector<char>> shuffle_character_sets;
};

}
