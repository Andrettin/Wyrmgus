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
//      (c) Copyright 2015-2017 by Andrettin
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
		quest->ID = Quests.size();
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
		} else if (!strcmp(value, "Rewards")) {
			quest->Rewards = LuaToString(l, -1);
		} else if (!strcmp(value, "Hint")) {
			quest->Hint = LuaToString(l, -1);
		} else if (!strcmp(value, "Civilization")) {
			quest->Civilization = PlayerRaces.GetRaceIndexByName(LuaToString(l, -1));
		} else if (!strcmp(value, "PlayerColor")) {
			std::string color_name = LuaToString(l, -1);
			int color = GetPlayerColorIndexByName(color_name);
			if (color != -1) {
				quest->PlayerColor = color;
			} else {
				LuaError(l, "Player color \"%s\" doesn't exist." _C_ color_name.c_str());
			}
		} else if (!strcmp(value, "HairColor")) {
			quest->HairColor = GetHairColorIndexByName(LuaToString(l, -1));
		} else if (!strcmp(value, "Hidden")) {
			quest->Hidden = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Competitive")) {
			quest->Competitive = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Unobtainable")) {
			quest->Unobtainable = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Uncompleteable")) {
			quest->Uncompleteable = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Unfailable")) {
			quest->Unfailable = LuaToBoolean(l, -1);
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
		} else if (!strcmp(value, "Conditions")) {
			quest->Conditions = new LuaCallback(l, -1);
		} else if (!strcmp(value, "AcceptEffects")) {
			quest->AcceptEffects = new LuaCallback(l, -1);
		} else if (!strcmp(value, "CompletionEffects")) {
			quest->CompletionEffects = new LuaCallback(l, -1);
		} else if (!strcmp(value, "FailEffects")) {
			quest->FailEffects = new LuaCallback(l, -1);
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
		// objective types
		} else if (!strcmp(value, "BuildUnits")) {
			quest->BuildUnits.clear();
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				CUnitType *unit_type = UnitTypeByIdent(LuaToString(l, -1, j + 1));
				if (!unit_type) {
					LuaError(l, "Unit type doesn't exist.");
				}
				++j;
				
				int quantity = LuaToNumber(l, -1, j + 1);
				
				quest->BuildUnits.push_back(std::tuple<CUnitType *, int>(unit_type, quantity));
			}
		} else if (!strcmp(value, "BuildUnitsOfClass")) {
			quest->BuildUnitsOfClass.clear();
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				int class_id = GetUnitTypeClassIndexByName(LuaToString(l, -1, j + 1));
				if (class_id == -1) {
					LuaError(l, "Unit type class doesn't exist.");
				}
				++j;
				
				int quantity = LuaToNumber(l, -1, j + 1);
				
				quest->BuildUnitsOfClass.push_back(std::tuple<int, int>(class_id, quantity));
			}
		} else if (!strcmp(value, "ResearchUpgrades")) {
			quest->ResearchUpgrades.clear();
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				CUpgrade *upgrade = CUpgrade::Get(LuaToString(l, -1, j + 1));
				if (!upgrade) {
					LuaError(l, "Upgrade doesn't exist.");
				}
				
				quest->ResearchUpgrades.push_back(upgrade);
			}
		} else if (!strcmp(value, "DestroyUnits")) {
			quest->DestroyUnits.clear();
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				CUnitType *unit_type = UnitTypeByIdent(LuaToString(l, -1, j + 1));
				if (!unit_type) {
					LuaError(l, "Unit type doesn't exist.");
				}
				++j;
				
				std::string faction_ident = LuaToString(l, -1, j + 1);
				CFaction *faction = PlayerRaces.GetFaction(faction_ident);
				if (!faction && !faction_ident.empty()) {
					LuaError(l, "Faction \"%s\" doesn't exist." _C_ faction_ident.c_str());
				}
				++j;
				
				int quantity = LuaToNumber(l, -1, j + 1);
				
				quest->DestroyUnits.push_back(std::tuple<CUnitType *, CFaction *, int>(unit_type, faction, quantity));
			}
		} else if (!strcmp(value, "DestroyUniques")) {
			quest->DestroyUniques.clear();
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				CUniqueItem *unique = GetUniqueItem(LuaToString(l, -1, j + 1));
				if (!unique) {
					LuaError(l, "Unique item doesn't exist.");
				}
				quest->DestroyUniques.push_back(unique);
			}
		} else if (!strcmp(value, "DestroyFactions")) {
			quest->DestroyFactions.clear();
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				CFaction *faction = PlayerRaces.GetFaction(LuaToString(l, -1, j + 1));
				if (!faction) {
					LuaError(l, "Faction doesn't exist.");
				}
				quest->DestroyFactions.push_back(faction);
			}
		} else if (!strcmp(value, "GatherResources")) {
			quest->GatherResources.clear();
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				int resource = GetResourceIdByName(LuaToString(l, -1, j + 1));
				if (resource == -1) {
					LuaError(l, "Resource doesn't exist.");
				}
				++j;
				
				int quantity = LuaToNumber(l, -1, j + 1);
				
				quest->GatherResources.push_back(std::tuple<int, int>(resource, quantity));
			}
		} else if (!strcmp(value, "HeroesMustSurvive")) {
			quest->HeroesMustSurvive.clear();
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				CCharacter *hero = GetCharacter(LuaToString(l, -1, j + 1));
				if (!hero) {
					LuaError(l, "Hero doesn't exist.");
				}
				quest->HeroesMustSurvive.push_back(hero);
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	if (!quest->Hidden && quest->Civilization != -1 && std::find(PlayerRaces.Civilizations[quest->Civilization]->Quests.begin(), PlayerRaces.Civilizations[quest->Civilization]->Quests.end(), quest) == PlayerRaces.Civilizations[quest->Civilization]->Quests.end()) {
		PlayerRaces.Civilizations[quest->Civilization]->Quests.push_back(quest);
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
	} else if (!strcmp(data, "PlayerColor")) {
		lua_pushstring(l, PlayerColorNames[quest->PlayerColor].c_str());
		return 1;
	} else if (!strcmp(data, "HairColor")) {
		if (quest->HairColor != 0) {
			lua_pushstring(l, HairColorNames[quest->HairColor].c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Hidden")) {
		lua_pushboolean(l, quest->Hidden);
		return 1;
	} else if (!strcmp(data, "Completed")) {
		lua_pushboolean(l, quest->Completed);
		return 1;
	} else if (!strcmp(data, "Competitive")) {
		lua_pushboolean(l, quest->Competitive);
		return 1;
	} else if (!strcmp(data, "HighestCompletedDifficulty")) {
		lua_pushnumber(l, quest->HighestCompletedDifficulty);
		return 1;
	} else if (!strcmp(data, "Icon")) {
		lua_pushstring(l, quest->Icon.Name.c_str());
		return 1;
	} else if (!strcmp(data, "QuestGiver")) {
		lua_pushstring(l, quest->QuestGiver->Ident.c_str());
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
**  Define a campaign.
**
**  @param l  Lua state.
*/
static int CclDefineCampaign(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string campaign_ident = LuaToString(l, 1);
	CCampaign *campaign = GetCampaign(campaign_ident);
	if (!campaign) {
		campaign = new CCampaign;
		campaign->ID = Campaigns.size();
		Campaigns.push_back(campaign);
		campaign->Ident = campaign_ident;
	}
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Name")) {
			campaign->Name = LuaToString(l, -1);
		} else if (!strcmp(value, "Description")) {
			campaign->Description = LuaToString(l, -1);
		} else if (!strcmp(value, "Faction")) {
			campaign->Faction = PlayerRaces.GetFaction(LuaToString(l, -1));
		} else if (!strcmp(value, "Hidden")) {
			campaign->Hidden = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Sandbox")) {
			campaign->Sandbox = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "StartYear")) {
			campaign->StartDate.year = LuaToNumber(l, -1);
		} else if (!strcmp(value, "StartDate")) {
			CclGetDate(l, &campaign->StartDate);
		} else if (!strcmp(value, "StartEffects")) {
			campaign->StartEffects = new LuaCallback(l, -1);
		} else if (!strcmp(value, "MapTemplate")) {
			std::string map_template_ident = LuaToString(l, -1);
			CMapTemplate *map_template = GetMapTemplate(map_template_ident);
			if (!map_template) {
				LuaError(l, "Map template \"%s\" doesn't exist." _C_ map_template_ident.c_str());
			}
			campaign->MapTemplate = map_template;
		} else if (!strcmp(value, "MapSize")) {
			CclGetPos(l, &campaign->MapSize.x, &campaign->MapSize.y);
		} else if (!strcmp(value, "MapTemplateStartPos")) {
			CclGetPos(l, &campaign->MapTemplateStartPos.x, &campaign->MapTemplateStartPos.y);
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	return 0;
}

static int CclGetCampaigns(lua_State *l)
{
	lua_createtable(l, Campaigns.size(), 0);
	for (size_t i = 1; i <= Campaigns.size(); ++i)
	{
		lua_pushstring(l, Campaigns[i-1]->Ident.c_str());
		lua_rawseti(l, -2, i);
	}
	return 1;
}

/**
**  Get campaign data.
**
**  @param l  Lua state.
*/
static int CclGetCampaignData(lua_State *l)
{
	if (lua_gettop(l) < 2) {
		LuaError(l, "incorrect argument");
	}
	std::string campaign_ident = LuaToString(l, 1);
	const CCampaign *campaign = GetCampaign(campaign_ident);
	if (!campaign) {
		LuaError(l, "Campaign \"%s\" doesn't exist." _C_ campaign_ident.c_str());
	}
	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Name")) {
		lua_pushstring(l, campaign->Name.c_str());
		return 1;
	} else if (!strcmp(data, "Description")) {
		lua_pushstring(l, campaign->Description.c_str());
		return 1;
	} else if (!strcmp(data, "StartYear")) {
		lua_pushnumber(l, campaign->StartDate.year);
		return 1;
	} else if (!strcmp(data, "Faction")) {
		if (campaign->Faction) {
			lua_pushstring(l, campaign->Faction->Ident.c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Hidden")) {
		lua_pushboolean(l, campaign->Hidden);
		return 1;
	} else if (!strcmp(data, "Sandbox")) {
		lua_pushboolean(l, campaign->Sandbox);
		return 1;
	} else if (!strcmp(data, "MapTemplate")) {
		if (campaign->MapTemplate) {
			lua_pushstring(l, campaign->MapTemplate->Ident.c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "MapWidth")) {
		lua_pushnumber(l, campaign->MapSize.x);
		return 1;
	} else if (!strcmp(data, "MapHeight")) {
		lua_pushnumber(l, campaign->MapSize.y);
		return 1;
	} else if (!strcmp(data, "MapTemplateStartPosX")) {
		lua_pushnumber(l, campaign->MapTemplateStartPos.x);
		return 1;
	} else if (!strcmp(data, "MapTemplateStartPosY")) {
		lua_pushnumber(l, campaign->MapTemplateStartPos.y);
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
		} else if (!strcmp(value, "HairColor")) {
			achievement->HairColor = GetHairColorIndexByName(LuaToString(l, -1));
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
	} else if (!strcmp(data, "HairColor")) {
		if (achievement->HairColor != 0) {
			lua_pushstring(l, HairColorNames[achievement->HairColor].c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Character")) {
		if (achievement->Character) {
			lua_pushstring(l, achievement->Character->Ident.c_str());
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
					} else if (!strcmp(value, "speaker-player")) {
						node->SpeakerPlayer = LuaToString(l, -1, k + 1);
					} else if (!strcmp(value, "text")) {
						node->Text = LuaToString(l, -1, k + 1);
					} else if (!strcmp(value, "conditions")) {
						lua_rawgeti(l, -1, k + 1);
						node->Conditions = new LuaCallback(l, -1);
						lua_pop(l, 1);
					} else if (!strcmp(value, "immediate-effects")) {
						lua_rawgeti(l, -1, k + 1);
						node->ImmediateEffects = new LuaCallback(l, -1);
						lua_pop(l, 1);
					} else if (!strcmp(value, "options")) {
						lua_rawgeti(l, -1, k + 1);
						const int subsubargs = lua_rawlen(l, -1);
						for (int n = 0; n < subsubargs; ++n) {
							node->Options.push_back(LuaToString(l, -1, n + 1));
						}
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
					} else if (!strcmp(value, "option-tooltips")) {
						lua_rawgeti(l, -1, k + 1);
						const int subsubargs = lua_rawlen(l, -1);
						for (int n = 0; n < subsubargs; ++n) {
							node->OptionTooltips.push_back(LuaToString(l, -1, n + 1));
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
	lua_register(Lua, "DefineCampaign", CclDefineCampaign);
	lua_register(Lua, "GetCampaigns", CclGetCampaigns);
	lua_register(Lua, "GetCampaignData", CclGetCampaignData);
	lua_register(Lua, "DefineAchievement", CclDefineAchievement);
	lua_register(Lua, "GetAchievements", CclGetAchievements);
	lua_register(Lua, "GetAchievementData", CclGetAchievementData);
	lua_register(Lua, "DefineDialogue", CclDefineDialogue);
}

//@}
