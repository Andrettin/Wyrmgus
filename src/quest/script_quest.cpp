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
//      (c) Copyright 2015-2021 by Andrettin
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

#include "stratagus.h"

#include "quest/quest.h"

#include "character.h"
#include "civilization.h"
#include "dialogue.h"
#include "dialogue_node.h"
#include "dialogue_option.h"
#include "faction.h"
#include "item/unique_item.h"
#include "luacallback.h"
#include "map/map.h"
#include "map/map_template.h"
#include "map/site.h"
#include "player.h"
#include "player_color.h"
#include "quest/achievement.h"
#include "quest/campaign.h"
#include "quest/objective/quest_objective.h"
#include "quest/objective_type.h"
#include "script.h"
#include "unit/unit_class.h"
#include "unit/unit_type.h"
#include "upgrade/upgrade.h"
#include "util/date_util.h"

static int CclDefineQuest(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string quest_ident = LuaToString(l, 1);
	wyrmgus::quest *quest = wyrmgus::quest::get_or_add(quest_ident, nullptr);
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Name")) {
			quest->set_name(LuaToString(l, -1));
		} else if (!strcmp(value, "Description")) {
			quest->set_description(LuaToString(l, -1));
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
		} else if (!strcmp(value, "StartSpeech")) {
			quest->StartSpeech = LuaToString(l, -1);
		} else if (!strcmp(value, "InProgressSpeech")) {
			quest->InProgressSpeech = LuaToString(l, -1);
		} else if (!strcmp(value, "CompletionSpeech")) {
			quest->CompletionSpeech = LuaToString(l, -1);
		} else if (!strcmp(value, "Rewards")) {
			quest->rewards_string = LuaToString(l, -1);
		} else if (!strcmp(value, "Hint")) {
			quest->hint = LuaToString(l, -1);
		} else if (!strcmp(value, "Civilization")) {
			wyrmgus::civilization *civilization = wyrmgus::civilization::get(LuaToString(l, -1));
			quest->civilization = civilization;
		} else if (!strcmp(value, "PlayerColor")) {
			const std::string color_name = LuaToString(l, -1);
			quest->player_color = wyrmgus::player_color::get(color_name);
		} else if (!strcmp(value, "Hidden")) {
			quest->hidden = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Competitive")) {
			quest->competitive = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Unobtainable")) {
			quest->unobtainable = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Uncompleteable")) {
			quest->uncompleteable = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Unfailable")) {
			quest->unfailable = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Icon")) {
			quest->icon = wyrmgus::icon::get(LuaToString(l, -1));
		} else if (!strcmp(value, "IntroductionDialogue")) {
			std::string dialogue_ident = LuaToString(l, -1);
			wyrmgus::dialogue *dialogue = wyrmgus::dialogue::get(dialogue_ident);
			quest->IntroductionDialogue = dialogue;
		} else if (!strcmp(value, "Conditions")) {
			quest->Conditions = std::make_unique<LuaCallback>(l, -1);
		} else if (!strcmp(value, "AcceptEffects")) {
			quest->AcceptEffects = std::make_unique<LuaCallback>(l, -1);
		} else if (!strcmp(value, "CompletionEffects")) {
			quest->CompletionEffects = std::make_unique<LuaCallback>(l, -1);
		} else if (!strcmp(value, "FailEffects")) {
			quest->FailEffects = std::make_unique<LuaCallback>(l, -1);
		} else if (!strcmp(value, "ObjectiveStrings")) {
			quest->objective_strings.clear();
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				quest->objective_strings.push_back(LuaToString(l, -1, j + 1));
			}
		} else if (!strcmp(value, "BriefingSounds")) {
			quest->BriefingSounds.clear();
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				quest->BriefingSounds.push_back(LuaToString(l, -1, j + 1));
			}
		// objective types
		} else if (!strcmp(value, "Objectives")) {
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				lua_rawgeti(l, -1, j + 1);
				wyrmgus::quest_objective *objective = nullptr;
				if (!lua_istable(l, -1)) {
					LuaError(l, "incorrect argument (expected table for quest objectives)");
				}
				const int subargs = lua_rawlen(l, -1);
				for (int k = 0; k < subargs; ++k) {
					value = LuaToString(l, -1, k + 1);
					++k;
					if (!strcmp(value, "objective-type")) {
						const std::string objective_type_str = LuaToString(l, -1, k + 1);

						auto objective_unique_ptr = wyrmgus::quest_objective::from_identifier(objective_type_str, quest);
						objective = objective_unique_ptr.get();
						quest->objectives.push_back(std::move(objective_unique_ptr));
					} else if (!strcmp(value, "objective-string")) {
						objective->objective_string = LuaToString(l, -1, k + 1);
					} else if (!strcmp(value, "quantity")) {
						objective->quantity = LuaToNumber(l, -1, k + 1);
					} else if (!strcmp(value, "unit-class")) {
						const wyrmgus::unit_class *unit_class = wyrmgus::unit_class::get(LuaToString(l, -1, k + 1));
						objective->unit_classes.push_back(unit_class);
					} else if (!strcmp(value, "unit-type")) {
						const wyrmgus::unit_type *unit_type = wyrmgus::unit_type::get(LuaToString(l, -1, k + 1));
						objective->unit_types.push_back(unit_type);
					} else if (!strcmp(value, "upgrade")) {
						const CUpgrade *upgrade = CUpgrade::get(LuaToString(l, -1, k + 1));
						objective->upgrade = upgrade;
					} else if (!strcmp(value, "settlement")) {
						const wyrmgus::site *site = wyrmgus::site::get(LuaToString(l, -1, k + 1));
						objective->settlement = site;
					} else if (!strcmp(value, "faction")) {
						const wyrmgus::faction *faction = wyrmgus::faction::get(LuaToString(l, -1, k + 1));
						objective->faction = faction;
					} else {
						printf("\n%s\n", quest->get_identifier().c_str());
						LuaError(l, "Unsupported tag: %s" _C_ value);
					}
				}
				lua_pop(l, 1);
			}
		} else if (!strcmp(value, "HeroesMustSurvive")) {
			quest->HeroesMustSurvive.clear();
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				wyrmgus::character *hero = wyrmgus::character::get(LuaToString(l, -1, j + 1));
				quest->HeroesMustSurvive.push_back(hero);
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	return 0;
}

