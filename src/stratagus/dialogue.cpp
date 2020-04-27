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

#include "dialogue_option.h"
#include "luacallback.h"
#include "player.h"
#include "script.h"

namespace stratagus {

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

void dialogue::call(const int player) const
{
	if (this->nodes.empty()) {
		return;
	}
	
	this->nodes.front()->call(player);
}

void dialogue::call_node(const int node_index, const int player) const
{
	if (node_index >= static_cast<int>(this->nodes.size())) {
		return;
	}

	this->nodes[node_index]->call(player);
}

void dialogue::call_node_option_effect(const int node_index, const int option_index, const int player) const
{
	if (node_index >= static_cast<int>(this->nodes.size())) {
		return;
	}

	CclCommand("trigger_player = " + std::to_string(player) + ";");
	this->nodes[node_index]->option_effect(option_index, player);
}

dialogue_node::dialogue_node()
{
}

dialogue_node::~dialogue_node()
{
	if (this->Conditions) {
		delete Conditions;
	}
	
	if (this->ImmediateEffects) {
		delete ImmediateEffects;
	}
}

void dialogue_node::process_sml_property(const sml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "text") {
		this->Text = value;
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

void dialogue_node::call(const int player) const
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
	
	if (this->SpeakerType == "character") {
		lua_command += "FindHero(\"" + this->Speaker;
	} else if (this->SpeakerType == "unit") {
		lua_command += "FindUnit(\"" + this->Speaker;
	} else {
		lua_command += "\"" + this->Speaker + "\", ";
	}
	
	if (this->SpeakerType == "character" || this->SpeakerType == "unit") {
		lua_command += "\"";
		if (!this->SpeakerPlayer.empty()) {
			lua_command += ", GetFactionPlayer(\"" + this->SpeakerPlayer + "\")";
		}
		lua_command += "), ";
	}
	
	lua_command += "\"" + FindAndReplaceString(FindAndReplaceString(this->Text, "\"", "\\\""), "\n", "\\n") + "\", ";
	lua_command += std::to_string((long long) player) + ", ";
	
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
			lua_command += "CallDialogueNodeOptionEffect(\"" + this->Dialogue->get_identifier() + "\", " + std::to_string((long long) this->ID) + ", " + std::to_string((long long) i) + ", " + std::to_string((long long) player) + ");";
			lua_command += " end";
		}
	} else {
		lua_command += "function(s) ";
		lua_command += "CallDialogueNodeOptionEffect(\"" + this->Dialogue->get_identifier() + "\", " + std::to_string((long long) this->ID) + ", " + std::to_string((long long) 0) + ", " + std::to_string((long long) player) + ");";
		lua_command += " end";
	}
	lua_command += "}, ";

	lua_command += "nil, nil, nil, ";
	
	lua_command += "{";
	if (!this->options.empty() && !this->options.front()->tooltip.empty()) {
		lua_command += "OptionTooltips = {";
		bool first = true;
		for (const auto &option : this->options) {
			if (!first) {
				lua_command += ", ";
			} else {
				first = false;
			}
			lua_command += "\"" + option->tooltip + "\"";
		}
		lua_command += "}";
	}
	lua_command += "}";
	
	lua_command += ")";
	
	CclCommand(lua_command);
}

void dialogue_node::option_effect(const int option_index, const int player) const
{
	if (option_index < static_cast<int>(this->options.size())) {
		const auto &option = this->options[option_index];
		option->do_effects(CPlayer::Players.at(player));
	}

	this->Dialogue->call_node(this->ID + 1, player);
}

}

void CallDialogue(const std::string &dialogue_ident, int player)
{
	stratagus::dialogue *dialogue = stratagus::dialogue::get(dialogue_ident);
	dialogue->call(player);
}

void CallDialogueNode(const std::string &dialogue_ident, int node, int player)
{
	stratagus::dialogue *dialogue = stratagus::dialogue::get(dialogue_ident);
	dialogue->call_node(node, player);
}

void CallDialogueNodeOptionEffect(const std::string &dialogue_ident, int node, int option, int player)
{
	stratagus::dialogue *dialogue = stratagus::dialogue::get(dialogue_ident);
	dialogue->call_node_option_effect(node, option, player);
}
