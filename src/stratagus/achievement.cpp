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
//      (c) Copyright 2017-2020 by Andrettin
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

#include "achievement.h"

#include "character.h"
#include "config.h"
#include "player.h"
#include "player_color.h"
#include "quest.h"
#include "unit/unit_type.h"
#include "util/string_util.h"

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

const std::vector<CAchievement *> &CAchievement::GetAchievements()
{
	return CAchievement::Achievements;
}

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
		if (achievement->is_obtained()) {
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
			this->name = value;
		} else if (key == "description") {
			this->description = value;
		} else if (key == "player_color") {
			this->PlayerColor = stratagus::player_color::get(value);
		} else if (key == "character_level") {
			this->CharacterLevel = std::stoi(value);
		} else if (key == "difficulty") {
			this->Difficulty = std::stoi(value);
		} else if (key == "hidden") {
			this->hidden = string::to_bool(value);
		} else if (key == "unobtainable") {
			this->Unobtainable = string::to_bool(value);
		} else if (key == "icon") {
			value = FindAndReplaceString(value, "_", "-");
			this->Icon.Name = value;
			this->Icon.Icon = nullptr;
			this->Icon.Load();
		} else if (key == "character") {
			const stratagus::character *character = stratagus::character::get(value);
			this->Character = character;
		} else if (key == "character_type") {
			const stratagus::unit_type *unit_type = stratagus::unit_type::get(value);
			this->CharacterType = unit_type;
		} else if (key == "required_quest") {
			const stratagus::quest *required_quest = stratagus::quest::get(value);
			this->RequiredQuests.push_back(required_quest);
		} else {
			fprintf(stderr, "Invalid achievement property: \"%s\".\n", key.c_str());
		}
	}
}

bool CAchievement::CanObtain() const
{
	if (this->is_obtained() || this->Unobtainable) {
		return false;
	}

	for (const stratagus::quest *required_quest : this->RequiredQuests) {
		if (!required_quest->Completed || (this->Difficulty != -1 && required_quest->HighestCompletedDifficulty < this->Difficulty)) {
			return false;
		}
	}

	if (this->Character) {
		if (this->CharacterType && this->Character->get_unit_type() != this->CharacterType) {
			return false;
		}
		if (this->CharacterLevel && this->Character->get_level() < this->CharacterLevel) {
			return false;
		}
	} else if (this->CharacterType || this->CharacterLevel) {
		bool found_hero = false;
		for (std::map<std::string, stratagus::character *>::iterator iterator = CustomHeroes.begin(); iterator != CustomHeroes.end(); ++iterator) {
			if (this->CharacterType && iterator->second->get_unit_type() != this->CharacterType) {
				continue;
			}
			if (this->CharacterLevel && iterator->second->get_level() < this->CharacterLevel) {
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

void CAchievement::Obtain(const bool save, const bool display)
{
	if (this->is_obtained()) {
		return;
	}

	this->obtained = true;

	if (save) {
		SaveQuestCompletion();
	}

	if (display) {
		CclCommand("if (GenericDialog ~= nil) then GenericDialog(\"Achievement Unlocked!\", \"You have unlocked the " + this->get_name() + " achievement.\", nil, \"" + this->Icon.Name + "\", \"" + (this->PlayerColor ? this->PlayerColor->get_identifier() : "") + "\") end;");
	}
}

int CAchievement::GetProgress() const
{
	if (this->Unobtainable) {
		return 0;
	}

	int progress = 0;

	for (const stratagus::quest *required_quest : this->RequiredQuests) {
		if (required_quest->Completed && (this->Difficulty == -1 || required_quest->HighestCompletedDifficulty >= this->Difficulty)) {
			progress++;
		}
	}

	if (this->Character) {
		if (this->CharacterLevel) {
			progress += std::min(this->Character->get_level(), this->CharacterLevel);
		}
	} else if (this->CharacterLevel) {
		int highest_level = 0;
		for (std::map<std::string, stratagus::character *>::iterator iterator = CustomHeroes.begin(); iterator != CustomHeroes.end(); ++iterator) {
			highest_level = std::max(highest_level, iterator->second->get_level());
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

void SetAchievementObtained(const std::string &achievement_ident, const bool save, const bool display)
{
	CAchievement *achievement = CAchievement::GetAchievement(achievement_ident);
	if (!achievement) {
		return;
	}

	achievement->Obtain(save, display);
}
