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
//      (c) Copyright 2015-2022 by Andrettin
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
#include "database/defines.h"
#include "dialogue.h"
#include "dialogue_node_instance.h"
#include "dialogue_option.h"
#include "engine_interface.h"
#include "game/game.h"
#include "luacallback.h"
#include "player/faction.h"
#include "player/player.h"
#include "player/player_color.h"
#include "player/player_type.h"
#include "script.h"
#include "script/condition/and_condition.h"
#include "script/context.h"
#include "sound/sound.h"
#include "text_processor.h"
#include "ui/icon.h"
#include "unit/unit_find.h"
#include "unit/unit_ref.h"
#include "unit/unit_type.h"
#include "util/exception_util.h"
#include "util/string_util.h"
#include "util/vector_random_util.h"

namespace wyrmgus {

dialogue_node::dialogue_node()
{
}

dialogue_node::dialogue_node(wyrmgus::dialogue *dialogue, const int index) : dialogue(dialogue), index(index)
{
}

dialogue_node::~dialogue_node()
{
}

void dialogue_node::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "title") {
		this->title = value;
	} else if (key == "icon") {
		this->icon = icon::get(value);
	} else if (key == "player_color") {
		this->player_color = player_color::get(value);
	} else if (key == "text") {
		this->text = value;
	} else if (key == "sound") {
		this->sound = sound::get(value);
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

void dialogue_node::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "conditions") {
		auto conditions = std::make_unique<and_condition<CPlayer>>();
		database::process_gsml_data(conditions, scope);
		this->conditions = std::move(conditions);
	} else if (tag == "option") {
		auto option = std::make_unique<dialogue_option>(this);
		database::process_gsml_data(option, scope);
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
	assert_throw(this->get_dialogue() != nullptr);
	assert_throw(this->get_index() != -1);

	for (const std::unique_ptr<dialogue_option> &option : this->options) {
		option->check();
	}

	if (this->conditions != nullptr) {
		this->conditions->check_validity();
	}
}

void dialogue_node::add_option(std::unique_ptr<dialogue_option> &&option)
{
	option->set_node(this);

	this->option_pointers.push_back(option.get());
	this->options.push_back(std::move(option));
}

void dialogue_node::call(CPlayer *player, context &ctx) const
{
	if (this->conditions != nullptr) {
		if (!this->conditions->check(player, ctx)) {
			this->get_dialogue()->call_node(this->get_index() + 1, player, ctx);
			return;
		}
	}

	if (this->Conditions != nullptr) {
		this->Conditions->pushPreamble();
		this->Conditions->run(1);
		if (this->Conditions->popBoolean() == false) {
			this->get_dialogue()->call_node(this->get_index() + 1, player, ctx);
			return;
		}
	}

	if (this->ImmediateEffects) {
		this->ImmediateEffects->pushPreamble();
		this->ImmediateEffects->run();
	}

	if (player != CPlayer::GetThisPlayer()) {
		if ((player->AiEnabled || player->get_type() != player_type::person) && !this->option_pointers.empty()) {
			//AIs will choose a random option

			std::vector<int> option_indexes;

			for (size_t i = 0; i < this->option_pointers.size(); ++i) {
				const dialogue_option *option = this->option_pointers[i];

				for (int j = 0; j < option->get_ai_weight(); ++j) {
					option_indexes.push_back(static_cast<int>(i));
				}
			}

			assert_throw(!option_indexes.empty());

			const int option_index = vector::get_random(option_indexes);
			this->dialogue->call_node_option_effect(this->get_index(), option_index, player, ctx);
		}

		return;
	}

	const CUnit *speaker_unit = this->get_speaker_unit();

	text_processing_context text_ctx(ctx);
	const text_processor text_processor(std::move(text_ctx));

	const QString title_str = QString::fromStdString(this->get_title_string(speaker_unit, text_processor));
	const QString text = QString::fromStdString(this->get_text(text_processor));

	const wyrmgus::icon *icon = this->get_icon(speaker_unit);

	const wyrmgus::player_color *player_color = this->get_player_color(speaker_unit);

	QStringList options;
	QStringList option_hotkeys;
	QStringList option_tooltips;

	if (!this->option_pointers.empty()) {
		for (const dialogue_option *option : this->option_pointers) {
			options.push_back(QString::fromStdString(option->get_name()));
			option_hotkeys.push_back(QString::fromStdString(option->get_hotkey()));
			option_tooltips.push_back(QString::fromStdString(option->get_tooltip(ctx)));
		}
	} else {
		options.push_back(dialogue_option::default_name);
		option_hotkeys.push_back(dialogue_option::default_hotkey);
		option_tooltips.push_back(QString());
	}

	const int unit_number = ctx.current_unit != nullptr ? UnitNumber(*ctx.current_unit->get()) : -1;

	auto dialogue_node_instance = make_qunique<wyrmgus::dialogue_node_instance>(this, title_str, text, icon, player_color, options, option_hotkeys, option_tooltips, unit_number);
	engine_interface::get()->add_dialogue_node_instance(std::move(dialogue_node_instance));

	if (this->sound != nullptr) {
		const int channel = PlayGameSound(this->sound, MaxSampleVolume);
		dialogue::add_sound_channel(channel);
	}

	if (!game::get()->is_multiplayer()) {
		game::get()->set_paused(true);
	}
}

