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
/**@name script_quest.cpp - The quest ccl functions. */
//
//      (c) Copyright 2015-2016 by Andrettin
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

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "quest.h"

#include "character.h"
#include "luacallback.h"
#include "player.h"
#include "script.h"
#include "unittype.h"
#include "upgrade.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**  Define a quest.
**
**  @param l  Lua state.
*/
static int CclDefineQuest(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string quest_ident = LuaToString(l, 1);
	CQuest *quest = GetQuest(quest_ident);
	if (!quest) {
		quest = new CQuest;
		Quests.push_back(quest);
		quest->Ident = quest_ident;
	}
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Name")) {
			quest->Name = LuaToString(l, -1);
		} else if (!strcmp(value, "Description")) {
			quest->Description = LuaToString(l, -1);
		} else if (!strcmp(value, "World")) {
			quest->World = LuaToString(l, -1);
		} else if (!strcmp(value, "Map")) {
			quest->Map = LuaToString(l, -1);
		} else if (!strcmp(value, "Scenario")) {
			quest->Scenario = LuaToString(l, -1);
		} else if (!strcmp(value, "RequiredQuest")) {
			quest->RequiredQuest = LuaToString(l, -1);
		} else if (!strcmp(value, "RequiredTechnology")) {
			quest->RequiredTechnology = LuaToString(l, -1);
		} else if (!strcmp(value, "Area")) {
			quest->Area = LuaToString(l, -1);
		} else if (!strcmp(value, "Briefing")) {
			quest->Briefing = LuaToString(l, -1);
		} else if (!strcmp(value, "BriefingBackground")) {
			quest->BriefingBackground = LuaToString(l, -1);
		} else if (!strcmp(value, "BriefingMusic")) {
			quest->BriefingMusic = LuaToString(l, -1);
		} else if (!strcmp(value, "LoadingMusic")) {
			quest->LoadingMusic = LuaToString(l, -1);
		} else if (!strcmp(value, "MapMusic")) {
			quest->MapMusic = LuaToString(l, -1);
		} else if (!strcmp(value, "StartSpeech")) {
			quest->StartSpeech = LuaToString(l, -1);
		} else if (!strcmp(value, "InProgressSpeech")) {
			quest->InProgressSpeech = LuaToString(l, -1);
		} else if (!strcmp(value, "CompletionSpeech")) {
			quest->CompletionSpeech = LuaToString(l, -1);
		} else if (!strcmp(value, "Civilization")) {
			quest->Civilization = PlayerRaces.GetRaceIndexByName(LuaToString(l, -1));
		} else if (!strcmp(value, "TechnologyPoints")) {
			quest->TechnologyPoints = LuaToNumber(l, -1);
		} else if (!strcmp(value, "PlayerColor")) {
			std::string color_name = LuaToString(l, -1);
			int color = GetPlayerColorIndexByName(color_name);
			if (color != -1) {
				quest->PlayerColor = color;
			} else {
				LuaError(l, "Player color \"%s\" doesn't exist." _C_ color_name.c_str());
			}
		} else if (!strcmp(value, "Hidden")) {
			quest->Hidden = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Icon")) {
			quest->Icon.Name = LuaToString(l, -1);
			quest->Icon.Icon = NULL;
			quest->Icon.Load();
			quest->Icon.Icon->Load();
		} else if (!strcmp(value, "QuestGiver")) {
			std::string quest_giver_name = TransliterateText(LuaToString(l, -1));
			CCharacter *quest_giver = GetCharacter(quest_giver_name);
			if (quest_giver) {
				quest->QuestGiver = quest_giver;
			} else {
				LuaError(l, "Character \"%s\" doesn't exist." _C_ quest_giver_name.c_str());
			}
		} else if (!strcmp(value, "IntroductionDialogue")) {
			std::string dialogue_ident = LuaToString(l, -1);
			CDialogue *dialogue = GetDialogue(dialogue_ident);
			if (dialogue) {
				quest->IntroductionDialogue = dialogue;
			} else {
				LuaError(l, "Dialogue \"%s\" doesn't exist." _C_ dialogue_ident.c_str());
			}
		} else if (!strcmp(value, "Objectives")) {
			quest->Objectives.clear();
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				quest->Objectives.push_back(LuaToString(l, -1, j + 1));
			}
		} else if (!strcmp(value, "BriefingSounds")) {
			quest->BriefingSounds.clear();
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				quest->BriefingSounds.push_back(LuaToString(l, -1, j + 1));
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	return 0;
}

