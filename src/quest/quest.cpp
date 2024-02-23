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

#include "quest/quest.h"

#include "database/data_module.h"
#include "database/database.h"
#include "database/gsml_parser.h"
#include "dialogue.h"
#include "dialogue_node.h"
#include "dialogue_option.h"
#include "game/difficulty.h"
#include "game/game.h"
#include "iocompat.h"
#include "iolib.h"
#include "luacallback.h"
#include "parameters.h"
#include "player/civilization.h"
#include "player/player_flag.h"
#include "quest/achievement.h"
#include "quest/objective/have_settlement_objective.h"
#include "quest/objective/quest_objective.h"
#include "quest/objective_type.h"
#include "script/condition/and_condition.h"
#include "script/condition/can_accept_quest_condition.h"
#include "script/condition/has_flag_condition.h"
#include "script/condition/not_condition.h"
#include "script/condition/random_condition.h"
#include "script/context.h"
#include "script/effect/accept_quest_effect.h"
#include "script/effect/call_dialogue_effect.h"
#include "script/effect/effect_list.h"
#include "script/effect/set_flag_effect.h"
#include "script/trigger.h"
#include "script/trigger_random_group.h"
#include "script/trigger_target.h"
#include "script/trigger_type.h"
#include "script.h"
#include "text_processor.h"
#include "util/exception_util.h"
#include "util/log_util.h"
#include "util/path_util.h"
#include "util/string_util.h"
#include "util/vector_util.h"

wyrmgus::quest *CurrentQuest = nullptr;

namespace wyrmgus {

std::filesystem::path quest::get_quest_completion_filepath()
{
	return database::get_user_data_path() / "quests.txt";
}

void quest::load_quest_completion()
{
	const std::filesystem::path quests_filepath = quest::get_quest_completion_filepath();

	if (!std::filesystem::exists(quests_filepath)) {
		return;
	}

	gsml_parser parser;
	const gsml_data data = parser.parse(quests_filepath);

	quest::load_quest_completion_scope(data);

	data.for_each_child([&](const gsml_data &scope) {
		if (!database::get()->has_module(scope.get_tag())) {
			return;
		}

		quest::load_quest_completion_scope(scope);
	});
}

void quest::load_quest_completion_scope(const gsml_data &scope)
{
	scope.for_each_property([&](const gsml_property &property) {
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		try {
			quest *quest = quest::get(key);
			const difficulty difficulty = enum_converter<wyrmgus::difficulty>::to_enum(value);

			quest->set_completed(true);

			if (difficulty > quest->get_highest_completed_difficulty()) {
				quest->set_highest_completed_difficulty(difficulty);
			}
		} catch (...) {
			exception::report(std::current_exception());
			log::log_error("Failed to load completion data for quest \"" + key + "\".");
		}
	});
}

void quest::save_quest_completion()
{
	//save quests
	const std::filesystem::path quests_filepath = quest::get_quest_completion_filepath();

	gsml_data data;

	if (std::filesystem::exists(quests_filepath)) {
		gsml_parser parser;
		data = parser.parse(quests_filepath);

		//keep scopes for non-loaded modules unchanged; otherwise, clear data for re-saving
		data.clear_properties();

		for (const qunique_ptr<wyrmgus::data_module> &data_module : database::get()->get_modules()) {
			if (data.has_child(data_module->get_identifier())) {
				data.remove_child(data_module->get_identifier());
			}
		}
	}

	//save quest completion
	std::map<const wyrmgus::data_module *, std::vector<const quest *>> completed_quests;

	for (const quest *quest : quest::get_all()) {
		if (quest->is_completed()) {
			completed_quests[quest->get_module()].push_back(quest);
		}
	}

	for (const auto &[data_module, quests] : completed_quests) {
		gsml_data *quest_scope = &data;

		if (data_module != nullptr) {
			quest_scope = &data.add_child(gsml_data(data_module->get_identifier()));
		}

		for (const quest *quest : quests) {
			quest_scope->add_property(quest->get_identifier(), enum_converter<difficulty>::to_string(quest->get_highest_completed_difficulty()));
		}
	}

	try {
		data.print_to_file(quests_filepath);
	} catch (...) {
		exception::report(std::current_exception());
		log::log_error("Failed to save quest completion file.");
	}
}

quest::quest(const std::string &identifier) : detailed_data_entry(identifier), highest_completed_difficulty(difficulty::none)
{
}

quest::~quest()
{
}

void quest::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "hint") {
		this->hint = value;
	} else {
		data_entry::process_gsml_property(property);
	}
}

