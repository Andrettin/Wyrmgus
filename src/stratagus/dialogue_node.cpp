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
//      (c) Copyright 2015-2021 by Andrettin
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

#include "dialogue_node.h"

#include "character.h"
#include "dialogue.h"
#include "dialogue_option.h"
#include "faction.h"
#include "luacallback.h"
#include "script.h"
#include "script/condition/and_condition.h"
#include "script/context.h"
#include "text_processor.h"
#include "unit/unit_find.h"
#include "unit/unit_type.h"
#include "util/exception_util.h"
#include "util/string_util.h"
#include "util/vector_random_util.h"

namespace wyrmgus {

dialogue_node::dialogue_node(wyrmgus::dialogue *dialogue) : dialogue(dialogue)
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
	} else if (key == "speaker_index") {
		this->speaker_index = std::stoul(value);
	} else if (key == "option") {
		this->option_pointers.push_back(this->get_dialogue()->get_option(value));
	} else {
		throw std::runtime_error("Invalid dialogue node property: \"" + key + "\".");
	}
}

void dialogue_node::process_sml_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "conditions") {
		auto conditions = std::make_unique<and_condition>();
		database::process_sml_data(conditions, scope);
		this->conditions = std::move(conditions);
	} else if (tag == "option") {
		auto option = std::make_unique<dialogue_option>(this);
		database::process_sml_data(option, scope);
		this->add_option(std::move(option));
	} else {
		throw std::runtime_error("Invalid dialogue node scope: \"" + tag + "\".");
	}
}

void dialogue_node::initialize()
{
	for (const std::unique_ptr<dialogue_option> &option : this->options) {
		option->initialize();
	}
}

void dialogue_node::check() const
{
	for (const std::unique_ptr<dialogue_option> &option : this->options) {
		option->check();
	}

	if (this->conditions != nullptr) {
		this->conditions->check_validity();
	}
}

void dialogue_node::call(CPlayer *player, const context &ctx) const
{
	if (this->conditions != nullptr) {
		if (!this->conditions->check(player)) {
			this->get_dialogue()->call_node(this->ID + 1, player, ctx);
			return;
		}
	}

	if (this->Conditions != nullptr) {
		this->Conditions->pushPreamble();
		this->Conditions->run(1);
		if (this->Conditions->popBoolean() == false) {
			this->get_dialogue()->call_node(this->ID + 1, player, ctx);
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
				const size_t index = std::min(potential_speaker_units.size() - 1, this->speaker_index);
				speaker_unit = potential_speaker_units.at(index);
			}
		}
	}

	if (speaker_unit != nullptr) {
		lua_command += std::to_string(UnitNumber(*speaker_unit)) + ", ";
	} else {
		lua_command += "\"" + this->speaker_name + "\", ";
	}

	text_processing_context text_ctx(ctx);
	const text_processor text_processor(std::move(text_ctx));

	std::string text;
	try {
		text = text_processor.process_text(this->text);
	} catch (const std::exception &exception) {
		exception::report(exception);
		text = this->text;
	}
	string::replace(text, "\"", "\\\"");
	string::replace(text, "\n", "\\n");

	lua_command += "\"" + std::move(text) + "\", ";
	lua_command += std::to_string(player->Index) + ", ";

	lua_command += "{";
	if (!this->option_pointers.empty() && !this->option_pointers.front()->get_name().empty()) {
		bool first = true;
		for (const dialogue_option *option : this->option_pointers) {
			if (!first) {
				lua_command += ", ";
			} else {
				first = false;
			}
			lua_command += "\"" + option->get_name() + "\"";
		}
	} else {
		lua_command += "\"~!Continue\"";
	}
	lua_command += "}, ";

	lua_command += "{";
	if (!this->option_pointers.empty()) {
		bool first = true;
		for (size_t i = 0; i < this->option_pointers.size(); ++i) {
			if (!first) {
				lua_command += ", ";
			} else {
				first = false;
			}
			lua_command += "function(s) ";
			lua_command += "CallDialogueNodeOptionEffect(\"" + this->get_dialogue()->get_identifier() + "\", " + std::to_string(this->ID) + ", " + std::to_string(i) + ", " + std::to_string(player->Index) + ");";
			lua_command += " end";
		}
	} else {
		lua_command += "function(s) ";
		lua_command += "CallDialogueNodeOptionEffect(\"" + this->get_dialogue()->get_identifier() + "\", " + std::to_string(this->ID) + ", " + std::to_string(0) + ", " + std::to_string(player->Index) + ");";
		lua_command += " end";
	}
	lua_command += "}, ";

	lua_command += "nil, nil, nil, ";

	lua_command += "{";
	if (!this->option_pointers.empty() && !this->option_pointers.front()->get_tooltip(player).empty()) {
		lua_command += "OptionTooltips = {";
		bool first = true;
		for (const dialogue_option *option : this->option_pointers) {
			if (!first) {
				lua_command += ", ";
			} else {
				first = false;
			}
			std::string tooltip = string::to_tooltip(option->get_tooltip(player));
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

void dialogue_node::option_effect(const int option_index, CPlayer *player, const context &ctx) const
{
	if (option_index < static_cast<int>(this->option_pointers.size())) {
		const dialogue_option *option = this->option_pointers[option_index];

		option->do_effects(player, ctx);

		if (option->ends_dialogue()) {
			return;
		} else if (option->get_next_node() != nullptr) {
			option->get_next_node()->call(player, ctx);
			return;
		}
	}

	this->get_dialogue()->call_node(this->ID + 1, player, ctx);
}

void dialogue_node::delete_lua_callbacks()
{
	this->Conditions.reset();
	this->ImmediateEffects.reset();

	for (const std::unique_ptr<dialogue_option> &option : this->options) {
		option->delete_lua_callbacks();
	}
}

}
