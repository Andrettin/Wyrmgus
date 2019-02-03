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
/**@name quest.cpp - The quest source file. */
//
//      (c) Copyright 2015-2019 by Andrettin
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

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "quest.h"

#include "character.h"
#include "game.h"
#include "iocompat.h"
#include "iolib.h"
#include "luacallback.h"
#include "parameters.h"
#include "player.h"
#include "script.h"

#include <ctype.h>

#include <string>
#include <map>

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

std::vector<CQuest *> Quests;
CQuest *CurrentQuest = nullptr;
std::vector<CAchievement *> Achievements;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

void CleanQuests()
{
	for (size_t i = 0; i < Quests.size(); ++i) {
		delete Quests[i];
	}
	Quests.clear();
	
	for (size_t i = 0; i < Achievements.size(); ++i) {
		delete Achievements[i];
	}
	Achievements.clear();
}

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

	for (size_t i = 0; i < Achievements.size(); ++i) {
		if (Achievements[i]->Obtained) {
			fprintf(fd, "SetAchievementObtained(\"%s\", false, false)\n", Achievements[i]->Ident.c_str());
		}
	}
	
	fprintf(fd, "\n");
	
	for (size_t i = 0; i < Quests.size(); ++i) {
		if (Quests[i]->Completed) {
			fprintf(fd, "SetQuestCompleted(\"%s\", %d, false)\n", Quests[i]->Ident.c_str(), Quests[i]->HighestCompletedDifficulty);
		}
	}
	
	fclose(fd);
}

void CheckAchievements()
{
	for (size_t i = 0; i < Achievements.size(); ++i) {
		if (Achievements[i]->Obtained) {
			continue;
		}
		
		if (Achievements[i]->CanObtain()) {
			Achievements[i]->Obtain();
		}
	}
}

std::string GetQuestObjectiveTypeNameById(int objective_type)
{
	if (objective_type == GatherResourceObjectiveType) {
		return "gather-resource";
	} else if (objective_type == HaveResourceObjectiveType) {
		return "have-resource";
	} else if (objective_type == BuildUnitsObjectiveType) {
		return "build-units";
	} else if (objective_type == BuildUnitsOfClassObjectiveType) {
		return "build-units-of-class";
	} else if (objective_type == DestroyUnitsObjectiveType) {
		return "destroy-units";
	} else if (objective_type == ResearchUpgradeObjectiveType) {
		return "research-upgrade";
	} else if (objective_type == RecruitHeroObjectiveType) {
		return "recruit-hero";
	} else if (objective_type == DestroyHeroObjectiveType) {
		return "destroy-hero";
	} else if (objective_type == HeroMustSurviveObjectiveType) {
		return "hero-must-survive";
	} else if (objective_type == DestroyUniqueObjectiveType) {
		return "destroy-unique";
	} else if (objective_type == DestroyFactionObjectiveType) {
		return "destroy-faction";
	}

	return "";
}

int GetQuestObjectiveTypeIdByName(const std::string &objective_type)
{
	if (objective_type == "gather-resource") {
		return GatherResourceObjectiveType;
	} else if (objective_type == "have-resource") {
		return HaveResourceObjectiveType;
	} else if (objective_type == "build-units") {
		return BuildUnitsObjectiveType;
	} else if (objective_type == "build-units-of-class") {
		return BuildUnitsOfClassObjectiveType;
	} else if (objective_type == "destroy-units") {
		return DestroyUnitsObjectiveType;
	} else if (objective_type == "research-upgrade") {
		return ResearchUpgradeObjectiveType;
	} else if (objective_type == "recruit-hero") {
		return RecruitHeroObjectiveType;
	} else if (objective_type == "destroy-hero") {
		return DestroyHeroObjectiveType;
	} else if (objective_type == "hero-must-survive") {
		return HeroMustSurviveObjectiveType;
	} else if (objective_type == "destroy-unique") {
		return DestroyUniqueObjectiveType;
	} else if (objective_type == "destroy-faction") {
		return DestroyFactionObjectiveType;
	}

	return -1;
}

CQuest *GetQuest(const std::string &quest_ident)
{
	for (size_t i = 0; i < Quests.size(); ++i) {
		if (quest_ident == Quests[i]->Ident) {
			return Quests[i];
		}
	}
	
	for (size_t i = 0; i < Quests.size(); ++i) { // for backwards compatibility
		if (NameToIdent(quest_ident) == Quests[i]->Ident) {
			return Quests[i];
		}
	}
	
	return nullptr;
}

