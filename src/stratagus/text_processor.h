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

#include "text_processing_context.h"
#include "util/text_processor_base.h"

class CPlayer;
class CUnit;
class CUpgrade;

namespace wyrmgus {

class civilization;
class dialogue_node;
class dialogue_option;
class faction;
class literary_text;
class site;
class unit_class;
class unit_type;
class upgrade_class;

class text_processor final : public text_processor_base
{
public:
	explicit text_processor(text_processing_context &&ctx) : context(std::move(ctx))
	{
	}

	virtual std::string process_tokens(std::queue<std::string> &&tokens, const bool process_in_game_data, bool &processed) const override;
	std::string process_civilization_tokens(const civilization *civilization, std::queue<std::string> &tokens) const;
	std::string process_dialogue_node_tokens(const dialogue_node *dialogue_node, std::queue<std::string> &tokens) const;
	std::string process_dialogue_option_tokens(const dialogue_option *dialogue_option, std::queue<std::string> &tokens) const;
	std::string process_faction_tokens(const faction *faction, std::queue<std::string> &tokens) const;
	std::string process_literary_text_tokens(const literary_text *literary_text, std::queue<std::string> &tokens) const;
	std::string process_player_tokens(const CPlayer *player, std::queue<std::string> &tokens) const;
	std::string process_site_tokens(const site *site, std::queue<std::string> &tokens, const bool process_in_game_data, bool &processed) const;
	std::string process_unit_tokens(const CUnit *unit, std::queue<std::string> &tokens) const;

private:
	const text_processing_context context;
};

}