void quest::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "objectives") {
		scope.for_each_element([&](const gsml_property &property) {
			auto objective = quest_objective::from_gsml_property(property, this);
			this->objectives.push_back(std::move(objective));
		}, [&](const gsml_data &child_scope) {
			auto objective = quest_objective::from_gsml_scope(child_scope, this);
			this->objectives.push_back(std::move(objective));
		});
	} else if (tag == "objective_strings") {
		for (const std::string &value : values) {
			this->objective_strings.push_back(value);
		}
	} else if (tag == "conditions") {
		this->conditions = std::make_unique<and_condition<CPlayer>>();
		database::process_gsml_data(this->conditions, scope);
	} else if (tag == "accept_effects") {
		this->accept_effects = std::make_unique<effect_list<CPlayer>>();
		database::process_gsml_data(this->accept_effects, scope);
	} else if (tag == "completion_effects") {
		this->completion_effects = std::make_unique<effect_list<CPlayer>>();
		database::process_gsml_data(this->completion_effects, scope);
	} else if (tag == "failure_effects") {
		this->failure_effects = std::make_unique<effect_list<CPlayer>>();
		database::process_gsml_data(this->failure_effects, scope);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void quest::initialize()
{
	if (!this->is_hidden() && this->civilization != nullptr && !vector::contains(this->civilization->Quests, this)) {
		this->civilization->Quests.push_back(this);
	}

	if (!this->is_hidden() && !this->is_unobtainable()) {
		trigger_random_group *trigger_random_group = trigger_random_group::try_get("quest");

		if (trigger_random_group == nullptr) {
			trigger_random_group = trigger_random_group::add("quest", nullptr);
			trigger_random_group->set_type(trigger_type::half_minute_pulse);
		}

		//create a trigger and dialogue for the quest
		wyrmgus::trigger *trigger = trigger::add("quest_" + this->get_identifier(), this->get_module());
		trigger->set_target(trigger_target::player);
		trigger->set_random_group(trigger_random_group);

		const player_flag *decline_flag = player_flag::add("quest_" + this->get_identifier() + "_declined", this->get_module());

		trigger->add_condition(std::make_unique<can_accept_quest_condition>(this));
		trigger->add_condition(std::make_unique<not_condition<CPlayer>>(std::make_unique<has_flag_condition>(decline_flag)));

		wyrmgus::dialogue *dialogue = dialogue::add("quest_" + this->get_identifier(), this->get_module());

		auto dialogue_node = std::make_unique<wyrmgus::dialogue_node>();
		dialogue_node->set_title("Quest: " + this->get_name());
		dialogue_node->set_text("[player.quest_text:" + this->get_identifier() + "]");
		dialogue_node->set_icon(this->get_icon());
		dialogue_node->set_player_color(this->get_player_color());

		auto accept_option = std::make_unique<wyrmgus::dialogue_option>();
		accept_option->set_name("Accept");
		accept_option->set_hotkey("a");
		accept_option->add_effect(std::make_unique<accept_quest_effect>(this));
		dialogue_node->add_option(std::move(accept_option));

		auto decline_option = std::make_unique<wyrmgus::dialogue_option>();
		decline_option->set_name("Decline");
		decline_option->set_hotkey("d");
		decline_option->set_ai_weight(0); //AI players always accept the quests they get
		decline_option->add_effect(std::make_unique<set_flag_effect>(decline_flag));
		dialogue_node->add_option(std::move(decline_option));

		dialogue->add_node(std::move(dialogue_node));

		trigger->add_effect(std::make_unique<call_dialogue_effect<CPlayer>>(dialogue));

		dialogue->initialize();
		trigger->initialize();
	}

	data_entry::initialize();
}

void quest::process_text()
{
	//process the hint text for the quest
	if (!this->hint.empty()) {
		const std::unique_ptr<text_processor_base> text_processor = this->create_text_processor();
		if (text_processor != nullptr) {
			this->hint = text_processor->process_text(std::move(this->hint), false);
		}
	}

	detailed_data_entry::process_text();
}

void quest::check() const
{
	if (this->get_conditions() != nullptr) {
		this->get_conditions()->check_validity();
	}

	if (this->get_accept_effects() != nullptr) {
		this->get_accept_effects()->check();
	}

	if (this->get_completion_effects() != nullptr) {
		this->get_completion_effects()->check();
	}

	if (this->get_failure_effects() != nullptr) {
		this->get_failure_effects()->check();
	}

	for (const std::unique_ptr<quest_objective> &objective : this->get_objectives()) {
		objective->check();
	}
}

std::unique_ptr<text_processor_base> quest::create_text_processor() const
{
	text_processing_context ctx;
	return std::make_unique<text_processor>(std::move(ctx));
}

const icon *quest::get_icon() const
{
	if (this->icon != nullptr) {
		return this->icon;
	}

	return defines::get()->get_default_quest_icon();
}

std::string quest::get_rewards_string(const CPlayer *player) const
{
	if (!this->rewards_string.empty()) {
		return "- " + this->rewards_string;
	}

	if (this->completion_effects != nullptr) {
		read_only_context ctx;
		ctx.current_player = player;
		return this->completion_effects->get_effects_string(player, ctx, 0, "- ");
	}

	return std::string();
}

bool quest::overlaps_with(const quest *other_quest) const
{
	for (const std::unique_ptr<quest_objective> &objective : this->get_objectives()) {
		for (const std::unique_ptr<quest_objective> &other_objective : other_quest->get_objectives()) {
			if (objective->overlaps_with(other_objective.get())) {
				return true;
			}
		}
	}

	return false;
}

void quest::on_completed(const difficulty difficulty)
{
	if (this->is_completed() && difficulty <= this->get_highest_completed_difficulty()) {
		return;
	}

	this->set_completed(true);

	if (difficulty > this->get_highest_completed_difficulty()) {
		this->set_highest_completed_difficulty(difficulty);
	}

	quest::save_quest_completion();

	achievement::check_achievements();
}

bool quest::has_settlement_objective(const site *settlement) const
{
	for (const auto &objective : this->get_objectives()) {
		if (objective->get_objective_type() != objective_type::have_settlement) {
			continue;
		}

		const have_settlement_objective *have_settlement_objective = static_cast<const wyrmgus::have_settlement_objective *>(objective.get());

		if (have_settlement_objective->get_settlement() == settlement) {
			return true;
		}
	}

	return false;
}

}

