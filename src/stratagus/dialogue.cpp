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

#include "dialogue.h"

#include "dialogue_node.h"
#include "dialogue_option.h"
#include "player.h"
#include "script.h"
#include "script/context.h"
#include "script/effect/call_dialogue_effect.h"
#include "script/trigger.h"

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

	if (tag == "trigger") {
		//create the trigger for this dialogue
		wyrmgus::trigger *trigger = trigger::add(this->get_identifier(), this->get_module());
		database::process_sml_data(trigger, scope);

		//add an effect to call this dialogue to the trigger
		auto dialogue_effect = std::make_unique<call_dialogue_effect<CPlayer>>(this, sml_operator::assignment);
		trigger->add_effect(std::move(dialogue_effect));

		trigger->Type = trigger::TriggerType::PlayerTrigger;
	} else {
		auto node = std::make_unique<dialogue_node>(this);
		node->ID = this->nodes.size();
		database::process_sml_data(node, scope);

		if (!tag.empty()) {
			this->nodes_by_identifier[tag] = node.get();
		}

		this->nodes.push_back(std::move(node));
	}
}

void dialogue::initialize()
{
	for (const std::unique_ptr<dialogue_node> &node : this->nodes) {
		node->initialize();
	}

	data_entry::initialize();
}

void dialogue::check() const
{
	for (const std::unique_ptr<dialogue_node> &node : this->nodes) {
		node->check();
	}
}

void dialogue::map_option(const dialogue_option *option, const std::string &identifier)
{
	if (identifier.empty()) {
		throw std::runtime_error("Cannot map option to an empty string identifier.");
	}

	this->options_by_identifier[identifier] = option;
}

void dialogue::call(CPlayer *player, const context &ctx) const
{
	if (this->nodes.empty()) {
		return;
	}
	
	this->nodes.front()->call(player, ctx);
}

void dialogue::call_node(const int node_index, CPlayer *player, const context &ctx) const
{
	if (node_index >= static_cast<int>(this->nodes.size())) {
		return;
	}

	this->nodes[node_index]->call(player, ctx);
}

void dialogue::call_node_option_effect(const int node_index, const int option_index, CPlayer *player, const context &ctx) const
{
	if (node_index >= static_cast<int>(this->nodes.size())) {
		return;
	}

	CclCommand("trigger_player = " + std::to_string(player->Index) + ";");
	this->nodes[node_index]->option_effect(option_index, player, ctx);
}

void dialogue::delete_lua_callbacks()
{
	for (const std::unique_ptr<dialogue_node> &node : this->nodes) {
		node->delete_lua_callbacks();
	}
}

}

void CallDialogue(const std::string &dialogue_ident, int player_index)
{
	const wyrmgus::dialogue *dialogue = wyrmgus::dialogue::get(dialogue_ident);

	CPlayer *player = CPlayer::Players.at(player_index);
	wyrmgus::context ctx;
	ctx.current_player = player;

	dialogue->call(player, ctx);
}

void CallDialogueNode(const std::string &dialogue_ident, int node, int player_index)
{
	const wyrmgus::dialogue *dialogue = wyrmgus::dialogue::get(dialogue_ident);

	CPlayer *player = CPlayer::Players.at(player_index);
	wyrmgus::context ctx;
	ctx.current_player = player;

	dialogue->call_node(node, player, ctx);
}

void CallDialogueNodeOptionEffect(const std::string &dialogue_ident, int node, int option, int player_index)
{
	const wyrmgus::dialogue *dialogue = wyrmgus::dialogue::get(dialogue_ident);

	CPlayer *player = CPlayer::Players.at(player_index);
	wyrmgus::context ctx;
	ctx.current_player = player;

	dialogue->call_node_option_effect(node, option, player, ctx);
}
