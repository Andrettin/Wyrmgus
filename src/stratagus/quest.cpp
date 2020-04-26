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
#include "game.h"
#include "iocompat.h"
#include "iolib.h"
#include "luacallback.h"
#include "parameters.h"
#include "player.h"
#include "script.h"

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
		return "gather-resource";
	} else if (objective_type == ObjectiveType::HaveResource) {
		return "have-resource";
	} else if (objective_type == ObjectiveType::BuildUnits) {
		return "build-units";
	} else if (objective_type == ObjectiveType::BuildUnitsOfClass) {
		return "build-units-of-class";
	} else if (objective_type == ObjectiveType::DestroyUnits) {
		return "destroy-units";
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
	if (objective_type == "gather-resource") {
		return ObjectiveType::GatherResource;
	} else if (objective_type == "have-resource") {
		return ObjectiveType::HaveResource;
	} else if (objective_type == "build-units") {
		return ObjectiveType::BuildUnits;
	} else if (objective_type == "build-units-of-class") {
		return ObjectiveType::BuildUnitsOfClass;
	} else if (objective_type == "destroy-units") {
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

namespace stratagus {

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
