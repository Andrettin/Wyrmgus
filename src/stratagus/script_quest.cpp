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
#include "player.h"
#include "script.h"

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

	std::string quest_name = LuaToString(l, 1);
	CQuest *quest = GetQuest(quest_name);
	if (!quest) {
		quest = new CQuest;
		Quests.push_back(quest);
		quest->Name = quest_name;
	}
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Description")) {
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
		} else if (!strcmp(value, "X")) {
			quest->X = LuaToNumber(l, -1);
		} else if (!strcmp(value, "Y")) {
			quest->Y = LuaToNumber(l, -1);
		} else if (!strcmp(value, "PlayerColor")) {
			std::string color_name = LuaToString(l, -1);
			bool found_color = false;
			for (int c = 0; c < PlayerColorMax; ++c) {
				if (PlayerColorNames[c] == color_name) {
					quest->PlayerColor = c;
					found_color = true;
					break;
				}
			}
			if (!found_color) {
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
				quest->QuestGiver = const_cast<CCharacter *>(&(*quest_giver));
			} else {
				LuaError(l, "Character \"%s\" doesn't exist." _C_ quest_giver_name.c_str());
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
		lua_pushstring(l, Quests[i-1]->Name.c_str());
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
	std::string quest_name = LuaToString(l, 1);
	const CQuest *quest = GetQuest(quest_name);
	if (!quest) {
		LuaError(l, "Quest \"%s\" doesn't exist." _C_ quest_name.c_str());
	}
	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Description")) {
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
	} else if (!strcmp(data, "X")) {
		lua_pushnumber(l, quest->X);
		return 1;
	} else if (!strcmp(data, "Y")) {
		lua_pushnumber(l, quest->Y);
		return 1;
	} else if (!strcmp(data, "PlayerColor")) {
		lua_pushstring(l, PlayerColorNames[quest->PlayerColor].c_str());
		return 1;
	} else if (!strcmp(data, "Hidden")) {
		lua_pushboolean(l, quest->Hidden);
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

// ----------------------------------------------------------------------------

/**
**  Register CCL features for quests.
*/
void QuestCclRegister()
{
	lua_register(Lua, "DefineQuest", CclDefineQuest);
	lua_register(Lua, "GetQuests", CclGetQuests);
	lua_register(Lua, "GetQuestData", CclGetQuestData);
}

//@}
