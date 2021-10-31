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

#pragma once

#include "text_processing_context.h"

class CPlayer;
class CUnit;
class CUpgrade;

namespace wyrmgus {

class civilization;
class faction;
class literary_text;
class named_data_entry;
class site;
class unit_class;
class unit_type;
class upgrade_class;
class word;

class text_processor final
{
public:
	explicit text_processor(text_processing_context &&ctx) : context(std::move(ctx))
	{
	}

	std::string process_text(std::string &&text, const bool process_in_game_data) const;
	std::string process_text(const std::string &text, const bool process_in_game_data) const;
	std::string process_tokens(std::queue<std::string> &&tokens, const bool process_in_game_data, bool &processed) const;
	std::string process_string_tokens(std::string &&str, std::queue<std::string> &&tokens) const;
	std::string process_named_data_entry_token(const named_data_entry *data_entry, const std::string &token) const;
	std::string process_named_data_entry_tokens(const named_data_entry *data_entry, std::queue<std::string> &tokens) const;
	std::string process_civilization_tokens(const civilization *civilization, std::queue<std::string> &tokens) const;
	std::string process_faction_tokens(const faction *faction, std::queue<std::string> &tokens) const;
	std::string process_literary_text_tokens(const literary_text *literary_text, std::queue<std::string> &tokens) const;
	std::string process_player_tokens(const CPlayer *player, std::queue<std::string> &tokens) const;
	std::string process_site_tokens(const site *site, std::queue<std::string> &tokens, const bool process_in_game_data, bool &processed) const;
	std::string process_unit_tokens(const CUnit *unit, std::queue<std::string> &tokens) const;
	std::string process_word_tokens(const word *word, std::queue<std::string> &tokens) const;
	std::string process_word_meaning_tokens(const word *word, std::queue<std::string> &tokens) const;

private:
	const text_processing_context context;
};

}