static int CclGetQuests(lua_State *l)
{
	lua_createtable(l, Quests.size(), 0);
	for (size_t i = 1; i <= Quests.size(); ++i)
	{
		lua_pushstring(l, Quests[i-1]->Ident.c_str());
		lua_rawseti(l, -2, i);
	}
	return 1;
}

/**
**  Get quest data.
**
**  @param l  Lua state.
*/
static int CclGetQuestData(lua_State *l)
{
	if (lua_gettop(l) < 2) {
		LuaError(l, "incorrect argument");
	}
	std::string quest_ident = LuaToString(l, 1);
	const CQuest *quest = GetQuest(quest_ident);
	if (!quest) {
		LuaError(l, "Quest \"%s\" doesn't exist." _C_ quest_ident.c_str());
	}
	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Name")) {
		lua_pushstring(l, quest->Name.c_str());
		return 1;
	} else if (!strcmp(data, "Description")) {
		lua_pushstring(l, quest->Description.c_str());
		return 1;
	} else if (!strcmp(data, "World")) {
		lua_pushstring(l, quest->World.c_str());
		return 1;
	} else if (!strcmp(data, "Map")) {
		lua_pushstring(l, quest->Map.c_str());
		return 1;
	} else if (!strcmp(data, "Scenario")) {
		lua_pushstring(l, quest->Scenario.c_str());
		return 1;
	} else if (!strcmp(data, "RequiredQuest")) {
		lua_pushstring(l, quest->RequiredQuest.c_str());
		return 1;
	} else if (!strcmp(data, "RequiredTechnology")) {
		lua_pushstring(l, quest->RequiredTechnology.c_str());
		return 1;
	} else if (!strcmp(data, "Area")) {
		lua_pushstring(l, quest->Area.c_str());
		return 1;
	} else if (!strcmp(data, "Briefing")) {
		lua_pushstring(l, quest->Briefing.c_str());
		return 1;
	} else if (!strcmp(data, "BriefingBackground")) {
		lua_pushstring(l, quest->BriefingBackground.c_str());
		return 1;
	} else if (!strcmp(data, "BriefingMusic")) {
		lua_pushstring(l, quest->BriefingMusic.c_str());
		return 1;
	} else if (!strcmp(data, "LoadingMusic")) {
		lua_pushstring(l, quest->LoadingMusic.c_str());
		return 1;
	} else if (!strcmp(data, "MapMusic")) {
		lua_pushstring(l, quest->MapMusic.c_str());
		return 1;
	} else if (!strcmp(data, "StartSpeech")) {
		lua_pushstring(l, quest->StartSpeech.c_str());
		return 1;
	} else if (!strcmp(data, "InProgressSpeech")) {
		lua_pushstring(l, quest->InProgressSpeech.c_str());
		return 1;
	} else if (!strcmp(data, "CompletionSpeech")) {
		lua_pushstring(l, quest->CompletionSpeech.c_str());
		return 1;
	} else if (!strcmp(data, "Civilization")) {
		if (quest->Civilization != -1) {
			lua_pushstring(l, PlayerRaces.Name[quest->Civilization].c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "TechnologyPoints")) {
		lua_pushnumber(l, quest->TechnologyPoints);
		return 1;
	} else if (!strcmp(data, "PlayerColor")) {
		lua_pushstring(l, PlayerColorNames[quest->PlayerColor].c_str());
		return 1;
	} else if (!strcmp(data, "Hidden")) {
		lua_pushboolean(l, quest->Hidden);
		return 1;
	} else if (!strcmp(data, "Completed")) {
		lua_pushboolean(l, quest->Completed);
		return 1;
	} else if (!strcmp(data, "HighestCompletedDifficulty")) {
		lua_pushnumber(l, quest->HighestCompletedDifficulty);
		return 1;
	} else if (!strcmp(data, "Icon")) {
		lua_pushstring(l, quest->Icon.Name.c_str());
		return 1;
	} else if (!strcmp(data, "QuestGiver")) {
		lua_pushstring(l, quest->QuestGiver->GetFullName().c_str());
		return 1;
	} else if (!strcmp(data, "Objectives")) {
		lua_createtable(l, quest->Objectives.size(), 0);
		for (size_t i = 1; i <= quest->Objectives.size(); ++i)
		{
			lua_pushstring(l, quest->Objectives[i-1].c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "BriefingSounds")) {
		lua_createtable(l, quest->BriefingSounds.size(), 0);
		for (size_t i = 1; i <= quest->BriefingSounds.size(); ++i)
		{
			lua_pushstring(l, quest->BriefingSounds[i-1].c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else {
		LuaError(l, "Invalid field: %s" _C_ data);
	}

	return 0;
}

/**
**  Define an achievement.
**
**  @param l  Lua state.
*/
static int CclDefineAchievement(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string achievement_ident = LuaToString(l, 1);
	CAchievement *achievement = GetAchievement(achievement_ident);
	if (!achievement) {
		achievement = new CAchievement;
		Achievements.push_back(achievement);
		achievement->Ident = achievement_ident;
	}
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Name")) {
			achievement->Name = LuaToString(l, -1);
		} else if (!strcmp(value, "Description")) {
			achievement->Description = LuaToString(l, -1);
		} else if (!strcmp(value, "PlayerColor")) {
			std::string color_name = LuaToString(l, -1);
			int color = GetPlayerColorIndexByName(color_name);
			if (color != -1) {
				achievement->PlayerColor = color;
			} else {
				LuaError(l, "Player color \"%s\" doesn't exist." _C_ color_name.c_str());
			}
		} else if (!strcmp(value, "CharacterLevel")) {
			achievement->CharacterLevel = LuaToNumber(l, -1);
		} else if (!strcmp(value, "Difficulty")) {
			achievement->Difficulty = LuaToNumber(l, -1);
		} else if (!strcmp(value, "Hidden")) {
			achievement->Hidden = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Unobtainable")) {
			achievement->Unobtainable = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Icon")) {
			achievement->Icon.Name = LuaToString(l, -1);
			achievement->Icon.Icon = NULL;
			achievement->Icon.Load();
			achievement->Icon.Icon->Load();
		} else if (!strcmp(value, "Character")) {
			std::string character_name = TransliterateText(LuaToString(l, -1));
			CCharacter *character = GetCharacter(character_name);
			if (character) {
				achievement->Character = character;
			} else {
				LuaError(l, "Character \"%s\" doesn't exist." _C_ character_name.c_str());
			}
		} else if (!strcmp(value, "CharacterType")) {
			std::string unit_type_ident = LuaToString(l, -1);
			int unit_type_id = UnitTypeIdByIdent(unit_type_ident);
			if (unit_type_id != -1) {
				achievement->CharacterType = UnitTypes[unit_type_id];
			} else {
				LuaError(l, "Unit type \"%s\" doesn't exist." _C_ unit_type_ident.c_str());
			}
		} else if (!strcmp(value, "RequiredQuests")) {
			achievement->RequiredQuests.clear();
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				std::string quest_ident = LuaToString(l, -1, j + 1);
				CQuest *required_quest = GetQuest(quest_ident);
				if (required_quest) {
					achievement->RequiredQuests.push_back(required_quest);
				} else {
					LuaError(l, "Quest \"%s\" doesn't exist." _C_ quest_ident.c_str());
				}
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	return 0;
}

static int CclGetAchievements(lua_State *l)
{
	lua_createtable(l, Achievements.size(), 0);
	for (size_t i = 1; i <= Achievements.size(); ++i)
	{
		lua_pushstring(l, Achievements[i-1]->Ident.c_str());
		lua_rawseti(l, -2, i);
	}
	return 1;
}

/**
**  Get achievement data.
**
**  @param l  Lua state.
*/
static int CclGetAchievementData(lua_State *l)
{
	if (lua_gettop(l) < 2) {
		LuaError(l, "incorrect argument");
	}
	std::string achievement_ident = LuaToString(l, 1);
	const CAchievement *achievement = GetAchievement(achievement_ident);
	if (!achievement) {
		LuaError(l, "Achievement \"%s\" doesn't exist." _C_ achievement_ident.c_str());
	}
	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Name")) {
		lua_pushstring(l, achievement->Name.c_str());
		return 1;
	} else if (!strcmp(data, "Description")) {
		lua_pushstring(l, achievement->Description.c_str());
		return 1;
	} else if (!strcmp(data, "PlayerColor")) {
		lua_pushstring(l, PlayerColorNames[achievement->PlayerColor].c_str());
		return 1;
	} else if (!strcmp(data, "Character")) {
		if (achievement->Character) {
			lua_pushstring(l, achievement->Character->GetFullName().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "CharacterType")) {
		if (achievement->CharacterType) {
			lua_pushstring(l, achievement->CharacterType->Ident.c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "CharacterLevel")) {
		lua_pushnumber(l, achievement->CharacterLevel);
		return 1;
	} else if (!strcmp(data, "Difficulty")) {
		lua_pushnumber(l, achievement->Difficulty);
		return 1;
	} else if (!strcmp(data, "Hidden")) {
		lua_pushboolean(l, achievement->Hidden);
		return 1;
	} else if (!strcmp(data, "Obtained")) {
		lua_pushboolean(l, achievement->Obtained);
		return 1;
	} else if (!strcmp(data, "Unobtainable")) {
		lua_pushboolean(l, achievement->Unobtainable);
		return 1;
	} else if (!strcmp(data, "Icon")) {
		lua_pushstring(l, achievement->Icon.Name.c_str());
		return 1;
	} else {
		LuaError(l, "Invalid field: %s" _C_ data);
	}

	return 0;
}

/**
**  Define a dialogue.
**
**  @param l  Lua state.
*/
static int CclDefineDialogue(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string dialogue_name = LuaToString(l, 1);
	CDialogue *dialogue = GetDialogue(dialogue_name);
	if (!dialogue) {
		dialogue = new CDialogue;
		Dialogues.push_back(dialogue);
		dialogue->Ident = dialogue_name;
	}
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Nodes")) {
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				lua_rawgeti(l, -1, j + 1);
				CDialogueNode *node = new CDialogueNode;
				node->ID = dialogue->Nodes.size();
				dialogue->Nodes.push_back(node);
				node->Dialogue = dialogue;
				if (!lua_istable(l, -1)) {
					LuaError(l, "incorrect argument (expected table for dialogue nodes)");
				}
				const int subargs = lua_rawlen(l, -1);
				for (int k = 0; k < subargs; ++k) {
					value = LuaToString(l, -1, k + 1);
					++k;
					if (!strcmp(value, "speaker")) {
						node->SpeakerType = LuaToString(l, -1, k + 1);
						++k;
						node->Speaker = LuaToString(l, -1, k + 1);
					} else if (!strcmp(value, "text")) {
						node->Text = LuaToString(l, -1, k + 1);
					} else if (!strcmp(value, "conditions")) {
						lua_rawgeti(l, -1, k + 1);
						node->Conditions = new LuaCallback(l, -1);
						lua_pop(l, 1);
					} else if (!strcmp(value, "option-effects")) {
						lua_rawgeti(l, -1, k + 1);
						const int subsubargs = lua_rawlen(l, -1);
						for (int n = 0; n < subsubargs; ++n) {
							lua_rawgeti(l, -1, n + 1);
							node->OptionEffects.push_back(new LuaCallback(l, -1));
							lua_pop(l, 1);
						}
						lua_pop(l, 1);
					} else {
						printf("\n%s\n", dialogue->Ident.c_str());
						LuaError(l, "Unsupported tag: %s" _C_ value);
					}
				}
				lua_pop(l, 1);
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	return 0;
}

// ----------------------------------------------------------------------------

/**
**  Register CCL features for quests.
*/
void QuestCclRegister()
{
	lua_register(Lua, "DefineQuest", CclDefineQuest);
	lua_register(Lua, "GetQuests", CclGetQuests);
	lua_register(Lua, "GetQuestData", CclGetQuestData);
	lua_register(Lua, "DefineAchievement", CclDefineAchievement);
	lua_register(Lua, "GetAchievements", CclGetAchievements);
	lua_register(Lua, "GetAchievementData", CclGetAchievementData);
	lua_register(Lua, "DefineDialogue", CclDefineDialogue);
}

//@}
