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

#include "quest.h"

#include "achievement.h"
#include "character.h"
#include "civilization.h"
#include "faction.h"
#include "game.h"
#include "iocompat.h"
#include "iolib.h"
#include "luacallback.h"
#include "map/site.h"
#include "objective_type.h"
#include "parameters.h"
#include "player.h"
#include "player_color.h"
#include "script/condition/and_condition.h"
#include "script/effect/effect_list.h"
#include "script.h"
#include "unit/unit_class.h"

wyrmgus::quest *CurrentQuest = nullptr;

void SaveQuestCompletion()
{
	std::string path = Parameters::Instance.GetUserDirectory();

	if (!GameName.empty()) {
		path += "/";
		path += GameName;
	}
	path += "/";
	path += "quests.lua";

	FILE *fd = fopen(path.c_str(), "w");
	if (!fd) {
		fprintf(stderr, "Cannot open file %s for writing.\n", path.c_str());
		return;
	}

	for (const CAchievement *achievement : CAchievement::GetAchievements()) {
		if (achievement->is_obtained()) {
			fprintf(fd, "SetAchievementObtained(\"%s\", false, false)\n", achievement->Ident.c_str());
		}
	}
	
	fprintf(fd, "\n");
	
	for (const wyrmgus::quest *quest : wyrmgus::quest::get_all()) {
		if (quest->Completed) {
			fprintf(fd, "SetQuestCompleted(\"%s\", %d, false)\n", quest->get_identifier().c_str(), quest->HighestCompletedDifficulty);
		}
	}
	
	fclose(fd);
}

namespace wyrmgus {

quest_objective::quest_objective(const wyrmgus::objective_type objective_type, const wyrmgus::quest *quest)
	: objective_type(objective_type), quest(quest), index(quest->get_objectives().size())
{
	if (objective_type == objective_type::hero_must_survive) {
		this->quantity = 0;
	}
}

void quest_objective::process_sml_property(const wyrmgus::sml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "quantity") {
		this->quantity = std::stoi(value);
	} else if (key == "objective_string") {
		this->objective_string = value;
	} else if (key == "settlement") {
		this->settlement = wyrmgus::site::get(value);
	} else if (key == "faction") {
		this->faction = wyrmgus::faction::get(value);
	} else if (key == "character") {
		this->character = wyrmgus::character::get(value);
	} else {
		throw std::runtime_error("Invalid quest objective property: \"" + key + "\".");
	}
}

void quest_objective::process_sml_scope(const wyrmgus::sml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "unit_classes") {
		for (const std::string &value : values) {
			this->unit_classes.push_back(wyrmgus::unit_class::get(value));
		}
	} else {
		throw std::runtime_error("Invalid quest objective scope: \"" + scope.get_tag() + "\".");
	}
}

quest::quest(const std::string &identifier) : detailed_data_entry(identifier)
{
}

quest::~quest()
{
}

void quest::process_sml_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "objectives") {
		scope.for_each_child([&](const sml_data &child_scope) {
			const objective_type objective_type = string_to_objective_type(child_scope.get_tag());

			auto objective = std::make_unique<quest_objective>(objective_type, this);
			database::process_sml_data(objective, child_scope);
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
		this->accept_effects = std::make_unique<effect_list>();
		database::process_sml_data(this->accept_effects, scope);
	} else if (tag == "completion_effects") {
		this->completion_effects = std::make_unique<effect_list>();
		database::process_sml_data(this->completion_effects, scope);
	} else {
		data_entry::process_sml_scope(scope);
	}
}

void quest::initialize()
{
	if (!this->Hidden && this->civilization != nullptr && std::find(this->civilization->Quests.begin(), this->civilization->Quests.end(), this) == this->civilization->Quests.end()) {
		this->civilization->Quests.push_back(this);
	}
}

std::string quest::get_rewards_string() const
{
	if (!this->rewards_string.empty()) {
		return "- " + this->rewards_string;
	}

	if (this->completion_effects != nullptr) {
		return this->completion_effects->get_effects_string();
	}

	return std::string();
}

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

void SetQuestCompleted(const std::string &quest_ident, int difficulty, bool save)
{
	wyrmgus::quest *quest = wyrmgus::quest::try_get(quest_ident);
	if (!quest) {
		return;
	}
	
	quest->Completed = true;
	if (difficulty > quest->HighestCompletedDifficulty) {
		quest->HighestCompletedDifficulty = difficulty;
	}
	if (save) {
		SaveQuestCompletion();
	}

	CAchievement::CheckAchievements();
}

void SetQuestCompleted(const std::string &quest_ident, bool save)
{
	SetQuestCompleted(quest_ident, 2, save);
}