static int CclGetQuests(lua_State *l)
{
	lua_createtable(l, wyrmgus::quest::get_all().size(), 0);
	for (size_t i = 1; i <= wyrmgus::quest::get_all().size(); ++i)
	{
		lua_pushstring(l, wyrmgus::quest::get_all()[i-1]->get_identifier().c_str());
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
	const wyrmgus::quest *quest = wyrmgus::quest::get(quest_ident);
	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Name")) {
		lua_pushstring(l, quest->get_name().c_str());
		return 1;
	} else if (!strcmp(data, "Description")) {
		lua_pushstring(l, quest->get_description().c_str());
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
		if (quest->civilization != nullptr) {
			lua_pushstring(l, quest->civilization->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "PlayerColor")) {
		if (quest->get_player_color() != nullptr) {
			lua_pushstring(l, quest->get_player_color()->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Hidden")) {
		lua_pushboolean(l, quest->is_hidden());
		return 1;
	} else if (!strcmp(data, "Completed")) {
		lua_pushboolean(l, quest->is_completed());
		return 1;
	} else if (!strcmp(data, "Competitive")) {
		lua_pushboolean(l, quest->is_competitive());
		return 1;
	} else if (!strcmp(data, "HighestCompletedDifficulty")) {
		lua_pushnumber(l, static_cast<int>(quest->get_highest_completed_difficulty()));
		return 1;
	} else if (!strcmp(data, "Icon")) {
		if (quest->get_icon() != nullptr) {
			lua_pushstring(l, quest->get_icon()->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Objectives")) {
		lua_createtable(l, quest->get_objective_strings().size(), 0);
		for (size_t i = 1; i <= quest->get_objective_strings().size(); ++i)
		{
			lua_pushstring(l, quest->get_objective_strings()[i-1].c_str());
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
	wyrmgus::campaign *campaign = wyrmgus::campaign::get_or_add(campaign_ident, nullptr);
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Name")) {
			campaign->set_name(LuaToString(l, -1));
		} else if (!strcmp(value, "Description")) {
			campaign->set_description(LuaToString(l, -1));
		} else if (!strcmp(value, "Faction")) {
			campaign->faction = wyrmgus::faction::get(LuaToString(l, -1));
		} else if (!strcmp(value, "Hidden")) {
			campaign->hidden = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Sandbox")) {
			campaign->Sandbox = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "StartYear")) {
			campaign->start_date.setDate(QDate(LuaToNumber(l, -1), 0, 0));
		} else if (!strcmp(value, "StartDate")) {
			CDate start_date;
			CclGetDate(l, &start_date);
			campaign->start_date = start_date;
		} else if (!strcmp(value, "RequiredQuests")) {
			campaign->required_quests.clear();
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				std::string quest_ident = LuaToString(l, -1, j + 1);
				wyrmgus::quest *required_quest = wyrmgus::quest::get(quest_ident);
				campaign->required_quests.push_back(required_quest);
			}
		} else if (!strcmp(value, "MapTemplates")) {
			campaign->map_templates.clear();
			campaign->MapSizes.clear();
			campaign->MapTemplateStartPos.clear();
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				std::string map_template_ident = LuaToString(l, -1, j + 1);
				wyrmgus::map_template *map_template = wyrmgus::map_template::get_or_add(map_template_ident, nullptr);
				campaign->map_templates.push_back(map_template);
				++j;
				
				lua_rawgeti(l, -1, j + 1);
				Vec2i map_template_start_pos;
				CclGetPos(l, &map_template_start_pos.x, &map_template_start_pos.y);
				campaign->MapTemplateStartPos.push_back(map_template_start_pos);
				lua_pop(l, 1);
				++j;
				
				lua_rawgeti(l, -1, j + 1);
				Vec2i map_size;
				CclGetPos(l, &map_size.x, &map_size.y);
				campaign->MapSizes.push_back(map_size);
				lua_pop(l, 1);
			}
		} else if (!strcmp(value, "MapTemplate")) {
			std::string map_template_ident = LuaToString(l, -1);
			wyrmgus::map_template *map_template = wyrmgus::map_template::get_or_add(map_template_ident, nullptr);
			campaign->map_templates.push_back(map_template);
		} else if (!strcmp(value, "MapTemplateStartPos")) {
			Vec2i map_template_start_pos;
			CclGetPos(l, &map_template_start_pos.x, &map_template_start_pos.y);
			campaign->MapTemplateStartPos.push_back(map_template_start_pos);
		} else if (!strcmp(value, "MapSize")) {
			Vec2i map_size;
			CclGetPos(l, &map_size.x, &map_size.y);
			campaign->MapSizes.push_back(map_size);
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	return 0;
}

static int CclGetCampaigns(lua_State *l)
{
	lua_createtable(l, wyrmgus::campaign::get_all().size(), 0);
	for (size_t i = 1; i <= wyrmgus::campaign::get_all().size(); ++i)
	{
		lua_pushstring(l, wyrmgus::campaign::get_all()[i - 1]->get_identifier().c_str());
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
	const wyrmgus::campaign *campaign = wyrmgus::campaign::get(campaign_ident);
	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Name")) {
		lua_pushstring(l, campaign->get_name().c_str());
		return 1;
	} else if (!strcmp(data, "Description")) {
		lua_pushstring(l, campaign->get_description().c_str());
		return 1;
	} else if (!strcmp(data, "StartYear")) {
		lua_pushnumber(l, campaign->get_start_date().date().year());
		return 1;
	} else if (!strcmp(data, "StartYearString")) {
		lua_pushstring(l, wyrmgus::date::year_to_string(campaign->get_start_date().date().year()).c_str());
		return 1;
	} else if (!strcmp(data, "Faction")) {
		if (campaign->get_faction() != nullptr) {
			lua_pushstring(l, campaign->get_faction()->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Hidden")) {
		lua_pushboolean(l, campaign->is_hidden());
		return 1;
	} else if (!strcmp(data, "Sandbox")) {
		lua_pushboolean(l, campaign->Sandbox);
		return 1;
	} else if (!strcmp(data, "RequiredQuests")) {
		lua_createtable(l, campaign->get_required_quests().size(), 0);
		for (size_t i = 1; i <= campaign->get_required_quests().size(); ++i)
		{
			lua_pushstring(l, campaign->get_required_quests()[i-1]->get_identifier().c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "MapTemplate")) {
		if (!campaign->map_templates.empty()) {
			lua_pushstring(l, campaign->map_templates[0]->Ident.c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "MapWidth")) {
		if (!campaign->MapSizes.empty()) {
			lua_pushnumber(l, campaign->MapSizes[0].x);
		} else {
			lua_pushnumber(l, campaign->get_map_templates().front()->get_applied_width());
		}
		return 1;
	} else if (!strcmp(data, "MapHeight")) {
		if (!campaign->MapSizes.empty()) {
			lua_pushnumber(l, campaign->MapSizes[0].y);
		} else {
			lua_pushnumber(l, campaign->get_map_templates().front()->get_applied_height());
		}
		return 1;
	} else if (!strcmp(data, "MapTemplateStartPosX")) {
		if (!campaign->MapTemplateStartPos.empty()) {
			lua_pushnumber(l, campaign->MapTemplateStartPos[0].x);
		} else {
			lua_pushnumber(l, 0);
		}
		return 1;
	} else if (!strcmp(data, "MapTemplateStartPosY")) {
		if (!campaign->MapTemplateStartPos.empty()) {
			lua_pushnumber(l, campaign->MapTemplateStartPos[0].y);
		} else {
			lua_pushnumber(l, 0);
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

	const std::string identifier = LuaToString(l, 1);
	wyrmgus::achievement *achievement = wyrmgus::achievement::get_or_add(identifier, nullptr);
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Name")) {
			achievement->set_name(LuaToString(l, -1));
		} else if (!strcmp(value, "Description")) {
			achievement->description = LuaToString(l, -1);
		} else if (!strcmp(value, "PlayerColor")) {
			const std::string color_name = LuaToString(l, -1);
			achievement->player_color = wyrmgus::player_color::get(color_name);
		} else if (!strcmp(value, "CharacterLevel")) {
			achievement->character_level = LuaToNumber(l, -1);
		} else if (!strcmp(value, "Difficulty")) {
			achievement->difficulty = static_cast<difficulty>(LuaToNumber(l, -1));
		} else if (!strcmp(value, "Hidden")) {
			achievement->hidden = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Unobtainable")) {
			achievement->unobtainable = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Icon")) {
			achievement->icon = wyrmgus::icon::get(LuaToString(l, -1));
		} else if (!strcmp(value, "Character")) {
			const std::string character_identifier = LuaToString(l, -1);
			wyrmgus::character *character = wyrmgus::character::get(character_identifier);
			achievement->character = character;
		} else if (!strcmp(value, "CharacterType")) {
			const std::string unit_type_ident = LuaToString(l, -1);
			wyrmgus::unit_type *unit_type = wyrmgus::unit_type::get(unit_type_ident);
			achievement->character_type = unit_type;
		} else if (!strcmp(value, "RequiredQuests")) {
			achievement->RequiredQuests.clear();
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				std::string quest_ident = LuaToString(l, -1, j + 1);
				wyrmgus::quest *required_quest = wyrmgus::quest::get(quest_ident);
				achievement->RequiredQuests.push_back(required_quest);
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	return 0;
}

static int CclGetAchievements(lua_State *l)
{
	lua_createtable(l, wyrmgus::achievement::get_all().size(), 0);
	for (size_t i = 1; i <= wyrmgus::achievement::get_all().size(); ++i)
	{
		lua_pushstring(l, wyrmgus::achievement::get_all()[i-1]->get_identifier().c_str());
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
	const std::string achievement_ident = LuaToString(l, 1);
	const wyrmgus::achievement *achievement = wyrmgus::achievement::get(achievement_ident);
	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Name")) {
		lua_pushstring(l, achievement->get_name().c_str());
		return 1;
	} else if (!strcmp(data, "Description")) {
		lua_pushstring(l, achievement->get_description().c_str());
		return 1;
	} else if (!strcmp(data, "PlayerColor")) {
		if (achievement->get_player_color() != nullptr) {
			lua_pushstring(l, achievement->get_player_color()->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Difficulty")) {
		lua_pushnumber(l, static_cast<int>(achievement->get_difficulty()));
		return 1;
	} else if (!strcmp(data, "Hidden")) {
		lua_pushboolean(l, achievement->is_hidden());
		return 1;
	} else if (!strcmp(data, "Obtained")) {
		lua_pushboolean(l, achievement->is_obtained());
		return 1;
	} else if (!strcmp(data, "Progress")) {
		lua_pushnumber(l, achievement->get_progress());
		return 1;
	} else if (!strcmp(data, "ProgressMax")) {
		lua_pushnumber(l, achievement->get_progress_max());
		return 1;
	} else if (!strcmp(data, "Icon")) {
		lua_pushstring(l, achievement->get_icon()->get_identifier().c_str());
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

	std::string dialogue_ident = LuaToString(l, 1);
	wyrmgus::dialogue *dialogue = wyrmgus::dialogue::get_or_add(dialogue_ident, nullptr);
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Nodes")) {
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				lua_rawgeti(l, -1, j + 1);
				auto node = std::make_unique<wyrmgus::dialogue_node>(dialogue);
				node->ID = dialogue->nodes.size();
				if (!lua_istable(l, -1)) {
					LuaError(l, "incorrect argument (expected table for dialogue nodes)");
				}
				const int subargs = lua_rawlen(l, -1);
				for (int k = 0; k < subargs; ++k) {
					value = LuaToString(l, -1, k + 1);
					++k;
					if (!strcmp(value, "speaker")) {
						const std::string speaker_type = LuaToString(l, -1, k + 1);
						++k;
						const std::string speaker = LuaToString(l, -1, k + 1);
						if (speaker_type == "character") {
							node->speaker = wyrmgus::character::get(speaker);
						} else if (speaker_type == "unit") {
							node->speaker_unit_type = wyrmgus::unit_type::get(speaker);
						} else {
							node->speaker_name = speaker;
						}
					} else if (!strcmp(value, "speaker-player")) {
						node->speaker_faction = wyrmgus::faction::get(LuaToString(l, -1, k + 1));
					} else if (!strcmp(value, "text")) {
						node->text = LuaToString(l, -1, k + 1);
					} else if (!strcmp(value, "conditions")) {
						lua_rawgeti(l, -1, k + 1);
						node->Conditions = std::make_unique<LuaCallback>(l, -1);
						lua_pop(l, 1);
					} else if (!strcmp(value, "immediate-effects")) {
						lua_rawgeti(l, -1, k + 1);
						node->ImmediateEffects = std::make_unique<LuaCallback>(l, -1);
						lua_pop(l, 1);
					} else if (!strcmp(value, "options")) {
						lua_rawgeti(l, -1, k + 1);
						const int subsubargs = lua_rawlen(l, -1);
						for (int n = 0; n < subsubargs; ++n) {
							if (n >= static_cast<int>(node->options.size())) {
								auto option = std::make_unique<wyrmgus::dialogue_option>(node.get());
								node->add_option(std::move(option));
							}
							node->options.at(n)->name = LuaToString(l, -1, n + 1);
						}
						lua_pop(l, 1);
					} else if (!strcmp(value, "option-effects")) {
						lua_rawgeti(l, -1, k + 1);
						const int subsubargs = lua_rawlen(l, -1);
						for (int n = 0; n < subsubargs; ++n) {
							lua_rawgeti(l, -1, n + 1);
							if (n >= static_cast<int>(node->options.size())) {
								auto option = std::make_unique<wyrmgus::dialogue_option>(node.get());
								node->add_option(std::move(option));
							}
							node->options.at(n)->lua_effects = std::make_unique<LuaCallback>(l, -1);
							lua_pop(l, 1);
						}
						lua_pop(l, 1);
					} else if (!strcmp(value, "option-tooltips")) {
						lua_rawgeti(l, -1, k + 1);
						const int subsubargs = lua_rawlen(l, -1);
						for (int n = 0; n < subsubargs; ++n) {
							if (n >= static_cast<int>(node->options.size())) {
								auto option = std::make_unique<wyrmgus::dialogue_option>(node.get());
								node->add_option(std::move(option));
							}
							node->options.at(n)->tooltip = LuaToString(l, -1, n + 1);
						}
						lua_pop(l, 1);
					} else {
						printf("\n%s\n", dialogue->get_identifier().c_str());
						LuaError(l, "Unsupported tag: %s" _C_ value);
					}
				}
				lua_pop(l, 1);
				dialogue->nodes.push_back(std::move(node));
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
