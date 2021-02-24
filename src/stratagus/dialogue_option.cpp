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

#include "dialogue_option.h"

#include "database/database.h"
#include "database/sml_data.h"
#include "database/sml_property.h"
#include "dialogue.h"
#include "dialogue_node.h"
#include "luacallback.h"
#include "script/context.h"
#include "script/effect/effect_list.h"
#include "util/string_conversion_util.h"

namespace wyrmgus {

dialogue_option::dialogue_option(const dialogue_node *node) : node(node)
{
}

dialogue_option::~dialogue_option()
{
}

void dialogue_option::process_sml_property(const sml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "identifier") {
		this->get_dialogue()->map_option(this, value);
	} else if (key == "name") {
		this->name = value;
	} else if (key == "next_node") {
		this->next_node_identifier = value;
	} else if (key == "end_dialogue") {
		this->end_dialogue = string::to_bool(value);
	} else if (key == "tooltip") {
		this->tooltip = value;
	} else {
		throw std::runtime_error("Invalid dialogue option property: \"" + key + "\".");
	}
}

void dialogue_option::process_sml_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "effects") {
		this->effects = std::make_unique<effect_list<CPlayer>>();
		database::process_sml_data(this->effects, scope);
	} else {
		throw std::runtime_error("Invalid dialogue option scope: \"" + tag + "\".");
	}
}

void dialogue_option::initialize()
{
	if (!this->next_node_identifier.empty()) {
		this->next_node = this->get_dialogue()->get_node(this->next_node_identifier);
		this->next_node_identifier.clear();
	}
}

void dialogue_option::check() const
{
	if (this->ends_dialogue() && this->get_next_node() != nullptr) {
		throw std::runtime_error("Dialogue option is set to end the dialogue at the same time that it has a next node set for it.");
	}

	if (this->effects != nullptr) {
		this->effects->check();
	}
}

dialogue *dialogue_option::get_dialogue() const
{
	return this->node->get_dialogue();
}

void dialogue_option::do_effects(CPlayer *player, const context &ctx) const
{
	if (this->effects != nullptr) {
		this->effects->do_effects(player, ctx);
	}

	if (this->lua_effects != nullptr) {
		this->lua_effects->pushPreamble();
		this->lua_effects->run();
	}
}

std::string dialogue_option::get_tooltip(CPlayer *player) const
{
	if (!this->tooltip.empty()) {
		return this->tooltip;
	}

	if (this->effects != nullptr) {
		read_only_context ctx;
		ctx.current_player = player;
		return this->effects->get_effects_string(player, ctx);
	}

	return std::string();
}

void dialogue_option::delete_lua_callbacks()
{
	this->lua_effects.reset();
}

}
