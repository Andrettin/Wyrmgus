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
//      (c) Copyright 2015-2020 by Andrettin
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

#include "dialogue.h"

#include "character.h"
#include "dialogue_option.h"
#include "faction.h"
#include "luacallback.h"
#include "player.h"
#include "script.h"
#include "unit/unit.h"
#include "unit/unit_find.h"
#include "unit/unit_type.h"
#include "util/string_util.h"
#include "util/vector_random_util.h"

namespace wyrmgus {

dialogue::dialogue(const std::string &identifier) : data_entry(identifier)
{
}

dialogue::~dialogue()
{
}


void dialogue::process_sml_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag.empty()) {
		auto node = std::make_unique<dialogue_node>();
		node->ID = this->nodes.size();
		node->Dialogue = this;
		database::process_sml_data(node, scope);
		this->nodes.push_back(std::move(node));
	} else {
		data_entry::process_sml_scope(scope);
	}
}

void dialogue::check() const
{
	for (const std::unique_ptr<dialogue_node> &node : this->nodes) {
		node->check();
	}
}

void dialogue::call(CPlayer *player) const
{
	if (this->nodes.empty()) {
		return;
	}
	
	this->nodes.front()->call(player);
}

void dialogue::call_node(const int node_index, CPlayer *player) const
{
	if (node_index >= static_cast<int>(this->nodes.size())) {
		return;
	}

	this->nodes[node_index]->call(player);
}

void dialogue::call_node_option_effect(const int node_index, const int option_index, CPlayer *player) const
{
	if (node_index >= static_cast<int>(this->nodes.size())) {
		return;
	}

	CclCommand("trigger_player = " + std::to_string(player->Index) + ";");
	this->nodes[node_index]->option_effect(option_index, player);
}

dialogue_node::dialogue_node()
{
}

dialogue_node::~dialogue_node()
{
}

void dialogue_node::process_sml_property(const sml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "text") {
		this->text = value;
	} else if (key == "speaker") {
		this->speaker = character::get(value);
	} else if (key == "speaker_unit_type") {
		this->speaker_unit_type = unit_type::get(value);
	} else if (key == "speaker_faction") {
		this->speaker_faction = faction::get(value);
	} else {
		throw std::runtime_error("Invalid dialogue node property: \"" + key + "\".");
	}
}

void dialogue_node::process_sml_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "option") {
		auto option = std::make_unique<dialogue_option>();
		database::process_sml_data(option, scope);
		this->options.push_back(std::move(option));
	} else {
		throw std::runtime_error("Invalid dialogue node scope: \"" + tag + "\".");
	}
}

void dialogue_node::check() const
{
	for (const auto &option : this->options) {
		option->check();
	}

}

void dialogue_node::call(CPlayer *player) const
{
	if (this->Conditions) {
		this->Conditions->pushPreamble();
		this->Conditions->run(1);
		if (this->Conditions->popBoolean() == false) {
			this->Dialogue->call_node(this->ID + 1, player);
			return;
		}
	}
	
	if (this->ImmediateEffects) {
		this->ImmediateEffects->pushPreamble();
		this->ImmediateEffects->run();
	}
	
	std::string lua_command = "Event(";
	
	const CUnit *speaker_unit = nullptr;
	if (this->speaker != nullptr) {
		speaker_unit = this->speaker->get_unit();
	} else if (this->speaker_unit_type != nullptr) {
		const CPlayer *speaker_player = CPlayer::Players[PlayerNumNeutral];
		if (this->speaker_faction != nullptr) {
			speaker_player = GetFactionPlayer(this->speaker_faction);
		}

		if (speaker_player != nullptr) {
			std::vector<CUnit *> potential_speaker_units;
			FindPlayerUnitsByType(*speaker_player, *this->speaker_unit_type, potential_speaker_units);

			if (!potential_speaker_units.empty()) {
				speaker_unit = vector::get_random(potential_speaker_units);
			}
		}
	}

	if (speaker_unit != nullptr) {
		lua_command += std::to_string(UnitNumber(*speaker_unit)) + ", ";
	} else {
		lua_command += "\"" + this->speaker_name + "\", ";
	}

	lua_command += "\"" + FindAndReplaceString(FindAndReplaceString(this->text, "\"", "\\\""), "\n", "\\n") + "\", ";
	lua_command += std::to_string(player->Index) + ", ";
	
	lua_command += "{";
	if (!this->options.empty() && !this->options.front()->get_name().empty()) {
		bool first = true;
		for (const auto &option : this->options) {
			if (!first) {
				lua_command += ", ";
			} else {
				first = false;
			}
			lua_command += "\"" + option->get_name() +"\"";
		}
	} else {
		lua_command += "\"~!Continue\"";
	}
	lua_command += "}, ";
	
	lua_command += "{";
	if (!this->options.empty()) {
		bool first = true;
		for (size_t i = 0; i < this->options.size(); ++i) {
			if (!first) {
				lua_command += ", ";
			} else {
				first = false;
			}
			lua_command += "function(s) ";
			lua_command += "CallDialogueNodeOptionEffect(\"" + this->Dialogue->get_identifier() + "\", " + std::to_string(this->ID) + ", " + std::to_string(i) + ", " + std::to_string(player->Index) + ");";
			lua_command += " end";
		}
	} else {
		lua_command += "function(s) ";
		lua_command += "CallDialogueNodeOptionEffect(\"" + this->Dialogue->get_identifier() + "\", " + std::to_string(this->ID) + ", " + std::to_string(0) + ", " + std::to_string(player->Index) + ");";
		lua_command += " end";
	}
	lua_command += "}, ";

	lua_command += "nil, nil, nil, ";
	
	lua_command += "{";
	if (!this->options.empty() && !this->options.front()->get_tooltip().empty()) {
		lua_command += "OptionTooltips = {";
		bool first = true;
		for (const auto &option : this->options) {
			if (!first) {
				lua_command += ", ";
			} else {
				first = false;
			}
			std::string tooltip = option->get_tooltip();
			string::replace(tooltip, "\n", "\\n");
			string::replace(tooltip, "\t", "\\t");
			lua_command += "\"" + tooltip + "\"";
		}
		lua_command += "}";
	}
	lua_command += "}";
	
	lua_command += ")";
	
	CclCommand(lua_command);
}

void dialogue_node::option_effect(const int option_index, CPlayer *player) const
{
	if (option_index < static_cast<int>(this->options.size())) {
		const auto &option = this->options[option_index];
		option->do_effects(player);
	}

	this->Dialogue->call_node(this->ID + 1, player);
}

}

void CallDialogue(const std::string &dialogue_ident, int player)
{
	wyrmgus::dialogue *dialogue = wyrmgus::dialogue::get(dialogue_ident);
	dialogue->call(CPlayer::Players.at(player));
}

void CallDialogueNode(const std::string &dialogue_ident, int node, int player)
{
	wyrmgus::dialogue *dialogue = wyrmgus::dialogue::get(dialogue_ident);
	dialogue->call_node(node, CPlayer::Players.at(player));
}

void CallDialogueNodeOptionEffect(const std::string &dialogue_ident, int node, int option, int player)
{
	wyrmgus::dialogue *dialogue = wyrmgus::dialogue::get(dialogue_ident);
	dialogue->call_node_option_effect(node, option, CPlayer::Players.at(player));
}
