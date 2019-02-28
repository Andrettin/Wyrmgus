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
/**@name achievement.cpp - The achievement source file. */
//
//      (c) Copyright 2017-2019 by Andrettin
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

#include "achievement.h"

#include "character.h"
#include "config.h"
#include "icon.h"
#include "player.h"
#include "quest.h"
#include "unit/unittype.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

std::vector<CAchievement *> CAchievement::Achievements;
std::map<std::string, CAchievement *> CAchievement::AchievementsByIdent;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Get an achievement
**
**	@param	ident	The achievement's string identifier
**
**	@return	The achievement if found, or null otherwise
*/
CAchievement *CAchievement::GetAchievement(const std::string &ident, const bool should_find)
{
	std::map<std::string, CAchievement *>::const_iterator find_iterator = CAchievement::AchievementsByIdent.find(ident);
	
	if (find_iterator != CAchievement::AchievementsByIdent.end()) {
		return find_iterator->second;
	}
	
	if (should_find) {
		fprintf(stderr, "Invalid achievement: \"%s\".\n", ident.c_str());
	}
	
	return nullptr;
}

/**
**	@brief	Get or add an achievement
**
**	@param	ident	The achievement's string identifier
**
**	@return	The achievement if found, otherwise a new achievement is created and returned
*/
CAchievement *CAchievement::GetOrAddAchievement(const std::string &ident)
{
	CAchievement *achievement = CAchievement::GetAchievement(ident, false);
	
	if (!achievement) {
		achievement = new CAchievement;
		achievement->Ident = ident;
		CAchievement::Achievements.push_back(achievement);
		CAchievement::AchievementsByIdent[ident] = achievement;
	}
	
	return achievement;
}

/**
**	@brief	Get the existing achievements
**
**	@return	The achievements
*/
const std::vector<CAchievement *> &CAchievement::GetAchievements()
{
	return CAchievement::Achievements;
}

/**
**	@brief	Remove the existing achievements
*/
void CAchievement::ClearAchievements()
{
	for (CAchievement *achievement : CAchievement::Achievements) {
		delete achievement;
	}
	CAchievement::Achievements.clear();
}

void CAchievement::CheckAchievements()
{
	for (CAchievement *achievement : CAchievement::Achievements) {
		if (achievement->IsObtained()) {
			continue;
		}
		
		if (achievement->CanObtain()) {
			achievement->Obtain();
		}
	}
}

/**
**	@brief	Process data provided by a configuration file
**
**	@param	config_data	The configuration data
*/
void CAchievement::ProcessConfigData(const CConfigData *config_data)
{
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "name") {
			this->Name = value;
		} else if (key == "description") {
			this->Description = value;
		} else if (key == "player_color") {
			value = FindAndReplaceString(value, "_", "-");
			const int color = GetPlayerColorIndexByName(value);
			if (color != -1) {
				this->PlayerColor = color;
			} else {
				fprintf(stderr, "Invalid player color: \"%s\".\n", value.c_str());
			}
		} else if (key == "character_level") {
			this->CharacterLevel = std::stoi(value);
		} else if (key == "difficulty") {
			this->Difficulty = std::stoi(value);
		} else if (key == "hidden") {
			this->Hidden = StringToBool(value);
		} else if (key == "unobtainable") {
			this->Unobtainable = StringToBool(value);
		} else if (key == "icon") {
			value = FindAndReplaceString(value, "_", "-");
			this->Icon.Name = value;
			this->Icon.Icon = nullptr;
			this->Icon.Load();
			this->Icon.Icon->Load();
		} else if (key == "character") {
			value = FindAndReplaceString(value, "_", "-");
			const CCharacter *character = CCharacter::GetCharacter(value);
			if (character) {
				this->Character = character;
			}
		} else if (key == "character_type") {
			value = FindAndReplaceString(value, "_", "-");
			const CUnitType *unit_type = UnitTypeByIdent(value);
			if (unit_type) {
				this->CharacterType = unit_type;
			} else {
				fprintf(stderr, "Unit type \"%s\" does not exist.\n", value.c_str());
			}
		} else if (key == "required_quest") {
			value = FindAndReplaceString(value, "_", "-");
			const CQuest *required_quest = GetQuest(value);
			if (required_quest) {
				this->RequiredQuests.push_back(required_quest);
			} else {
				fprintf(stderr, "Quest \"%s\" does not exist.\n", value.c_str());
			}
		} else {
			fprintf(stderr, "Invalid achievement property: \"%s\".\n", key.c_str());
		}
	}
}

void CAchievement::Obtain(const bool save, const bool display)
{
	if (this->IsObtained()) {
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
	if (this->IsObtained() || this->Unobtainable) {
		return false;
	}
	
	for (const CQuest *required_quest : this->RequiredQuests) {
		if (!required_quest->Completed || (this->Difficulty != -1 && required_quest->HighestCompletedDifficulty < this->Difficulty)) {
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
	
	for (const CQuest *required_quest : this->RequiredQuests) {
		if (required_quest->Completed && (this->Difficulty == -1 || required_quest->HighestCompletedDifficulty >= this->Difficulty)) {
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

void CAchievement::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("get_name"), &CAchievement::GetName);
	ClassDB::bind_method(D_METHOD("get_description"), &CAchievement::GetDescription);
	ClassDB::bind_method(D_METHOD("get_icon"), &CAchievement::GetIcon);
	ClassDB::bind_method(D_METHOD("is_hidden"), &CAchievement::IsHidden);
	ClassDB::bind_method(D_METHOD("is_obtained"), &CAchievement::IsObtained);
	ClassDB::bind_method(D_METHOD("get_progress"), &CAchievement::GetProgress);
	ClassDB::bind_method(D_METHOD("get_progress_max"), &CAchievement::GetProgressMax);
}

void SetAchievementObtained(const std::string &achievement_ident, const bool save, const bool display)
{
	CAchievement *achievement = CAchievement::GetAchievement(achievement_ident);
	if (!achievement) {
		return;
	}
	
	achievement->Obtain(save, display);
}
