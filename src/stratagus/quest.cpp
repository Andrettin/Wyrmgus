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
#include "parameters.h"
#include "player.h"
#include "player_color.h"
#include "script.h"
#include "unit/unit_class.h"

stratagus::quest *CurrentQuest = nullptr;

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
	
	for (const stratagus::quest *quest : stratagus::quest::get_all()) {
		if (quest->Completed) {
			fprintf(fd, "SetQuestCompleted(\"%s\", %d, false)\n", quest->get_identifier().c_str(), quest->HighestCompletedDifficulty);
		}
	}
	
	fclose(fd);
}

std::string GetQuestObjectiveTypeNameById(const ObjectiveType objective_type)
{
	if (objective_type == ObjectiveType::GatherResource) {
		return "gather_resource";
	} else if (objective_type == ObjectiveType::HaveResource) {
		return "have_resource";
	} else if (objective_type == ObjectiveType::BuildUnits) {
		return "build-units";
	} else if (objective_type == ObjectiveType::BuildUnitsOfClass) {
		return "build-units-of-class";
	} else if (objective_type == ObjectiveType::DestroyUnits) {
		return "destroy_units";
	} else if (objective_type == ObjectiveType::ResearchUpgrade) {
		return "research-upgrade";
	} else if (objective_type == ObjectiveType::RecruitHero) {
		return "recruit-hero";
	} else if (objective_type == ObjectiveType::DestroyHero) {
		return "destroy-hero";
	} else if (objective_type == ObjectiveType::HeroMustSurvive) {
		return "hero-must-survive";
	} else if (objective_type == ObjectiveType::DestroyUnique) {
		return "destroy-unique";
	} else if (objective_type == ObjectiveType::DestroyFaction) {
		return "destroy-faction";
	}

	return "";
}

ObjectiveType GetQuestObjectiveTypeIdByName(const std::string &objective_type)
{
	if (objective_type == "gather_resource") {
		return ObjectiveType::GatherResource;
	} else if (objective_type == "have_resource") {
		return ObjectiveType::HaveResource;
	} else if (objective_type == "build-units") {
		return ObjectiveType::BuildUnits;
	} else if (objective_type == "build-units-of-class") {
		return ObjectiveType::BuildUnitsOfClass;
	} else if (objective_type == "destroy_units") {
		return ObjectiveType::DestroyUnits;
	} else if (objective_type == "research-upgrade") {
		return ObjectiveType::ResearchUpgrade;
	} else if (objective_type == "recruit-hero") {
		return ObjectiveType::RecruitHero;
	} else if (objective_type == "destroy-hero") {
		return ObjectiveType::DestroyHero;
	} else if (objective_type == "hero-must-survive") {
		return ObjectiveType::HeroMustSurvive;
	} else if (objective_type == "destroy-unique") {
		return ObjectiveType::DestroyUnique;
	} else if (objective_type == "destroy-faction") {
		return ObjectiveType::DestroyFaction;
	}

	return ObjectiveType::None;
}


void CQuestObjective::process_sml_property(const stratagus::sml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "objective_string") {
		this->objective_string = value;
	} else if (key == "settlement") {
		this->settlement = stratagus::site::get(value);
	} else if (key == "faction") {
		this->faction = stratagus::faction::get(value);
	} else {
		throw std::runtime_error("Invalid quest objective property: \"" + key + "\".");
	}
}

void CQuestObjective::process_sml_scope(const stratagus::sml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "unit_classes") {
		for (const std::string &value : values) {
			this->unit_classes.push_back(stratagus::unit_class::get(value));
		}
	} else {
		throw std::runtime_error("Invalid quest objective scope: \"" + scope.get_tag() + "\".");
	}
}

namespace stratagus {

quest::quest(const std::string &identifier) : detailed_data_entry(identifier)
{
}

quest::~quest()
{
	if (this->Conditions) {
		delete Conditions;
	}
	if (this->AcceptEffects) {
		delete AcceptEffects;
	}
	if (this->CompletionEffects) {
		delete CompletionEffects;
	}
	if (this->FailEffects) {
		delete FailEffects;
	}
	for (size_t i = 0; i < this->Objectives.size(); ++i) {
		delete this->Objectives[i];
	}
}

void quest::process_sml_property(const sml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "player_color") {
		this->PlayerColor = player_color::get(value);
	} else {
		data_entry::process_sml_property(property);
	}
}

void quest::process_sml_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "objectives") {
		scope.for_each_child([&](const sml_data &child_scope) {
			CQuestObjective *objective = new CQuestObjective(this->Objectives.size());
			objective->Quest = this;
			this->Objectives.push_back(objective);

			objective->ObjectiveType = GetQuestObjectiveTypeIdByName(child_scope.get_tag());
			if (objective->ObjectiveType == ObjectiveType::None) {
				throw std::runtime_error("Objective type doesn't exist.");
			}
			if (objective->ObjectiveType == ObjectiveType::HeroMustSurvive) {
				objective->Quantity = 0;
			}

			database::process_sml_data(objective, child_scope);
		});
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

}

void SetCurrentQuest(const std::string &quest_ident)
{
	if (quest_ident.empty()) {
		CurrentQuest = nullptr;
	} else {
		CurrentQuest = stratagus::quest::get(quest_ident);
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
	stratagus::quest *quest = stratagus::quest::try_get(quest_ident);
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