void dialogue_node::option_effect(const int option_index, CPlayer *player, context &ctx) const
{
	//stop any dialogue sounds (i.e. voice overs) if any are still playing
	dialogue::stop_sound_channels();

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

	this->get_dialogue()->call_node(this->get_index() + 1, player, ctx);
}

const CUnit *dialogue_node::get_speaker_unit() const
{
	if (this->speaker != nullptr) {
		return this->speaker->get_unit();
	} else if (this->speaker_unit_type != nullptr) {
		const CPlayer *speaker_player = CPlayer::get_neutral_player();
		if (this->speaker_faction != nullptr) {
			speaker_player = GetFactionPlayer(this->speaker_faction);
		}

		if (speaker_player != nullptr) {
			std::vector<CUnit *> potential_speaker_units;
			FindPlayerUnitsByType(*speaker_player, *this->speaker_unit_type, potential_speaker_units);

			if (!potential_speaker_units.empty()) {
				const size_t index = std::min(potential_speaker_units.size() - 1, this->speaker_index);
				return potential_speaker_units.at(index);
			}
		}
	}

	return nullptr;
}

std::string dialogue_node::get_title_string(const CUnit *speaker_unit, const text_processor &text_processor) const
{
	if (speaker_unit != nullptr) {
		std::string unit_name = speaker_unit->get_full_name();
		if (!unit_name.empty()) {
			return unit_name;
		} else {
			speaker_unit->Type->get_name();
		}
	}

	if (this->speaker != nullptr) {
		return this->speaker->get_full_name();
	}

	std::string title;

	try {
		title = text_processor.process_text(this->title, true);
	} catch (const std::exception &exception) {
		exception::report(exception);
		title = this->title;
	}

	return title;
}

const wyrmgus::icon *dialogue_node::get_icon(const CUnit *speaker_unit) const
{
	if (speaker_unit != nullptr) {
		return speaker_unit->get_icon();
	}

	return this->icon;
}

const wyrmgus::player_color *dialogue_node::get_player_color(const CUnit *speaker_unit) const
{
	if (speaker_unit != nullptr) {
		return speaker_unit->get_player_color();
	}

	if (this->speaker_faction != nullptr) {
		const CPlayer *speaker_player = GetFactionPlayer(this->speaker_faction);
		if (speaker_player != nullptr) {
			return speaker_player->get_player_color();
		}
	}

	if (this->player_color != nullptr) {
		return this->player_color;
	}

	return defines::get()->get_neutral_player_color();
}

std::string dialogue_node::get_text(const text_processor &text_processor) const
{
	std::string text;

	try {
		text = text_processor.process_text(this->text, true);
	} catch (const std::exception &exception) {
		exception::report(exception);
		text = this->text;
	}

	return text;
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
