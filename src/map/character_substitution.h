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
