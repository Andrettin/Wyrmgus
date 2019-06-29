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

#include "quest/quest.h"

#include "character.h"
#include "dependency/and_dependency.h"
#include "faction.h"
#include "game/game.h"
#include "iocompat.h"
#include "iolib.h"
#include "item/item.h"
#include "item/unique_item.h"
#include "luacallback.h"
#include "map/site.h"
#include "parameters.h"
#include "player_color.h"
#include "quest/achievement.h"
#include "script.h"
#include "trigger/trigger_effect.h"
#include "ui/icon.h"
#include "unit/unit_class.h"
#include "unit/unit_type.h"
#include "upgrade/upgrade.h"

#include <ctype.h>

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

CQuest *CurrentQuest = nullptr;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

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

	for (const CAchievement *achievement : CAchievement::GetAll()) {
		if (achievement->IsObtained()) {
			fprintf(fd, "SetAchievementObtained(\"%s\", false, false)\n", achievement->Ident.c_str());
		}
	}
	
	fprintf(fd, "\n");
	
	for (const CQuest *quest : CQuest::GetAll()) {
		if (quest->IsCompleted()) {
			fprintf(fd, "SetQuestCompleted(\"%s\", %d, false)\n", quest->Ident.c_str(), quest->HighestCompletedDifficulty);
		}
	}
	
	fclose(fd);
}

std::string GetQuestObjectiveTypeNameById(int objective_type)
{
	if (objective_type == GatherResourceObjectiveType) {
		return "gather-resource";
	} else if (objective_type == HaveResourceObjectiveType) {
		return "have-resource";
	} else if (objective_type == BuildUnitsObjectiveType) {
		return "build-units";
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

CQuest::~CQuest()
{
	if (this->Conditions) {
		delete Conditions;
	}
	if (this->AcceptEffectsLua) {
		delete AcceptEffectsLua;
	}
	if (this->CompletionEffectsLua) {
		delete CompletionEffectsLua;
	}
	if (this->FailEffectsLua) {
		delete FailEffectsLua;
	}
	
	for (CQuestObjective *objective : this->Objectives) {
		delete objective;
	}
	
	for (CTriggerEffect *effect : this->AcceptEffects) {
		delete effect;
	}
	for (CTriggerEffect *effect : this->CompletionEffects) {
		delete effect;
	}
	for (CTriggerEffect *effect : this->FailEffects) {
		delete effect;
	}
	
	if (this->Predependency != nullptr) {
		delete this->Predependency;
	}
	if (this->Dependency != nullptr) {
		delete this->Dependency;
	}
}

/**
**	@brief	Process a section in the data provided by a configuration file
**
**	@param	section		The section
**
**	@return	True if the section can be processed, or false otherwise
*/
bool CQuest::ProcessConfigDataSection(const CConfigData *section)
{
	if (section->Tag == "accept_effects" || section->Tag == "completion_effects" || section->Tag == "fail_effects") {
		for (const CConfigData *subsection : section->Sections) {
			CTriggerEffect *trigger_effect = CTriggerEffect::FromConfigData(subsection);
			
			if (section->Tag == "accept_effects") {
				this->AcceptEffects.push_back(trigger_effect);
			} else if (section->Tag == "completion_effects") {
				this->CompletionEffects.push_back(trigger_effect);
			} else if (section->Tag == "fail_effects") {
				this->FailEffects.push_back(trigger_effect);
			}
		}
	} else if (section->Tag == "objectives") {
		for (const CConfigData *subsection : section->Sections) {
			CQuestObjective *objective = CQuestObjective::FromConfigData(subsection);
			objective->Quest = this;
			this->Objectives.push_back(objective);
		}
	} else if (section->Tag == "dependencies") {
		this->Dependency = new CAndDependency;
		this->Dependency->ProcessConfigData(section);
	} else {
		return false;
	}
	
	return true;
}

void CQuest::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("set_player_color", "player_color_ident"), +[](CQuest *quest, const String &player_color_ident){ quest->PlayerColor = CPlayerColor::Get(player_color_ident); });
	ClassDB::bind_method(D_METHOD("get_player_color"), +[](const CQuest *quest){ return const_cast<CPlayerColor *>(quest->GetPlayerColor()); });
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "player_color"), "set_player_color", "get_player_color");
	
	ClassDB::bind_method(D_METHOD("set_hint", "hint"), +[](CQuest *quest, const String &hint){ quest->Hint = hint; });
	ClassDB::bind_method(D_METHOD("get_hint"), &CQuest::GetHint);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "hint"), "set_hint", "get_hint");
	
	ClassDB::bind_method(D_METHOD("set_rewards_string", "rewards_string"), +[](CQuest *quest, const String &rewards_string){ quest->RewardsString = rewards_string; });
	ClassDB::bind_method(D_METHOD("get_rewards_string"), &CQuest::GetRewardsString);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "rewards_string"), "set_rewards_string", "get_rewards_string");
	
	ClassDB::bind_method(D_METHOD("set_repeatable", "repeatable"), +[](CQuest *quest, const bool repeatable){ quest->Repeatable = repeatable; });
	ClassDB::bind_method(D_METHOD("is_repeatable"), &CQuest::IsRepeatable);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "repeatable"), "set_repeatable", "is_repeatable");
	
	ClassDB::bind_method(D_METHOD("set_unobtainable", "unobtainable"), +[](CQuest *quest, const bool unobtainable){ quest->Unobtainable = unobtainable; });
	ClassDB::bind_method(D_METHOD("is_unobtainable"), &CQuest::IsUnobtainable);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "unobtainable"), "set_unobtainable", "is_unobtainable");
}

