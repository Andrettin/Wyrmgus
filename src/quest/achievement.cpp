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

#include "quest/achievement.h"

#include "character.h"
#include "config.h"
#include "player_color.h"
#include "quest/quest.h"
#include "script.h"
#include "ui/icon.h"
#include "unit/unit_type.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

void CAchievement::CheckAchievements()
{
	for (CAchievement *achievement : CAchievement::GetAll()) {
		if (achievement->IsObtained()) {
			continue;
		}
		
		if (achievement->CanObtain()) {
			achievement->Obtain();
		}
	}
}

/**
**	@brief	Process a property in the data provided by a configuration file
**
**	@param	key		The property's key
**	@param	value	The property's value
**
**	@return	True if the property can be processed, or false otherwise
*/
bool CAchievement::ProcessConfigDataProperty(const std::string &key, std::string value)
{
	if (key == "player_color") {
		value = FindAndReplaceString(value, "_", "-");
		CPlayerColor *player_color = CPlayerColor::Get(value);
		if (player_color != nullptr) {
			this->PlayerColor = player_color;
		}
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
		return false;
	}
	
	return true;
}
	
/**
**	@brief	Initialize the achievement
*/
void CAchievement::Initialize()
{
	if (!this->PlayerColor) {
		fprintf(stderr, "Achievement \"%s\" has no player color.\n", this->Ident.c_str());
	}
	
	this->Initialized = true;
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
		std::string lua_command = "if (GenericDialog ~= nil) then GenericDialog(\"Achievement Unlocked!\", \"You have unlocked the ";
		lua_command += this->Name.utf8().get_data();
		lua_command += " achievement.\", nil, \"";
		lua_command += this->Icon.Name;
		lua_command += "\", \"";
		lua_command += this->PlayerColor->GetIdent().utf8().get_data();
		lua_command += "\") end;";
		CclCommand(lua_command);
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
	BIND_PROPERTIES();
	
	ClassDB::bind_method(D_METHOD("get_icon"), &CAchievement::GetIcon);
	ClassDB::bind_method(D_METHOD("is_obtained"), &CAchievement::IsObtained);
	ClassDB::bind_method(D_METHOD("get_player_color"), &CAchievement::GetPlayerColor);
	ClassDB::bind_method(D_METHOD("get_progress"), &CAchievement::GetProgress);
	ClassDB::bind_method(D_METHOD("get_progress_max"), &CAchievement::GetProgressMax);
}

void SetAchievementObtained(const std::string &achievement_ident, const bool save, const bool display)
{
	CAchievement *achievement = CAchievement::Get(achievement_ident);
	if (!achievement) {
		return;
	}
	
	achievement->Obtain(save, display);
}
