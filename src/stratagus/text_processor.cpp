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

#include "stratagus.h"

#include "text_processor.h"

#include "dialogue_node.h"
#include "dialogue_option.h"
#include "language/word.h"
#include "literary_text.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "player/civilization.h"
#include "player/faction.h"
#include "player/player.h"
#include "quest/quest.h"
#include "unit/unit.h"
#include "unit/unit_class.h"
#include "unit/unit_ref.h"
#include "unit/unit_type.h"
#include "upgrade/upgrade_class.h"
#include "upgrade/upgrade_structs.h"
#include "util/assert_util.h"
#include "util/gender.h"
#include "util/queue_util.h"
#include "util/string_util.h"

namespace wyrmgus {


std::string text_processor::process_tokens(std::queue<std::string> &&tokens, const bool process_in_game_data, bool &processed) const
{
	processed = true;

	if (tokens.empty()) {
		return std::string();
	}

	const std::string token = queue::take(tokens);

	std::queue<std::string> subtokens = string::split_to_queue(token, ':');

	if (subtokens.size() > 2) {
		throw std::runtime_error("There can only be at most 2 subtokens.");
	}

	const std::string front_subtoken = queue::take(subtokens);

	std::string str;

	if (front_subtoken == "civilization") {
		const wyrmgus::civilization *civilization = nullptr;
		if (!subtokens.empty()) {
			civilization = civilization::get(queue::take(subtokens));
		}

		str = this->process_civilization_tokens(civilization, tokens);
	} else if (front_subtoken == "dialogue_node") {
		if (!process_in_game_data) {
			processed = false;
			return std::string();
		}

		str = this->process_dialogue_node_tokens(this->context.dialogue_node, tokens);
	} else if (front_subtoken == "faction") {
		const wyrmgus::faction *faction = this->context.faction;
		if (!subtokens.empty()) {
			faction = faction::get(queue::take(subtokens));
		}

		str = this->process_faction_tokens(faction, tokens);
	} else if (front_subtoken == "literary_text") {
		const literary_text *literary_text = nullptr;
		if (!subtokens.empty()) {
			literary_text = literary_text::get(queue::take(subtokens));
		}

		str = this->process_literary_text_tokens(literary_text, tokens);
	} else if (front_subtoken == "player") {
		if (!process_in_game_data) {
			processed = false;
			return std::string();
		}

		str = this->process_player_tokens(this->context.script_context.current_player, tokens);
	} else if (front_subtoken == "site") {
		const site *site = nullptr;
		if (!subtokens.empty()) {
			site = site::get(queue::take(subtokens));
		}

		str = this->process_site_tokens(site, tokens, process_in_game_data, processed);
	} else if (front_subtoken == "source_player") {
		if (!process_in_game_data) {
			processed = false;
			return std::string();
		}

		str = this->process_player_tokens(this->context.script_context.source_player, tokens);
	} else if (front_subtoken == "source_unit") {
		if (!process_in_game_data) {
			processed = false;
			return std::string();
		}

		if (this->context.script_context.source_unit == nullptr) {
			throw std::runtime_error("No source unit provided when processing source unit tokens.");
		}

		str = this->process_unit_tokens(this->context.script_context.source_unit->get(), tokens);
	} else if (front_subtoken == "unit") {
		if (!process_in_game_data) {
			processed = false;
			return std::string();
		}

		if (this->context.script_context.current_unit == nullptr) {
			throw std::runtime_error("No current unit provided when processing current unit tokens.");
		}

		str = this->process_unit_tokens(this->context.script_context.current_unit->get(), tokens);
	} else if (front_subtoken == "unit_class") {
		const wyrmgus::unit_class *unit_class = nullptr;
		if (!subtokens.empty()) {
			unit_class = unit_class::get(queue::take(subtokens));
		}

		str = this->process_named_data_entry_tokens(unit_class, tokens);
	} else if (front_subtoken == "unit_type") {
		const wyrmgus::unit_type *unit_type = nullptr;
		if (!subtokens.empty()) {
			unit_type = unit_type::get(queue::take(subtokens));
		}

		str = this->process_named_data_entry_tokens(unit_type, tokens);
	} else if (front_subtoken == "upgrade") {
		const CUpgrade *upgrade = nullptr;
		if (!subtokens.empty()) {
			upgrade = CUpgrade::get(queue::take(subtokens));
		}

		str = this->process_named_data_entry_tokens(upgrade, tokens);
	} else if (front_subtoken == "upgrade_class") {
		const wyrmgus::upgrade_class *upgrade_class = nullptr;
		if (!subtokens.empty()) {
			upgrade_class = upgrade_class::get(queue::take(subtokens));
		}

		str = this->process_named_data_entry_tokens(upgrade_class, tokens);
	} else if (front_subtoken == "word") {
		const wyrmgus::word *word = nullptr;
		if (!subtokens.empty()) {
			word = word::get(queue::take(subtokens));
		}

		str = this->process_word_tokens(word, tokens);
	} else {
		throw std::runtime_error("Failed to process token \"" + token + "\".");
	}

	if (!tokens.empty()) {
		return this->process_string_tokens(std::move(str), std::move(tokens));
	}

	return str;
}

std::string text_processor::process_civilization_tokens(const civilization *civilization, std::queue<std::string> &tokens) const
{
	if (civilization == nullptr) {
		throw std::runtime_error("No civilization provided when processing civilization tokens.");
	}

	if (tokens.empty()) {
		throw std::runtime_error("No tokens provided when processing civilization tokens.");
	}

	const std::string token = queue::take(tokens);

	std::queue<std::string> subtokens = string::split_to_queue(token, ':');

	if (subtokens.size() > 2) {
		throw std::runtime_error("There can only be at most 2 subtokens.");
	}

	const std::string front_subtoken = queue::take(subtokens);

	if (front_subtoken == "class_unit_type") {
		if (subtokens.empty()) {
			throw std::runtime_error("No unit class specified for the civilization \"class_unit_type\" token.");
		}

		const unit_class *unit_class = unit_class::get(queue::take(subtokens));
		const unit_type *unit_type = civilization->get_class_unit_type(unit_class);

		return this->process_named_data_entry_tokens(unit_type, tokens);
	} else if (front_subtoken == "class_upgrade") {
		if (subtokens.empty()) {
			throw std::runtime_error("No unit class specified for the civilization \"class_upgrade\" token.");
		}

		const upgrade_class *upgrade_class = upgrade_class::get(queue::take(subtokens));
		const CUpgrade *upgrade = civilization->get_class_upgrade(upgrade_class);

		return this->process_named_data_entry_tokens(upgrade, tokens);
	} else {
		return this->process_named_data_entry_token(civilization, front_subtoken);
	}
}

std::string text_processor::process_dialogue_node_tokens(const dialogue_node *dialogue_node, std::queue<std::string> &tokens) const
{
	if (dialogue_node == nullptr) {
		throw std::runtime_error("No dialogue node provided when processing dialogue node tokens.");
	}

	if (tokens.empty()) {
		throw std::runtime_error("No tokens provided when processing dialogue node tokens.");
	}

	const std::string token = queue::take(tokens);

	std::queue<std::string> subtokens = string::split_to_queue(token, ':');

	if (subtokens.size() > 2) {
		throw std::runtime_error("There can only be at most 2 subtokens.");
	}

	const std::string front_subtoken = queue::take(subtokens);

	static const std::string option_token_start = "option_";
	if (front_subtoken.starts_with(option_token_start)) {
		const std::string option_index_str = front_subtoken.substr(option_token_start.size());
		const int option_index = std::stoi(option_index_str) - 1;

		const dialogue_option *dialogue_option = dialogue_node->get_option(option_index);

		return this->process_dialogue_option_tokens(dialogue_option, tokens);
	}

	throw std::runtime_error("Failed to process dialogue node token \"" + front_subtoken + "\".");
}

std::string text_processor::process_dialogue_option_tokens(const dialogue_option *dialogue_option, std::queue<std::string> &tokens) const
{
	if (dialogue_option == nullptr) {
		throw std::runtime_error("No dialogue option provided when processing dialogue option tokens.");
	}

	if (tokens.empty()) {
		throw std::runtime_error("No tokens provided when processing dialogue option tokens.");
	}

	const std::string token = queue::take(tokens);

	std::queue<std::string> subtokens = string::split_to_queue(token, ':');

	if (subtokens.size() > 2) {
		throw std::runtime_error("There can only be at most 2 subtokens.");
	}

	const std::string front_subtoken = queue::take(subtokens);

	if (front_subtoken == "effects") {
		return dialogue_option->get_effects_string(this->context.script_context);
	}

	throw std::runtime_error("Failed to process dialogue option token \"" + front_subtoken + "\".");
}

std::string text_processor::process_faction_tokens(const wyrmgus::faction *faction, std::queue<std::string> &tokens) const
{
	if (faction == nullptr) {
		throw std::runtime_error("No faction provided when processing faction tokens.");
	}

	if (tokens.empty()) {
		throw std::runtime_error("No tokens provided when processing faction tokens.");
	}

	const std::string token = queue::take(tokens);

	std::queue<std::string> subtokens = string::split_to_queue(token, ':');

	if (subtokens.size() > 2) {
		throw std::runtime_error("There can only be at most 2 subtokens.");
	}

	const std::string front_subtoken = queue::take(subtokens);

	if (front_subtoken == "adjective") {
		return faction->get_adjective();
	} else if (front_subtoken == "class_unit_type") {
		if (subtokens.empty()) {
			throw std::runtime_error("No unit class specified for the faction \"class_unit_type\" token.");
		}

		const unit_class *unit_class = unit_class::get(queue::take(subtokens));
		const unit_type *unit_type = faction->get_class_unit_type(unit_class);

		return this->process_named_data_entry_tokens(unit_type, tokens);
	} else if (front_subtoken == "class_upgrade") {
		if (subtokens.empty()) {
			throw std::runtime_error("No unit class specified for the civilization \"class_upgrade\" token.");
		}

		const upgrade_class *upgrade_class = upgrade_class::get(queue::take(subtokens));
		const CUpgrade *upgrade = faction->get_class_upgrade(upgrade_class);

		return this->process_named_data_entry_tokens(upgrade, tokens);
	} else if (front_subtoken == "titled_name") {
		return faction->get_default_titled_name();
	} else {
		return this->process_named_data_entry_token(faction, front_subtoken);
	}
}

std::string text_processor::process_literary_text_tokens(const literary_text *literary_text, std::queue<std::string> &tokens) const
{
	if (literary_text == nullptr) {
		throw std::runtime_error("No literary text provided when processing literary text tokens.");
	}

	if (tokens.empty()) {
		throw std::runtime_error("No tokens provided when processing literary text tokens.");
	}

	const std::string token = queue::take(tokens);

	std::queue<std::string> subtokens = string::split_to_queue(token, ':');

	if (subtokens.size() > 2) {
		throw std::runtime_error("There can only be at most 2 subtokens.");
	}

	const std::string front_subtoken = queue::take(subtokens);

	if (front_subtoken == "link") {
		return literary_text->get_link_string();
	} else {
		return this->process_named_data_entry_token(literary_text, front_subtoken);
	}
}

std::string text_processor::process_player_tokens(const CPlayer *player, std::queue<std::string> &tokens) const
{
	if (player == nullptr) {
		throw std::runtime_error("No player provided when processing player tokens.");
	}

	if (tokens.empty()) {
		throw std::runtime_error("No tokens provided when processing player tokens.");
	}

	const std::string token = queue::take(tokens);

	std::queue<std::string> subtokens = string::split_to_queue(token, ':');

	if (subtokens.size() > 2) {
		throw std::runtime_error("There can only be at most 2 subtokens.");
	}

	const std::string front_subtoken = queue::take(subtokens);

	if (front_subtoken == "class_unit_type") {
		if (subtokens.empty()) {
			throw std::runtime_error("No unit class specified for the player \"class_unit_type\" token.");
		}

		const unit_class *unit_class = unit_class::get(queue::take(subtokens));
		const unit_type *unit_type = player->get_class_unit_type(unit_class);

		return this->process_named_data_entry_tokens(unit_type, tokens);
	} else if (front_subtoken == "faction") {
		return this->process_faction_tokens(player->get_faction(), tokens);
	} else if (front_subtoken == "last_created_unit") {
		return this->process_unit_tokens(player->get_last_created_unit(), tokens);
	} else if (front_subtoken == "quest_text") {
		if (subtokens.empty()) {
			throw std::runtime_error("No quest specified for the player \"quest_text\" token.");
		}

		const quest *quest = quest::get(queue::take(subtokens));

		return player->get_quest_text(quest);
	}

	throw std::runtime_error("Failed to process player token \"" + front_subtoken + "\".");
}

std::string text_processor::process_site_tokens(const wyrmgus::site *site, std::queue<std::string> &tokens, const bool process_in_game_data, bool &processed) const
{
	if (site == nullptr) {
		throw std::runtime_error("No site provided when processing site tokens.");
	}

	if (tokens.empty()) {
		throw std::runtime_error("No tokens provided when processing site tokens.");
	}

	const std::string token = queue::take(tokens);

	std::queue<std::string> subtokens = string::split_to_queue(token, ':');

	if (subtokens.size() > 2) {
		throw std::runtime_error("There can only be at most 2 subtokens.");
	}

	const std::string front_subtoken = queue::take(subtokens);

	if (front_subtoken == "current_cultural_name") {
		if (!process_in_game_data) {
			processed = false;
			return std::string();
		}

		return site->get_game_data()->get_current_cultural_name();
	} else {
		return this->process_named_data_entry_token(site, front_subtoken);
	}
}

std::string text_processor::process_unit_tokens(const CUnit *unit, std::queue<std::string> &tokens) const
{
	if (unit == nullptr) {
		throw std::runtime_error("No unit provided when processing unit tokens.");
	}

	if (tokens.empty()) {
		throw std::runtime_error("No tokens provided when processing unit tokens.");
	}

	const std::string token = queue::take(tokens);

	std::queue<std::string> subtokens = string::split_to_queue(token, ':');

	if (subtokens.size() > 2) {
		throw std::runtime_error("There can only be at most 2 subtokens.");
	}

	const std::string front_subtoken = queue::take(subtokens);

	if (front_subtoken == "name") {
		return unit->get_full_name();
	} else if (front_subtoken == "simple_name") {
		return unit->get_simple_name();
	} else if (front_subtoken == "personal_pronoun") {
		return get_gender_personal_pronoun(unit->get_gender());
	} else if (front_subtoken == "capitalized_personal_pronoun") {
		return string::capitalized(get_gender_personal_pronoun(unit->get_gender()));
	} else if (front_subtoken == "possessive_pronoun") {
		return get_gender_possessive_pronoun(unit->get_gender());
	} else if (front_subtoken == "capitalized_possessive_pronoun") {
		return string::capitalized(get_gender_possessive_pronoun(unit->get_gender()));
	} else if (front_subtoken == "oblique_pronoun") {
		return get_gender_oblique_pronoun(unit->get_gender());
	} else if (front_subtoken == "settlement") {
		const site *settlement = unit->get_settlement();

		if (settlement == nullptr) {
			settlement = unit->get_center_tile_settlement();
		}

		bool processed = true;
		return this->process_site_tokens(settlement, tokens, true, processed);
	} else if (front_subtoken == "site") {
		const site *site = unit->get_site();

		bool processed = true;
		return this->process_site_tokens(site, tokens, true, processed);
	}

	throw std::runtime_error("Failed to process unit token \"" + front_subtoken + "\".");
}

}