/**
**	@brief	Create a quest objective from config data
**
**	@param	config_data	The configuration data
*/
CQuestObjective *CQuestObjective::FromConfigData(const CConfigData *config_data)
{
	CQuestObjective *objective = new CQuestObjective;
	
	String objective_type_tag = config_data->Tag.replace("_", "-");
	objective->ObjectiveType = GetQuestObjectiveTypeIdByName(objective_type_tag.utf8().get_data());
	if (objective->ObjectiveType == -1) {
		fprintf(stderr, "Invalid quest objective type: \"%s\".\n", config_data->Tag.utf8().get_data());
		return objective;
	}
	
	if (objective->ObjectiveType == HeroMustSurviveObjectiveType) {
		objective->Quantity = 0;
	}
	
	objective->ProcessConfigData(config_data);
	
	return objective;
}

/**
**	@brief	Process data provided by a configuration file
**
**	@param	config_data	The configuration data
*/
void CQuestObjective::ProcessConfigData(const CConfigData *config_data)
{
	for (const CConfigProperty &property : config_data->Properties) {
		if (property.Operator != CConfigOperator::Assignment) {
			print_error("Wrong operator enumeration index for property \"" + property.Key + "\": " + String::num_int64(static_cast<int>(property.Operator)) + ".");
			continue;
		}
		
		if (property.Key == "objective_string") {
			this->ObjectiveString = property.Value.utf8().get_data();
		} else if (property.Key == "quantity") {
			this->Quantity = property.Value.to_int();
		} else if (property.Key == "resource") {
			const CResource *resource = CResource::Get(property.Value);
			if (resource != nullptr) {
				this->Resource = resource->GetIndex();
			}
		} else if (property.Key == "unit_class") {
			const ::UnitClass *unit_class = UnitClass::Get(property.Value);
			if (unit_class != nullptr) {
				this->UnitClasses.push_back(unit_class);
			}
		} else if (property.Key == "unit_type") {
			const CUnitType *unit_type = CUnitType::Get(property.Value);
			if (unit_type != nullptr) {
				this->UnitTypes.push_back(unit_type);
			}
		} else if (property.Key == "upgrade") {
			String value = property.Value.replace("_", "-");
			const CUpgrade *upgrade = CUpgrade::Get(value.utf8().get_data());
			if (upgrade != nullptr) {
				this->Upgrade = upgrade;
			}
		} else if (property.Key == "character") {
			const CCharacter *character = CCharacter::Get(property.Value);
			if (character != nullptr) {
				this->Character = character;
			}
		} else if (property.Key == "unique") {
			const UniqueItem *unique = UniqueItem::Get(property.Value);
			if (unique != nullptr) {
				this->Unique = unique;
			}
		} else if (property.Key == "site") {
			const CSite *site = CSite::Get(property.Value);
			if (site != nullptr) {
				this->Settlement = site;
			}
		} else if (property.Key == "faction") {
			const CFaction *faction = CFaction::Get(property.Value);
			if (faction != nullptr) {
				this->Faction = faction;
			}
		} else {
			fprintf(stderr, "Invalid quest objective property: \"%s\".\n", property.Key.utf8().get_data());
		}
	}
}

void SetCurrentQuest(const std::string &quest_ident)
{
	if (quest_ident.empty()) {
		CurrentQuest = nullptr;
	} else {
		CurrentQuest = CQuest::Get(quest_ident);
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
	CQuest *quest = CQuest::Get(quest_ident);
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