CAchievement *GetAchievement(const std::string &achievement_ident)
{
	for (size_t i = 0; i < Achievements.size(); ++i) {
		if (achievement_ident == Achievements[i]->Ident) {
			return Achievements[i];
		}
	}
	
	return nullptr;
}

CQuest::~CQuest()
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

void CAchievement::Obtain(bool save, bool display)
{
	if (this->Obtained) {
		return;
	}
	this->Obtained = true;
	
	if (save) {
		SaveQuestCompletion();
	}
	
	if (display) {
		CclCommand("if (GenericDialog ~= nil) then GenericDialog(\"Achievement Unlocked!\", \"You have unlocked the " + this->Name + " achievement.\", nil, \"" + this->Icon.Name + "\", \"" + PlayerColorNames[this->PlayerColor] + "\") end;");
	}
}

bool CAchievement::CanObtain() const
{
	if (this->Obtained || this->Unobtainable) {
		return false;
	}
	
	for (size_t i = 0; i < this->RequiredQuests.size(); ++i) {
		if (!this->RequiredQuests[i]->Completed || (this->Difficulty != -1 && this->RequiredQuests[i]->HighestCompletedDifficulty < this->Difficulty)) {
			return false;
		}
	}
	
	if (this->Character) {
		if (this->CharacterType && this->Character->Type != this->CharacterType) {
			return false;
		}
		if (this->CharacterLevel && this->Character->Level < this->CharacterLevel) {
			return false;
		}
	} else if (this->CharacterType || this->CharacterLevel) {
		bool found_hero = false;
		for (std::map<std::string, CCharacter *>::iterator iterator = CustomHeroes.begin(); iterator != CustomHeroes.end(); ++iterator) {
			if (this->CharacterType && iterator->second->Type != this->CharacterType) {
				continue;
			}
			if (this->CharacterLevel && iterator->second->Level < this->CharacterLevel) {
				continue;
			}
			found_hero = true;
			break;
		}
		if (!found_hero) {
			return false;
		}
	}
	
	return true;
}

int CAchievement::GetProgress() const
{
	if (this->Unobtainable) {
		return 0;
	}
	
	int progress = 0;
	
	for (size_t i = 0; i < this->RequiredQuests.size(); ++i) {
		if (this->RequiredQuests[i]->Completed && (this->Difficulty == -1 || this->RequiredQuests[i]->HighestCompletedDifficulty >= this->Difficulty)) {
			progress++;
		}
	}
	
	if (this->Character) {
		if (this->CharacterLevel) {
			progress += std::min(this->Character->Level, this->CharacterLevel);
		}
	} else if (this->CharacterLevel) {
		int highest_level = 0;
		for (std::map<std::string, CCharacter *>::iterator iterator = CustomHeroes.begin(); iterator != CustomHeroes.end(); ++iterator) {
			highest_level = std::max(highest_level, iterator->second->Level);
			if (highest_level >= this->CharacterLevel) {
				highest_level = this->CharacterLevel;
				break;
				continue;
			}
		}
		progress += highest_level;
	}
	
	return progress;
}

int CAchievement::GetProgressMax() const
{
	if (this->Unobtainable) {
		return 0;
	}
	
	int progress_max = 0;
	progress_max += this->RequiredQuests.size();
	progress_max += this->CharacterLevel;
	
	return progress_max;
}

void SetCurrentQuest(const std::string &quest_ident)
{
	if (quest_ident.empty()) {
		CurrentQuest = nullptr;
	} else {
		CurrentQuest = GetQuest(quest_ident);
	}
}

std::string GetCurrentQuest()
{
	if (!CurrentQuest) {
		return "";
	} else {
		return CurrentQuest->Ident;
	}
}

void SetQuestCompleted(const std::string &quest_ident, int difficulty, bool save)
{
	CQuest *quest = GetQuest(quest_ident);
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
	CheckAchievements();
}

void SetQuestCompleted(const std::string &quest_ident, bool save)
{
	SetQuestCompleted(quest_ident, 2, save);
}

void SetAchievementObtained(const std::string &achievement_ident, bool save, bool display)
{
	CAchievement *achievement = GetAchievement(achievement_ident);
	if (!achievement) {
		return;
	}
	
	achievement->Obtain(save, display);
}