void SaveQuestCompletion()
{
	quest::save_quest_completion();
}

void SetCurrentQuest(const std::string &quest_ident)
{
	if (quest_ident.empty()) {
		CurrentQuest = nullptr;
	} else {
		CurrentQuest = wyrmgus::quest::get(quest_ident);
	}
}

std::string GetCurrentQuest()
{
	if (!CurrentQuest) {
		return "";
	} else {
		return CurrentQuest->get_identifier();
	}
}

void SetQuestCompleted(const std::string &quest_ident, const int difficulty_int, const bool save)
{
	quest *quest = quest::try_get(quest_ident);
	if (!quest) {
		return;
	}
	
	quest->set_completed(true);

	const difficulty difficulty = static_cast<wyrmgus::difficulty>(difficulty_int);
	if (difficulty > quest->get_highest_completed_difficulty()) {
		quest->set_highest_completed_difficulty(difficulty);
	}

	if (save) {
		quest::save_quest_completion();
	}

	achievement::check_achievements();
}

void SetQuestCompleted(const std::string &quest_ident, const std::string &difficulty_str, const bool save)
{
	if (string::is_number(difficulty_str)) {
		SetQuestCompleted(quest_ident, std::stoi(difficulty_str), save);
		return;
	}

	quest *quest = quest::try_get(quest_ident);
	if (!quest) {
		return;
	}
	
	quest->set_completed(true);

	const difficulty difficulty = enum_converter<wyrmgus::difficulty>::to_enum(difficulty_str);
	if (difficulty > quest->get_highest_completed_difficulty()) {
		quest->set_highest_completed_difficulty(difficulty);
	}

	if (save) {
		quest::save_quest_completion();
	}

	achievement::check_achievements();
}

void SetQuestCompleted(const std::string &quest_ident, const bool save)
{
	SetQuestCompleted(quest_ident, static_cast<int>(difficulty::normal), save);
}
