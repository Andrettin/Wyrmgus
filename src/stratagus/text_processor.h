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

class civilization;
class faction;
class unit_class;
class unit_type;
class word;

class text_processor final
{
public:
	explicit text_processor(const faction *faction) : faction(faction)
	{
	}

	std::string process_text(std::string &&text) const;
	std::string process_text(const std::string &text) const;
	std::string process_tokens(std::queue<std::string> &&tokens) const;
	std::string process_string_tokens(std::string &&str, std::queue<std::string> &&tokens) const;
	std::string process_civilization_tokens(const civilization *civilization, std::queue<std::string> &tokens) const;
	std::string process_faction_tokens(const faction *faction, std::queue<std::string> &tokens) const;
	std::string process_unit_class_tokens(const unit_class *unit_class, std::queue<std::string> &tokens) const;
	std::string process_unit_type_tokens(const unit_type *unit_type, std::queue<std::string> &tokens) const;
	std::string process_word_tokens(const word *word, std::queue<std::string> &tokens) const;
	std::string process_word_meaning_tokens(const word *word, std::queue<std::string> &tokens) const;

private:
	const wyrmgus::faction *faction = nullptr;
};

}
