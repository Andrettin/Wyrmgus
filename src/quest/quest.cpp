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

#include "quest/quest.h"

#include "database/database.h"
#include "database/sml_parser.h"
#include "game/difficulty.h"
#include "game/game.h"
#include "iocompat.h"
#include "iolib.h"
#include "luacallback.h"
#include "parameters.h"
#include "player/civilization.h"
#include "quest/achievement.h"
#include "quest/objective/quest_objective.h"
#include "quest/objective_type.h"
#include "script/condition/and_condition.h"
#include "script/context.h"
#include "script/effect/effect_list.h"
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

	sml_parser parser;
	const sml_data data = parser.parse(quests_filepath);

	data.for_each_property([&](const sml_property &property) {
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		try {
			quest *quest = quest::get(key);
			const difficulty difficulty = string_to_difficulty(value);

			quest->set_completed(true);

			if (difficulty > quest->get_highest_completed_difficulty()) {
				quest->set_highest_completed_difficulty(difficulty);
			}
		} catch (const std::exception &exception) {
			exception::report(exception);
			log::log_error("Failed to load completion data for quest \"" + key + "\".");
		}
	});
}

void quest::save_quest_completion()
{
	//save quests
	const std::filesystem::path quests_filepath = quest::get_quest_completion_filepath();

	sml_data data;

	for (const quest *quest : quest::get_all()) {
		if (quest->is_completed()) {
			data.add_property(quest->get_identifier(), difficulty_to_string(quest->get_highest_completed_difficulty()));
		}
	}

	try {
		data.print_to_file(quests_filepath);
	} catch (const std::exception &exception) {
		exception::report(exception);
		log::log_error("Failed to save quest completion file.");
	}

	//save achievements to the old quests.lua file
	std::filesystem::path path = database::get()->get_root_path();

	if (!GameName.empty()) {
		path /= GameName;
	}

	path /= "quests.lua";

	path.make_preferred();

	FILE *fd = fopen(path::to_string(path).c_str(), "w");
	if (!fd) {
		log::log_error("Cannot open file \"" + path::to_string(path) + "\" for writing.");
		return;
	}

	for (const achievement *achievement : achievement::get_all()) {
		if (achievement->is_obtained()) {
			fprintf(fd, "SetAchievementObtained(\"%s\", false, false)\n", achievement->get_identifier().c_str());
		}
	}

	fclose(fd);
}

quest::quest(const std::string &identifier) : detailed_data_entry(identifier), highest_completed_difficulty(difficulty::none)
{
}

quest::~quest()
{
}

void quest::process_sml_property(const sml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "hint") {
		this->hint = value;
	} else {
		data_entry::process_sml_property(property);
	}
}

void quest::process_sml_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "objectives") {
		scope.for_each_element([&](const sml_property &property) {
			auto objective = quest_objective::from_sml_property(property, this);
			this->objectives.push_back(std::move(objective));
		}, [&](const sml_data &child_scope) {
			auto objective = quest_objective::from_sml_scope(child_scope, this);
			this->objectives.push_back(std::move(objective));
		});
	} else if (tag == "objective_strings") {
		for (const std::string &value : values) {
			this->objective_strings.push_back(value);
		}
	} else if (tag == "conditions") {
		this->conditions = std::make_unique<and_condition>();
		database::process_sml_data(this->conditions, scope);
	} else if (tag == "accept_effects") {
		this->accept_effects = std::make_unique<effect_list<CPlayer>>();
		database::process_sml_data(this->accept_effects, scope);
	} else if (tag == "completion_effects") {
		this->completion_effects = std::make_unique<effect_list<CPlayer>>();
		database::process_sml_data(this->completion_effects, scope);
	} else if (tag == "failure_effects") {
		this->failure_effects = std::make_unique<effect_list<CPlayer>>();
		database::process_sml_data(this->failure_effects, scope);
	} else {
		data_entry::process_sml_scope(scope);
	}
}

void quest::initialize()
{
	if (!this->is_hidden() && this->civilization != nullptr && !vector::contains(this->civilization->Quests, this)) {
		this->civilization->Quests.push_back(this);
	}

	data_entry::initialize();
}

void quest::process_text()
{
	//process the hint text for the quest
	if (!this->hint.empty()) {
		const text_processor text_processor = this->create_text_processor();
		this->hint = text_processor.process_text(std::move(this->hint));
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

	const difficulty difficulty = string_to_difficulty(difficulty_str);
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
