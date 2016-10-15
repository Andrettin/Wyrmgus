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
/**@name script_player.cpp - The player ccl functions. */
//
//      (c) Copyright 2001-2007 by Lutz Sammer and Jimmy Salmon
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

#include "player.h"

#include "actions.h"
#include "ai.h"
//Wyrmgus start
#include "character.h"
//Wyrmgus end
#include "commands.h"
//Wyrmgus start
#include "editor.h"
#include "font.h"
#include "grand_strategy.h"
#include "item.h"
//Wyrmgus end
#include "map.h"
//Wyrmgus start
#include "province.h"
#include "quest.h"
//Wyrmgus end
#include "script.h"
#include "unittype.h"
#include "unit.h"
#include "unit_find.h"
#include "video.h"
#include "upgrade.h"
//Wyrmgus start
#include "ui.h"
#include "util.h"
//Wyrmgus end

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

extern CUnit *CclGetUnitFromRef(lua_State *l);

/**
**  Get a player pointer
**
**  @param l  Lua state.
**
**  @return   The player pointer
*/
static CPlayer *CclGetPlayer(lua_State *l)
{
	return &Players[LuaToNumber(l, -1)];
}

/**
**  Parse the player configuration.
**
**  @param l  Lua state.
*/
static int CclPlayer(lua_State *l)
{
	int i = LuaToNumber(l, 1);

	CPlayer &player = Players[i];
	player.Index = i;

	if (NumPlayers <= i) {
		NumPlayers = i + 1;
	}

	player.Load(l);
	return 0;
}

void CPlayer::Load(lua_State *l)
{
	const int args = lua_gettop(l);

	this->Units.resize(0);
	this->FreeWorkers.resize(0);
	//Wyrmgus start
	this->LevelUpUnits.resize(0);
	//Wyrmgus end

	// j = 0 represent player Index.
	for (int j = 1; j < args; ++j) {
		const char *value = LuaToString(l, j + 1);
		++j;

		if (!strcmp(value, "name")) {
			this->SetName(LuaToString(l, j + 1));
		} else if (!strcmp(value, "type")) {
			value = LuaToString(l, j + 1);
			if (!strcmp(value, "neutral")) {
				this->Type = PlayerNeutral;
			} else if (!strcmp(value, "nobody")) {
				this->Type = PlayerNobody;
			} else if (!strcmp(value, "computer")) {
				this->Type = PlayerComputer;
			} else if (!strcmp(value, "person")) {
				this->Type = PlayerPerson;
			} else if (!strcmp(value, "rescue-passive")) {
				this->Type = PlayerRescuePassive;
			} else if (!strcmp(value, "rescue-active")) {
				this->Type = PlayerRescueActive;
			} else {
				LuaError(l, "Unsupported tag: %s" _C_ value);
			}
		} else if (!strcmp(value, "race")) {
			const char *raceName = LuaToString(l, j + 1);
			this->Race = PlayerRaces.GetRaceIndexByName(raceName);
			if (this->Race == -1) {
				LuaError(l, "Unsupported race: %s" _C_ raceName);
			}
		//Wyrmgus start
		} else if (!strcmp(value, "faction")) {
			this->Faction = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "color")) {
			int color_id = LuaToNumber(l, j + 1);
			this->Color = PlayerColors[color_id][0];
			this->UnitColors.Colors = PlayerColorsRGB[color_id];
		//Wyrmgus end
		} else if (!strcmp(value, "ai-name")) {
			this->AiName = LuaToString(l, j + 1);
		} else if (!strcmp(value, "team")) {
			this->Team = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "enemy")) {
			value = LuaToString(l, j + 1);
			for (int i = 0; i < PlayerMax && *value; ++i, ++value) {
				if (*value == '-' || *value == '_' || *value == ' ') {
					this->Enemy &= ~(1 << i);
				} else {
					this->Enemy |= (1 << i);
				}
			}
		} else if (!strcmp(value, "allied")) {
			value = LuaToString(l, j + 1);
			for (int i = 0; i < PlayerMax && *value; ++i, ++value) {
				if (*value == '-' || *value == '_' || *value == ' ') {
					this->Allied &= ~(1 << i);
				} else {
					this->Allied |= (1 << i);
				}
			}
		} else if (!strcmp(value, "shared-vision")) {
			value = LuaToString(l, j + 1);
			for (int i = 0; i < PlayerMax && *value; ++i, ++value) {
				if (*value == '-' || *value == '_' || *value == ' ') {
					this->SharedVision &= ~(1 << i);
				} else {
					this->SharedVision |= (1 << i);
				}
			}
		} else if (!strcmp(value, "start")) {
			CclGetPos(l, &this->StartPos.x, &this->StartPos.y, j + 1);
		//Wyrmgus start
		} else if (!strcmp(value, "start-map-layer")) {
			this->StartMapLayer = LuaToNumber(l, j + 1);
		//Wyrmgus end
		} else if (!strcmp(value, "resources")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				value = LuaToString(l, j + 1, k + 1);
				++k;
				const int resId = GetResourceIdByName(l, value);
				this->Resources[resId] = LuaToNumber(l, j + 1, k + 1);
			}
		} else if (!strcmp(value, "stored-resources")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				value = LuaToString(l, j + 1, k + 1);
				++k;

				const int resId = GetResourceIdByName(l, value);
				this->StoredResources[resId] = LuaToNumber(l, j + 1, k + 1);
			}
		} else if (!strcmp(value, "max-resources")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				value = LuaToString(l, j + 1, k + 1);
				++k;
				const int resId = GetResourceIdByName(l, value);
				this->MaxResources[resId] = LuaToNumber(l, j + 1, k + 1);
			}
		} else if (!strcmp(value, "last-resources")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				value = LuaToString(l, j + 1, k + 1);
				++k;
				const int resId = GetResourceIdByName(l, value);
				this->LastResources[resId] = LuaToNumber(l, j + 1, k + 1);
			}
		} else if (!strcmp(value, "incomes")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				value = LuaToString(l, j + 1, k + 1);
				++k;

				const int resId = GetResourceIdByName(l, value);
				this->Incomes[resId] = LuaToNumber(l, j + 1, k + 1);
			}
		} else if (!strcmp(value, "revenue")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				value = LuaToString(l, j + 1, k + 1);
				++k;

				const int resId = GetResourceIdByName(l, value);
				this->Revenue[resId] = LuaToNumber(l, j + 1, k + 1);
			}
		} else if (!strcmp(value, "ai-enabled")) {
			this->AiEnabled = true;
			--j;
		} else if (!strcmp(value, "ai-disabled")) {
			this->AiEnabled = false;
			--j;
		//Wyrmgus start
		} else if (!strcmp(value, "revealed")) {
			this->Revealed = true;
			--j;
		//Wyrmgus end
		} else if (!strcmp(value, "supply")) {
			this->Supply = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "demand")) {
			this->Demand = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "unit-limit")) {
			this->UnitLimit = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "building-limit")) {
			this->BuildingLimit = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "total-unit-limit")) {
			this->TotalUnitLimit = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "score")) {
			this->Score = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "total-units")) {
			this->TotalUnits = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "total-buildings")) {
			this->TotalBuildings = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "total-razings")) {
			this->TotalRazings = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "total-kills")) {
			this->TotalKills = LuaToNumber(l, j + 1);
		//Wyrmgus start
		} else if (!strcmp(value, "unit-type-kills")) {
			int unit_type_id = UnitTypeIdByIdent(LuaToString(l, j + 1));
			++j;
			this->UnitTypeKills[unit_type_id] = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "lost-town-hall-timer")) {
			this->LostTownHallTimer = LuaToNumber(l, j + 1);
		//Wyrmgus end
		} else if (!strcmp(value, "total-resources")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			//Wyrmgus start
//			if (subargs != MaxCosts) {
//				LuaError(l, "Wrong number of total-resources: %d" _C_ subargs);
//			}
			if (subargs != MaxCosts) {
				fprintf(stderr, "Wrong number of total-resources: %d.\n", subargs);
			}
			//Wyrmgus end
			for (int k = 0; k < subargs; ++k) {
				this->TotalResources[k] = LuaToNumber(l, j + 1, k + 1);
			}
		} else if (!strcmp(value, "speed-resource-harvest")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			//Wyrmgus start
//			if (subargs != MaxCosts) {
//				LuaError(l, "Wrong number of speed-resource-harvest: %d" _C_ subargs);
//			}
			if (subargs != MaxCosts) {
				fprintf(stderr, "Wrong number of speed-resource-harvest: %d.\n", subargs);
			}
			//Wyrmgus end
			for (int k = 0; k < subargs; ++k) {
				this->SpeedResourcesHarvest[k] = LuaToNumber(l, j + 1, k + 1);
			}
		} else if (!strcmp(value, "speed-resource-return")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			//Wyrmgus start
//			if (subargs != MaxCosts) {
//				LuaError(l, "Wrong number of speed-resource-harvest: %d" _C_ subargs);
//			}
			if (subargs != MaxCosts) {
				fprintf(stderr, "Wrong number of speed-resource-return: %d.\n", subargs);
			}
			//Wyrmgus end
			for (int k = 0; k < subargs; ++k) {
				this->SpeedResourcesReturn[k] = LuaToNumber(l, j + 1, k + 1);
			}
		} else if (!strcmp(value, "speed-build")) {
			this->SpeedBuild = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "speed-train")) {
			this->SpeedTrain = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "speed-upgrade")) {
			this->SpeedUpgrade = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "speed-research")) {
			this->SpeedResearch = LuaToNumber(l, j + 1);
		//Wyrmgus start
		/*
		} else if (!strcmp(value, "color")) {
			if (!lua_istable(l, j + 1) || lua_rawlen(l, j + 1) != 3) {
				LuaError(l, "incorrect argument");
			}
			const int r = LuaToNumber(l, j + 1, 1);
			const int g = LuaToNumber(l, j + 1, 2);
			const int b = LuaToNumber(l, j + 1, 3);
			this->Color = Video.MapRGB(TheScreen->format, r, g, b);
		*/
		//Wyrmgus end
		//Wyrmgus start
		} else if (!strcmp(value, "current-quests")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				CQuest *quest = GetQuest(LuaToString(l, j + 1, k + 1));
				if (quest) {
					this->CurrentQuests.push_back(quest);
				}
			}
		} else if (!strcmp(value, "completed-quests")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				CQuest *quest = GetQuest(LuaToString(l, j + 1, k + 1));
				if (quest) {
					this->CompletedQuests.push_back(quest);
					if (quest->Competitive) {
						quest->CurrentCompleted = true;
					}
				}
			}
		} else if (!strcmp(value, "quest-build-units")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				CQuest *quest = GetQuest(LuaToString(l, j + 1, k + 1));
				++k;
				CUnitType *unit_type = UnitTypeByIdent(LuaToString(l, j + 1, k + 1));
				++k;
				int quantity = LuaToNumber(l, j + 1, k + 1);
				if (quest) {
					this->QuestBuildUnits.push_back(std::tuple<CQuest *, CUnitType *, int>(quest, unit_type, quantity));
				}
			}
		} else if (!strcmp(value, "quest-research-upgrades")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				CQuest *quest = GetQuest(LuaToString(l, j + 1, k + 1));
				++k;
				CUpgrade *upgrade = CUpgrade::Get(LuaToString(l, j + 1, k + 1));
				if (quest) {
					this->QuestResearchUpgrades.push_back(std::tuple<CQuest *, CUpgrade *>(quest, upgrade));
				}
			}
		} else if (!strcmp(value, "quest-destroy-units")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				CQuest *quest = GetQuest(LuaToString(l, j + 1, k + 1));
				++k;
				CUnitType *unit_type = UnitTypeByIdent(LuaToString(l, j + 1, k + 1));
				++k;
				CFaction *faction = PlayerRaces.GetFaction(-1, LuaToString(l, j + 1, k + 1));
				++k;
				int quantity = LuaToNumber(l, j + 1, k + 1);
				if (quest) {
					this->QuestDestroyUnits.push_back(std::tuple<CQuest *, CUnitType *, CFaction *, int>(quest, unit_type, faction, quantity));
				}
			}
		} else if (!strcmp(value, "quest-destroy-uniques")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				CQuest *quest = GetQuest(LuaToString(l, j + 1, k + 1));
				++k;
				CUniqueItem *unique = GetUniqueItem(LuaToString(l, j + 1, k + 1));
				++k;
				bool destroyed = LuaToBoolean(l, j + 1, k + 1);
				if (quest) {
					this->QuestDestroyUniques.push_back(std::tuple<CQuest *, CUniqueItem *, bool>(quest, unique, destroyed));
				}
			}
		} else if (!strcmp(value, "quest-gather-resources")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				CQuest *quest = GetQuest(LuaToString(l, j + 1, k + 1));
				++k;
				int resource = GetResourceIdByName(LuaToString(l, j + 1, k + 1));
				++k;
				int quantity = LuaToNumber(l, j + 1, k + 1);
				if (quest) {
					this->QuestGatherResources.push_back(std::tuple<CQuest *, int, int>(quest, resource, quantity));
				}
			}
		//Wyrmgus end
		} else if (!strcmp(value, "timers")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			if (subargs != UpgradeMax) {
				LuaError(l, "Wrong upgrade timer length: %d" _C_ subargs);
			}
			for (int k = 0; k < subargs; ++k) {
				this->UpgradeTimers.Upgrades[k] = LuaToNumber(l, j + 1, k + 1);
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	// Manage max
	for (int i = 0; i < MaxCosts; ++i) {
		if (this->MaxResources[i] != -1) {
			this->SetResource(i, this->Resources[i] + this->StoredResources[i], STORE_BOTH);
		}
	}
}

/**
**  Change unit owner
**
**  @param l  Lua state.
*/
static int CclChangeUnitsOwner(lua_State *l)
{
	LuaCheckArgs(l, 4);

	Vec2i pos1;
	Vec2i pos2;
	CclGetPos(l, &pos1.x, &pos1.y, 1);
	CclGetPos(l, &pos2.x, &pos2.y, 2);
	const int oldp = LuaToNumber(l, 3);
	const int newp = LuaToNumber(l, 4);
	std::vector<CUnit *> table;

	//Wyrmgus start
//	Select(pos1, pos2, table, HasSamePlayerAs(Players[oldp]));
	Select(pos1, pos2, table, 0, HasSamePlayerAs(Players[oldp]));
	//Wyrmgus end
	for (size_t i = 0; i != table.size(); ++i) {
		table[i]->ChangeOwner(Players[newp]);
	}
	return 0;
}

/**
**  Get ThisPlayer.
**
**  @param l  Lua state.
*/
static int CclGetThisPlayer(lua_State *l)
{
	LuaCheckArgs(l, 0);
	if (ThisPlayer) {
		lua_pushnumber(l, ThisPlayer - Players);
	} else {
		lua_pushnumber(l, 0);
	}
	return 1;
}

/**
**  Set ThisPlayer.
**
**  @param l  Lua state.
*/
static int CclSetThisPlayer(lua_State *l)
{
	LuaCheckArgs(l, 1);
	int plynr = LuaToNumber(l, 1);

	ThisPlayer = &Players[plynr];
	
	//Wyrmgus start
	UI.Load();
	
	if (GameRunning) {
		for (size_t i = 0; i < ThisPlayer->CurrentQuests.size(); ++i) {
			for (size_t j = 0; j < ThisPlayer->CurrentQuests[i]->Objectives.size(); ++j) {
				SetObjective("%s", ThisPlayer->CurrentQuests[i]->Objectives[j].c_str());
			}
		}
	}
	//Wyrmgus end

	lua_pushnumber(l, plynr);
	return 1;
}

/**
**  Set MaxSelectable
**
**  @param l  Lua state.
*/
static int CclSetMaxSelectable(lua_State *l)
{
	LuaCheckArgs(l, 1);
	MaxSelectable = LuaToNumber(l, 1);

	lua_pushnumber(l, MaxSelectable);
	return 1;
}

/**
**  Set player unit limit.
**
**  @param l  Lua state.
*/
static int CclSetAllPlayersUnitLimit(lua_State *l)
{
	LuaCheckArgs(l, 1);
	for (int i = 0; i < PlayerMax; ++i) {
		Players[i].UnitLimit = LuaToNumber(l, 1);
	}

	lua_pushnumber(l, lua_tonumber(l, 1));
	return 1;
}

/**
**  Set player unit limit.
**
**  @param l  Lua state.
*/
static int CclSetAllPlayersBuildingLimit(lua_State *l)
{
	LuaCheckArgs(l, 1);
	for (int i = 0; i < PlayerMax; ++i) {
		Players[i].BuildingLimit = LuaToNumber(l, 1);
	}

	lua_pushnumber(l, lua_tonumber(l, 1));
	return 1;
}

/**
**  Set player unit limit.
**
**  @param l  Lua state.
*/
static int CclSetAllPlayersTotalUnitLimit(lua_State *l)
{
	LuaCheckArgs(l, 1);
	for (int i = 0; i < PlayerMax; ++i) {
		Players[i].TotalUnitLimit = LuaToNumber(l, 1);
	}

	lua_pushnumber(l, lua_tonumber(l, 1));
	return 1;
}

/**
**  Change the diplomacy from player to another player.
**
**  @param l  Lua state.
**
**  @return          FIXME: should return old state.
*/
static int CclSetDiplomacy(lua_State *l)
{
	LuaCheckArgs(l, 3);
	const int base = LuaToNumber(l, 1);
	const int plynr = LuaToNumber(l, 3);
	const char *state = LuaToString(l, 2);

	if (!strcmp(state, "allied")) {
		SendCommandDiplomacy(base, DiplomacyAllied, plynr);
	} else if (!strcmp(state, "neutral")) {
		SendCommandDiplomacy(base, DiplomacyNeutral, plynr);
	} else if (!strcmp(state, "crazy")) {
		SendCommandDiplomacy(base, DiplomacyCrazy, plynr);
	} else if (!strcmp(state, "enemy")) {
		SendCommandDiplomacy(base, DiplomacyEnemy, plynr);
	}
	return 0;
}

/**
**  Change the diplomacy from ThisPlayer to another player.
**
**  @param l  Lua state.
*/
static int CclDiplomacy(lua_State *l)
{
	lua_pushnumber(l, ThisPlayer->Index);
	lua_insert(l, 1);
	return CclSetDiplomacy(l);
}

/**
**  Change the shared vision from player to another player.
**
**  @param l  Lua state.
**
**  @return   FIXME: should return old state.
*/
static int CclSetSharedVision(lua_State *l)
{
	LuaCheckArgs(l, 3);

	const int base = LuaToNumber(l, 1);
	const bool shared = LuaToBoolean(l, 2);
	const int plynr = LuaToNumber(l, 3);

	SendCommandSharedVision(base, shared, plynr);

	return 0;
}

/**
**  Change the shared vision from ThisPlayer to another player.
**
**  @param l  Lua state.
*/
static int CclSharedVision(lua_State *l)
{
	lua_pushnumber(l, ThisPlayer->Index);
	lua_insert(l, 1);
	return CclSetSharedVision(l);
}

/**
**  Define race names
**
**  @param l  Lua state.
*/
static int CclDefineRaceNames(lua_State *l)
{
	PlayerRaces.Clean();
	int args = lua_gettop(l);
	for (int j = 0; j < args; ++j) {
		const char *value = LuaToString(l, j + 1);
		if (!strcmp(value, "race")) {
			++j;
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			int subargs = lua_rawlen(l, j + 1);
			int i = PlayerRaces.Count++;
			for (int k = 0; k < subargs; ++k) {
				value = LuaToString(l, j + 1, k + 1);
				if (!strcmp(value, "name")) {
					++k;
					PlayerRaces.Name[i] = LuaToString(l, j + 1, k + 1);
					//Wyrmgus start
					PlayerRaces.Playable[i] = true; //civilizations are playable by default
					SetCivilizationStringToIndex(PlayerRaces.Name[i], i);
					//Wyrmgus end
				} else if (!strcmp(value, "display")) {
					++k;
					PlayerRaces.Display[i] = LuaToString(l, j + 1, k + 1);
				} else if (!strcmp(value, "visible")) {
					PlayerRaces.Visible[i] = 1;
				} else {
					LuaError(l, "Unsupported tag: %s" _C_ value);
				}
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}

	return 0;
}

//Wyrmgus start
/**
**  Define a civilization.
**
**  @param l  Lua state.
*/
static int CclDefineCivilization(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string civilization_name = LuaToString(l, 1);
	int civilization_id;
	CCivilization *civilization = NULL;
	if (PlayerRaces.GetRaceIndexByName(civilization_name.c_str()) != -1) { // redefinition
		civilization_id = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
		civilization = PlayerRaces.Civilizations[civilization_id];
	} else {
		civilization_id = PlayerRaces.Count++;
		PlayerRaces.Name[civilization_id] = civilization_name;
		PlayerRaces.Playable[civilization_id] = true; //civilizations are playable by default
		SetCivilizationStringToIndex(PlayerRaces.Name[civilization_id], civilization_id);
		
		civilization = new CCivilization;
		civilization->Ident = civilization_name;
		civilization->ID = PlayerRaces.Civilizations.size();
		PlayerRaces.Civilizations.push_back(civilization);
	}
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		if (!strcmp(value, "Display")) {
			PlayerRaces.Display[civilization_id] = LuaToString(l, -1);
		} else if (!strcmp(value, "Description")) {
			civilization->Description = LuaToString(l, -1);
		} else if (!strcmp(value, "Quote")) {
			civilization->Quote = LuaToString(l, -1);
		} else if (!strcmp(value, "Background")) {
			civilization->Background = LuaToString(l, -1);
		} else if (!strcmp(value, "Adjective")) {
			civilization->Adjective = LuaToString(l, -1);
		} else if (!strcmp(value, "CalendarStartingYear")) {
			civilization->CalendarStartingYear = LuaToNumber(l, -1);
		} else if (!strcmp(value, "YearLabel")) {
			civilization->YearLabel = LuaToString(l, -1);
		} else if (!strcmp(value, "NegativeYearLabel")) {
			civilization->NegativeYearLabel = LuaToString(l, -1);
		} else if (!strcmp(value, "Visible")) {
			PlayerRaces.Visible[civilization_id] = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Playable")) {
			PlayerRaces.Playable[civilization_id] = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Species")) {
			PlayerRaces.Species[civilization_id] = LuaToString(l, -1);
		} else if (!strcmp(value, "ParentCivilization")) {
			PlayerRaces.ParentCivilization[civilization_id] = PlayerRaces.GetRaceIndexByName(LuaToString(l, -1));
		} else if (!strcmp(value, "Language")) {
			int language = PlayerRaces.GetLanguageIndexByIdent(LuaToString(l, -1));
			if (language != -1) {
				PlayerRaces.CivilizationLanguage[civilization_id] = language;
				PlayerRaces.Languages[language]->UsedByCivilizationOrFaction = true;
			} else {
				LuaError(l, "Language not found.");
			}
		} else if (!strcmp(value, "DefaultColor")) {
			PlayerRaces.DefaultColor[civilization_id] = LuaToString(l, -1);
		} else if (!strcmp(value, "CivilizationUpgrade")) {
			PlayerRaces.CivilizationUpgrades[civilization_id] = LuaToString(l, -1);
		} else if (!strcmp(value, "DevelopsFrom")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				std::string originary_civilization_name = LuaToString(l, -1, j + 1);
				int originary_civilization = PlayerRaces.GetRaceIndexByName(originary_civilization_name.c_str());
				if (originary_civilization == -1) {
					LuaError(l, "Civilization \"%s\" doesn't exist." _C_ originary_civilization_name.c_str());
				}
				PlayerRaces.DevelopsFrom[civilization_id].push_back(originary_civilization);
				PlayerRaces.DevelopsTo[originary_civilization].push_back(civilization_id);
			}
		} else if (!strcmp(value, "ButtonIcons")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				std::string button_action_name = LuaToString(l, -1, j + 1);
				int button_action = GetButtonActionIdByName(button_action_name);
				if (button_action != -1) {
					++j;
					PlayerRaces.ButtonIcons[civilization_id][button_action].Name = LuaToString(l, -1, j + 1);
					PlayerRaces.ButtonIcons[civilization_id][button_action].Icon = NULL;
					PlayerRaces.ButtonIcons[civilization_id][button_action].Load();
				} else {
					LuaError(l, "Button action \"%s\" doesn't exist." _C_ button_action_name.c_str());
				}
			}
		} else if (!strcmp(value, "UIFillers")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			
			PlayerRaces.CivilizationUIFillers[civilization_id].clear();
			
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				CFiller filler = CFiller();
				std::string filler_file = LuaToString(l, -1, j + 1);
				if (filler_file.empty()) {
					LuaError(l, "Filler graphic file is empty.");
				}				
				filler.G = CGraphic::New(filler_file);
				++j;
				filler.X = LuaToNumber(l, -1, j + 1);
				++j;
				filler.Y = LuaToNumber(l, -1, j + 1);
				PlayerRaces.CivilizationUIFillers[civilization_id].push_back(filler);
			}
		} else if (!strcmp(value, "Months")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				std::string month_name = LuaToString(l, -1, j + 1);
				int month = GetMonthIdByName(month_name);
				if (month != -1) {
					++j;
					civilization->Months[month] = LuaToString(l, -1, j + 1);
				} else {
					LuaError(l, "Month \"%s\" doesn't exist." _C_ month_name.c_str());
				}
			}
		} else if (!strcmp(value, "PersonalNames")) {
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				int gender_id = GetGenderIdByName(LuaToString(l, -1, j + 1));
				if (gender_id == -1) {
					gender_id = NoGender;
				} else {
					++j;
				}
				
				civilization->PersonalNames[gender_id].push_back(LuaToString(l, -1, j + 1));
			}
		} else if (!strcmp(value, "FamilyNames")) {
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				civilization->FamilyNames.push_back(LuaToString(l, -1, j + 1));
			}
		} else if (!strcmp(value, "SettlementNames")) {
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				civilization->SettlementNames.push_back(LuaToString(l, -1, j + 1));
			}
		} else if (!strcmp(value, "ProvinceNames")) {
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				civilization->ProvinceNames.push_back(LuaToString(l, -1, j + 1));
			}
		} else if (!strcmp(value, "ShipNames")) {
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				civilization->ShipNames.push_back(LuaToString(l, -1, j + 1));
			}
		} else if (!strcmp(value, "HistoricalTechnologies")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				std::string technology_ident = LuaToString(l, -1, j + 1);
				++j;
				int year = LuaToNumber(l, -1, j + 1);
				civilization->HistoricalTechnologies[technology_ident] = year;
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	if (PlayerRaces.ParentCivilization[civilization_id] != -1) {
		int parent_civilization = PlayerRaces.ParentCivilization[civilization_id];

		if (PlayerRaces.CivilizationUpgrades[civilization_id].empty() && !PlayerRaces.CivilizationUpgrades[parent_civilization].empty()) { //if the civilization has no civilization upgrade, inherit that of its parent civilization
			PlayerRaces.CivilizationUpgrades[civilization_id] = PlayerRaces.CivilizationUpgrades[parent_civilization];
		}
		
		//inherit button icons from parent civilization, for button actions which none are specified
		for (std::map<int, IconConfig>::iterator iterator = PlayerRaces.ButtonIcons[parent_civilization].begin(); iterator != PlayerRaces.ButtonIcons[parent_civilization].end(); ++iterator) {
			if (PlayerRaces.ButtonIcons[civilization_id].find(iterator->first) == PlayerRaces.ButtonIcons[civilization_id].end()) {
				PlayerRaces.ButtonIcons[civilization_id][iterator->first] = iterator->second;
			}
		}
		
		if (civilization->PersonalNames.size() == 0) {
			for (std::map<int, std::vector<std::string>>::iterator iterator = PlayerRaces.Civilizations[parent_civilization]->PersonalNames.begin(); iterator != PlayerRaces.Civilizations[parent_civilization]->PersonalNames.end(); ++iterator) {
				for (size_t i = 0; i < iterator->second.size(); ++i) {
					civilization->PersonalNames[iterator->first].push_back(iterator->second[i]);				
				}
			}
		}

		if (civilization->FamilyNames.size() == 0) {
			for (size_t i = 0; i < PlayerRaces.Civilizations[parent_civilization]->FamilyNames.size(); ++i) {
				civilization->FamilyNames.push_back(PlayerRaces.Civilizations[parent_civilization]->FamilyNames[i]);
			}
		}

		if (civilization->SettlementNames.size() == 0) {
			for (size_t i = 0; i < PlayerRaces.Civilizations[parent_civilization]->SettlementNames.size(); ++i) {
				civilization->SettlementNames.push_back(PlayerRaces.Civilizations[parent_civilization]->SettlementNames[i]);
			}
		}

		if (civilization->ProvinceNames.size() == 0) {
			for (size_t i = 0; i < PlayerRaces.Civilizations[parent_civilization]->ProvinceNames.size(); ++i) {
				civilization->ProvinceNames.push_back(PlayerRaces.Civilizations[parent_civilization]->ProvinceNames[i]);
			}
		}

		if (civilization->ShipNames.size() == 0) {
			for (size_t i = 0; i < PlayerRaces.Civilizations[parent_civilization]->ShipNames.size(); ++i) {
				civilization->ShipNames.push_back(PlayerRaces.Civilizations[parent_civilization]->ShipNames[i]);
			}
		}
		
		for (std::map<std::string, int>::iterator iterator = PlayerRaces.Civilizations[parent_civilization]->HistoricalTechnologies.begin(); iterator != PlayerRaces.Civilizations[parent_civilization]->HistoricalTechnologies.end(); ++iterator) {
			if (civilization->HistoricalTechnologies.find(iterator->first) == civilization->HistoricalTechnologies.end()) {
				civilization->HistoricalTechnologies[iterator->first] = iterator->second;
			}
		}
	}
	
	if (PlayerRaces.ButtonIcons[civilization_id].find(ButtonMove) != PlayerRaces.ButtonIcons[civilization_id].end()) {
		std::string button_definition = "DefineButton({\n";
		button_definition += "\tPos = 1,\n";
		button_definition += "\tLevel = 0,\n";
		button_definition += "\tAction = \"move\",\n";
		button_definition += "\tPopup = \"popup-commands\",\n";
		button_definition += "\tKey = \"m\",\n";
		button_definition += "\tHint = _(\"~!Move\"),\n";
		button_definition += "\tForUnit = {\"" + PlayerRaces.Name[civilization_id] + "-group\"},\n";
		button_definition += "})";
		CclCommand(button_definition);
	}
	
	if (PlayerRaces.ButtonIcons[civilization_id].find(ButtonStop) != PlayerRaces.ButtonIcons[civilization_id].end()) {
		std::string button_definition = "DefineButton({\n";
		button_definition += "\tPos = 2,\n";
		button_definition += "\tLevel = 0,\n";
		button_definition += "\tAction = \"stop\",\n";
		button_definition += "\tPopup = \"popup-commands\",\n";
		button_definition += "\tKey = \"s\",\n";
		button_definition += "\tHint = _(\"~!Stop\"),\n";
		button_definition += "\tForUnit = {\"" + PlayerRaces.Name[civilization_id] + "-group\"},\n";
		button_definition += "})";
		CclCommand(button_definition);
	}
	
	if (PlayerRaces.ButtonIcons[civilization_id].find(ButtonAttack) != PlayerRaces.ButtonIcons[civilization_id].end()) {
		std::string button_definition = "DefineButton({\n";
		button_definition += "\tPos = 3,\n";
		button_definition += "\tLevel = 0,\n";
		button_definition += "\tAction = \"attack\",\n";
		button_definition += "\tPopup = \"popup-commands\",\n";
		button_definition += "\tKey = \"a\",\n";
		button_definition += "\tHint = _(\"~!Attack\"),\n";
		button_definition += "\tForUnit = {\"" + PlayerRaces.Name[civilization_id] + "-group\"},\n";
		button_definition += "})";
		CclCommand(button_definition);
	}
	
	if (PlayerRaces.ButtonIcons[civilization_id].find(ButtonPatrol) != PlayerRaces.ButtonIcons[civilization_id].end()) {
		std::string button_definition = "DefineButton({\n";
		button_definition += "\tPos = 4,\n";
		button_definition += "\tLevel = 0,\n";
		button_definition += "\tAction = \"patrol\",\n";
		button_definition += "\tPopup = \"popup-commands\",\n";
		button_definition += "\tKey = \"p\",\n";
		button_definition += "\tHint = _(\"~!Patrol\"),\n";
		button_definition += "\tForUnit = {\"" + PlayerRaces.Name[civilization_id] + "-group\"},\n";
		button_definition += "})";
		CclCommand(button_definition);
	}
	
	if (PlayerRaces.ButtonIcons[civilization_id].find(ButtonStandGround) != PlayerRaces.ButtonIcons[civilization_id].end()) {
		std::string button_definition = "DefineButton({\n";
		button_definition += "\tPos = 5,\n";
		button_definition += "\tLevel = 0,\n";
		button_definition += "\tAction = \"stand-ground\",\n";
		button_definition += "\tPopup = \"popup-commands\",\n";
		button_definition += "\tKey = \"t\",\n";
		button_definition += "\tHint = _(\"S~!tand Ground\"),\n";
		button_definition += "\tForUnit = {\"" + PlayerRaces.Name[civilization_id] + "-group\"},\n";
		button_definition += "})";
		CclCommand(button_definition);
	}
	
	return 0;
}

/**
**  Define a word for a particular language.
**
**  @param l  Lua state.
*/
static int CclDefineLanguageWord(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	LanguageWord *word = new LanguageWord;
	word->Word = LuaToString(l, 1);
	
	LanguageWord *replaces = NULL;
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Language")) {
			int language = PlayerRaces.GetLanguageIndexByIdent(LuaToString(l, -1));
			
			if (language != -1) {
				PlayerRaces.Languages[language]->LanguageWords.push_back(word);
				word->Language = language;
				
				for (size_t i = 0; i < PlayerRaces.Languages[language]->Dialects.size(); ++i) { //copy the word over for dialects
					PlayerRaces.Languages[language]->Dialects[i]->LanguageWords.push_back(word);
				}
			} else {
				LuaError(l, "Language not found.");
			}
		} else if (!strcmp(value, "Meanings")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				word->Meanings.push_back(LuaToString(l, -1, j + 1));
			}
		} else if (!strcmp(value, "Type")) {
			std::string word_type_name = LuaToString(l, -1);
			int word_type = GetWordTypeIdByName(word_type_name);
			if (word_type != -1) {
				word->Type = word_type;
			} else {
				LuaError(l, "Word type \"%s\" doesn't exist." _C_ word_type_name.c_str());
			}
		} else if (!strcmp(value, "DerivesFrom")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			int j = 0;
			int derives_from_language = PlayerRaces.GetLanguageIndexByIdent(LuaToString(l, -1, j + 1));
			++j;
			int derives_from_word_type = GetWordTypeIdByName(LuaToString(l, -1, j + 1));
			++j;
			
			std::vector<std::string> word_meanings;
			lua_rawgeti(l, -1, j + 1);
			if (lua_istable(l, -1)) {
				const int subargs = lua_rawlen(l, -1);
				for (int k = 0; k < subargs; ++k) {
					word_meanings.push_back(LuaToString(l, -1, k + 1));
				}
				
				++j;
			}
			lua_pop(l, 1);
			
			if (derives_from_language != -1 && derives_from_word_type != -1) {
				std::string derives_from_word = LuaToString(l, -1, j + 1);
				word->DerivesFrom = const_cast<LanguageWord *>(&(*PlayerRaces.Languages[derives_from_language]->GetWord(derives_from_word, derives_from_word_type, word_meanings)));
				
				if (word->DerivesFrom != NULL) {
					word->DerivesFrom->DerivesTo.push_back(word);
				} else {
					LuaError(l, "Word \"%s\" is set to derive from \"%s\" (%s, %s), but the latter doesn't exist" _C_ word->Word.c_str() _C_ derives_from_word.c_str() _C_ PlayerRaces.Languages[derives_from_language]->Name.c_str() _C_ GetWordTypeNameById(derives_from_word_type).c_str());
				}
			} else {
				LuaError(l, "Word \"%s\"'s derives from is incorrectly set, as either the language or the word type set for the original word given is incorrect" _C_ word->Word.c_str());
			}
		} else if (!strcmp(value, "Replaces")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			int j = 0;
			int replaces_language = PlayerRaces.GetLanguageIndexByIdent(LuaToString(l, -1, j + 1));
			++j;
			int replaces_word_type = GetWordTypeIdByName(LuaToString(l, -1, j + 1));
			++j;
			
			std::vector<std::string> word_meanings;
			lua_rawgeti(l, -1, j + 1);
			if (lua_istable(l, -1)) {
				const int subargs = lua_rawlen(l, -1);
				for (int k = 0; k < subargs; ++k) {
					word_meanings.push_back(LuaToString(l, -1, k + 1));
				}
				
				++j;
			}
			lua_pop(l, 1);
			
			if (replaces_language != -1 && replaces_word_type != -1) {
				std::string replaces_word = LuaToString(l, -1, j + 1);
				replaces = const_cast<LanguageWord *>(&(*PlayerRaces.Languages[replaces_language]->GetWord(replaces_word, replaces_word_type, word_meanings)));
				
				if (replaces == NULL) {
					LuaError(l, "Word \"%s\" is set to replace \"%s\" (%s, %s), but the latter doesn't exist" _C_ word->Word.c_str() _C_ replaces_word.c_str() _C_ PlayerRaces.Languages[replaces_language]->Name.c_str() _C_ GetWordTypeNameById(replaces_word_type).c_str());
				}
			} else {
				LuaError(l, "Word \"%s\"'s replace is incorrectly set, as either the language or the word type set for the original word given is incorrect" _C_ word->Word.c_str());
			}
		} else if (!strcmp(value, "CompoundElements")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				std::string affix_type_name = LuaToString(l, -1, j + 1);
				int affix_type = GetAffixTypeIdByName(affix_type_name);
				if (affix_type == -1) {
					LuaError(l, "Affix type \"%s\" doesn't exist." _C_ affix_type_name.c_str());
				}
				++j;
				
				int affix_language = PlayerRaces.GetLanguageIndexByIdent(LuaToString(l, -1, j + 1)); // should be the same language as that of the word, but needs to be specified since the word's language may not have been set yet
				++j;
				int affix_word_type = GetWordTypeIdByName(LuaToString(l, -1, j + 1));
				++j;
				
				std::vector<std::string> word_meanings;
				lua_rawgeti(l, -1, j + 1);
				if (lua_istable(l, -1)) {
					const int subargs = lua_rawlen(l, -1);
					for (int k = 0; k < subargs; ++k) {
						word_meanings.push_back(LuaToString(l, -1, k + 1));
					}
					
					++j;
				}
				lua_pop(l, 1);

				if (affix_language != -1 && affix_word_type != -1) {
					std::string affix_word = LuaToString(l, -1, j + 1);
					word->CompoundElements[affix_type] = const_cast<LanguageWord *>(&(*PlayerRaces.Languages[affix_language]->GetWord(affix_word, affix_word_type, word_meanings)));
					
					if (word->CompoundElements[affix_type] != NULL) {
						word->CompoundElements[affix_type]->CompoundElementOf[affix_type].push_back(word);
					} else {
						LuaError(l, "Word \"%s\" is set to be a compound formed by \"%s\" (%s, %s), but the latter doesn't exist" _C_ word->Word.c_str() _C_ affix_word.c_str() _C_ PlayerRaces.Languages[affix_language]->Name.c_str() _C_ GetWordTypeNameById(affix_word_type).c_str());
					}
				} else {
					LuaError(l, "Word \"%s\"'s compound elements are incorrectly set, as either the language or the word type set for one of the element words given is incorrect" _C_ word->Word.c_str());
				}
			}
		} else if (!strcmp(value, "Gender")) {
			std::string grammatical_gender_name = LuaToString(l, -1);
			int grammatical_gender = GetGrammaticalGenderIdByName(grammatical_gender_name);
			if (grammatical_gender != -1) {
				word->Gender = grammatical_gender;
			} else {
				LuaError(l, "Grammatical gender \"%s\" doesn't exist." _C_ grammatical_gender_name.c_str());
			}
		} else if (!strcmp(value, "GrammaticalNumber")) {
			std::string grammatical_number_name = LuaToString(l, -1);
			int grammatical_number = GetGrammaticalNumberIdByName(grammatical_number_name);
			if (grammatical_number != -1) {
				word->GrammaticalNumber = grammatical_number;
			} else {
				LuaError(l, "Grammatical number \"%s\" doesn't exist." _C_ grammatical_number_name.c_str());
			}
		} else if (!strcmp(value, "Archaic")) {
			word->Archaic = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "NumberCaseInflections")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				std::string grammatical_number_name = LuaToString(l, -1, j + 1);
				int grammatical_number = GetGrammaticalNumberIdByName(grammatical_number_name);
				if (grammatical_number == -1) {
					LuaError(l, "Grammatical number \"%s\" doesn't exist." _C_ grammatical_number_name.c_str());
				}
				++j;
				
				std::string grammatical_case_name = LuaToString(l, -1, j + 1);
				int grammatical_case = GetGrammaticalCaseIdByName(grammatical_case_name);
				if (grammatical_case == -1) {
					LuaError(l, "Grammatical case \"%s\" doesn't exist." _C_ grammatical_case_name.c_str());
				}
				++j;

				word->NumberCaseInflections[grammatical_number][grammatical_case] = LuaToString(l, -1, j + 1);
			}
		} else if (!strcmp(value, "NumberPersonTenseMoodInflections")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				std::string grammatical_number_name = LuaToString(l, -1, j + 1);
				int grammatical_number = GetGrammaticalNumberIdByName(grammatical_number_name);
				if (grammatical_number == -1) {
					LuaError(l, "Grammatical number \"%s\" doesn't exist." _C_ grammatical_number_name.c_str());
				}
				++j;
				
				std::string grammatical_person_name = LuaToString(l, -1, j + 1);
				int grammatical_person = GetGrammaticalPersonIdByName(grammatical_person_name);
				if (grammatical_person == -1) {
					LuaError(l, "Grammatical person \"%s\" doesn't exist." _C_ grammatical_person_name.c_str());
				}
				++j;
				
				std::string grammatical_tense_name = LuaToString(l, -1, j + 1);
				int grammatical_tense = GetGrammaticalTenseIdByName(grammatical_tense_name);
				if (grammatical_tense == -1) {
					LuaError(l, "Grammatical tense \"%s\" doesn't exist." _C_ grammatical_tense_name.c_str());
				}
				++j;
				
				std::string grammatical_mood_name = LuaToString(l, -1, j + 1);
				int grammatical_mood = GetGrammaticalMoodIdByName(grammatical_mood_name);
				if (grammatical_mood == -1) {
					LuaError(l, "Grammatical mood \"%s\" doesn't exist." _C_ grammatical_mood_name.c_str());
				}
				++j;

				word->NumberPersonTenseMoodInflections[grammatical_number][grammatical_person][grammatical_tense][grammatical_mood] = LuaToString(l, -1, j + 1);
			}
		} else if (!strcmp(value, "ComparisonDegreeCaseInflections")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				std::string comparison_degree_name = LuaToString(l, -1, j + 1);
				int comparison_degree = GetComparisonDegreeIdByName(comparison_degree_name);
				if (comparison_degree == -1) {
					LuaError(l, "Comparison degree \"%s\" doesn't exist." _C_ comparison_degree_name.c_str());
				}
				++j;
				
				int grammatical_case = GrammaticalCaseNoCase;
				if (GetGrammaticalCaseIdByName(LuaToString(l, -1, j + 1)) != -1) {
					std::string grammatical_case_name = LuaToString(l, -1, j + 1);
					grammatical_case = GetGrammaticalCaseIdByName(grammatical_case_name);
					if (grammatical_case == -1) {
						LuaError(l, "Grammatical case \"%s\" doesn't exist." _C_ grammatical_case_name.c_str());
					}
					++j;
				}
				
				word->ComparisonDegreeCaseInflections[comparison_degree][grammatical_case] = LuaToString(l, -1, j + 1);
			}
		} else if (!strcmp(value, "Participles")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				std::string grammatical_tense_name = LuaToString(l, -1, j + 1);
				int grammatical_tense = GetGrammaticalTenseIdByName(grammatical_tense_name);
				if (grammatical_tense == -1) {
					LuaError(l, "Grammatical tense \"%s\" doesn't exist." _C_ grammatical_tense_name.c_str());
				}
				++j;
				
				word->Participles[grammatical_tense] = LuaToString(l, -1, j + 1);
			}
		//noun-specific variables
		} else if (!strcmp(value, "Uncountable")) {
			word->Uncountable = LuaToBoolean(l, -1);
		//pronoun and article-specific variables
		} else if (!strcmp(value, "Nominative")) {
			word->Nominative = LuaToString(l, -1);
		} else if (!strcmp(value, "Accusative")) {
			word->Accusative = LuaToString(l, -1);
		} else if (!strcmp(value, "Dative")) {
			word->Dative = LuaToString(l, -1);
		} else if (!strcmp(value, "Genitive")) {
			word->Genitive = LuaToString(l, -1);
		//article-specific variables
		} else if (!strcmp(value, "ArticleType")) {
			std::string article_type_name = LuaToString(l, -1);
			int article_type = GetArticleTypeIdByName(article_type_name);
			if (article_type != -1) {
				word->ArticleType = article_type;
			} else {
				LuaError(l, "Article type \"%s\" doesn't exist." _C_ article_type_name.c_str());
			}
		//numeral-specific variables
		} else if (!strcmp(value, "Number")) {
			word->Number = LuaToNumber(l, -1);
		//type name variables
		} else if (!strcmp(value, "NameTypes")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				int grammatical_number = GrammaticalNumberSingular;
				if (GetGrammaticalNumberIdByName(LuaToString(l, -1, j + 1)) != -1) {
					std::string grammatical_number_name = LuaToString(l, -1, j + 1);
					grammatical_number = GetGrammaticalNumberIdByName(grammatical_number_name);
					if (grammatical_number == -1) {
						LuaError(l, "Grammatical number \"%s\" doesn't exist." _C_ grammatical_number_name.c_str());
					}
					++j;
				}
				
				int grammatical_case = GrammaticalCaseNominative;
				if (GetGrammaticalCaseIdByName(LuaToString(l, -1, j + 1)) != -1) {
					std::string grammatical_case_name = LuaToString(l, -1, j + 1);
					grammatical_case = GetGrammaticalCaseIdByName(grammatical_case_name);
					if (grammatical_case == -1) {
						LuaError(l, "Grammatical case \"%s\" doesn't exist." _C_ grammatical_case_name.c_str());
					}
					++j;
				}
				
				int grammatical_tense = GrammaticalTenseNoTense;
				if (GetGrammaticalTenseIdByName(LuaToString(l, -1, j + 1)) != -1) {
					std::string grammatical_tense_name = LuaToString(l, -1, j + 1);
					grammatical_tense = GetGrammaticalTenseIdByName(grammatical_tense_name);
					if (grammatical_tense == -1) {
						LuaError(l, "Grammatical tense \"%s\" doesn't exist." _C_ grammatical_tense_name.c_str());
					}
					++j;
				}
				
				std::string type = LuaToString(l, -1, j + 1);
				word->IncreaseNameType(type, grammatical_number, grammatical_case, grammatical_tense);
			}
		} else if (!strcmp(value, "AffixNameTypes")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				std::string word_junction_type_name = LuaToString(l, -1, j + 1);
				int word_junction_type = GetWordJunctionTypeIdByName(word_junction_type_name);
				if (word_junction_type == -1) {
					LuaError(l, "Word junction type \"%s\" doesn't exist." _C_ word_junction_type_name.c_str());
				}
				++j;
				
				std::string affix_type_name = LuaToString(l, -1, j + 1);
				int affix_type = GetAffixTypeIdByName(affix_type_name);
				if (affix_type == -1) {
					LuaError(l, "Affix type \"%s\" doesn't exist." _C_ affix_type_name.c_str());
				}
				++j;
				
				int grammatical_number = GrammaticalNumberSingular;
				if (GetGrammaticalNumberIdByName(LuaToString(l, -1, j + 1)) != -1) {
					std::string grammatical_number_name = LuaToString(l, -1, j + 1);
					grammatical_number = GetGrammaticalNumberIdByName(grammatical_number_name);
					if (grammatical_number == -1) {
						LuaError(l, "Grammatical number \"%s\" doesn't exist." _C_ grammatical_number_name.c_str());
					}
					++j;
				}
				
				int grammatical_case = GrammaticalCaseNominative;
				if (GetGrammaticalCaseIdByName(LuaToString(l, -1, j + 1)) != -1) {
					std::string grammatical_case_name = LuaToString(l, -1, j + 1);
					grammatical_case = GetGrammaticalCaseIdByName(grammatical_case_name);
					if (grammatical_case == -1) {
						LuaError(l, "Grammatical case \"%s\" doesn't exist." _C_ grammatical_case_name.c_str());
					}
					++j;
				}
				
				int grammatical_tense = GrammaticalTenseNoTense;
				if (GetGrammaticalTenseIdByName(LuaToString(l, -1, j + 1)) != -1) {
					std::string grammatical_tense_name = LuaToString(l, -1, j + 1);
					grammatical_tense = GetGrammaticalTenseIdByName(grammatical_tense_name);
					if (grammatical_tense == -1) {
						LuaError(l, "Grammatical tense \"%s\" doesn't exist." _C_ grammatical_tense_name.c_str());
					}
					++j;
				}
				
				std::string type = LuaToString(l, -1, j + 1);
				word->IncreaseAffixNameType(type, word_junction_type, affix_type, grammatical_number, grammatical_case, grammatical_tense);
			}
		} else if (!strcmp(value, "Mod")) {
			word->Mod = LuaToString(l, -1);
		} else if (!strcmp(value, "MapWord")) { //to keep backwards compatibility
			word->Mod = Map.Info.Filename;
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	if (word->Language == -1) {
		LuaError(l, "Word \"%s\" has not been assigned to any language" _C_ word->Word.c_str());
	}
	
	if (word->Type == -1) {
		LuaError(l, "Word \"%s\" has no type" _C_ word->Word.c_str());
	}
	
	for (int i = 0; i < MaxGrammaticalNumbers; ++i) {
		for (int j = 0; j < MaxGrammaticalCases; ++j) {
			for (int k = 0; k < MaxGrammaticalTenses; ++k) {
				for (std::map<std::string, int>::iterator iterator = word->NameTypes[i][j][k].begin(); iterator != word->NameTypes[i][j][k].end(); ++iterator) {
					if (iterator->second > 0) {
						word->AddToLanguageNameTypes(iterator->first);
					}
				}
			}
		}
	}
						
	for (int i = 0; i < MaxWordJunctionTypes; ++i) {
		for (int j = 0; j < MaxAffixTypes; ++j) {
			for (int k = 0; k < MaxGrammaticalNumbers; ++k) {
				for (int n = 0; n < MaxGrammaticalCases; ++n) {
					for (int o = 0; o < MaxGrammaticalTenses; ++o) {
						for (std::map<std::string, int>::iterator iterator = word->AffixNameTypes[i][j][k][n][o].begin(); iterator != word->AffixNameTypes[i][j][k][n][o].end(); ++iterator) {
							if (iterator->second > 0) {
								word->AddToLanguageAffixNameTypes(iterator->first, i, j);
							}
						}
					}
				}
			}
		}
	}
	
	if (replaces != NULL) {
		PlayerRaces.Languages[word->Language]->RemoveWord(replaces);
	}
	
	if (!word->Mod.empty()) { //put the word in the language's ModWords vector if it is a mod word
		if (std::find(PlayerRaces.Languages[word->Language]->ModWords.begin(), PlayerRaces.Languages[word->Language]->ModWords.end(), word) == PlayerRaces.Languages[word->Language]->ModWords.end()) {
			PlayerRaces.Languages[word->Language]->ModWords.push_back(word);
		}
	}
	
	return 0;
}

/**
**  Get a civilization's data.
**
**  @param l  Lua state.
*/
static int CclGetCivilizationData(lua_State *l)
{
	if (lua_gettop(l) < 2) {
		LuaError(l, "incorrect argument");
	}
	std::string civilization_name = LuaToString(l, 1);
	int civilization_id = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	if (civilization_id == -1) {
		LuaError(l, "Civilization \"%s\" doesn't exist." _C_ civilization_name.c_str());
	}
	CCivilization *civilization = PlayerRaces.Civilizations[civilization_id];
	
	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Display")) {
		lua_pushstring(l, PlayerRaces.Display[civilization_id].c_str());
		return 1;
	} else if (!strcmp(data, "Description")) {
		lua_pushstring(l, civilization->Description.c_str());
		return 1;
	} else if (!strcmp(data, "Quote")) {
		lua_pushstring(l, civilization->Quote.c_str());
		return 1;
	} else if (!strcmp(data, "Background")) {
		lua_pushstring(l, civilization->Background.c_str());
		return 1;
	} else if (!strcmp(data, "Adjective")) {
		if (!civilization->Adjective.empty()) {
			lua_pushstring(l, civilization->Adjective.c_str());
		} else {
			lua_pushstring(l, PlayerRaces.Display[civilization_id].c_str());
		}
		return 1;
	} else if (!strcmp(data, "CalendarStartingYear")) {
		lua_pushnumber(l, civilization->CalendarStartingYear);
		return 1;
	} else if (!strcmp(data, "YearLabel")) {
		lua_pushstring(l, civilization->YearLabel.c_str());
		return 1;
	} else if (!strcmp(data, "NegativeYearLabel")) {
		lua_pushstring(l, civilization->NegativeYearLabel.c_str());
		return 1;
	} else if (!strcmp(data, "Playable")) {
		lua_pushboolean(l, PlayerRaces.Playable[civilization_id]);
		return 1;
	} else if (!strcmp(data, "Species")) {
		lua_pushstring(l, PlayerRaces.Species[civilization_id].c_str());
		return 1;
	} else if (!strcmp(data, "ParentCivilization")) {
		int parent_civilization = PlayerRaces.ParentCivilization[civilization_id];
		if (parent_civilization != -1) {
			lua_pushstring(l, PlayerRaces.Name[parent_civilization].c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Language")) {
		int language = PlayerRaces.GetCivilizationLanguage(civilization_id);
		if (language != -1) {
			lua_pushstring(l, PlayerRaces.Languages[language]->Ident.c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "DefaultColor")) {
		lua_pushstring(l, PlayerRaces.DefaultColor[civilization_id].c_str());
		return 1;
	} else if (!strcmp(data, "CivilizationUpgrade")) {
		lua_pushstring(l, PlayerRaces.CivilizationUpgrades[civilization_id].c_str());
		return 1;
	} else if (!strcmp(data, "MonthName")) {
		LuaCheckArgs(l, 3);		
		
		int month = GetMonthIdByName(LuaToString(l, 3));
		if (month == -1) {
			LuaError(l, "Month doesn't exist.");
		}
		
		lua_pushstring(l, civilization->GetMonthName(month).c_str());
		return 1;
	} else if (!strcmp(data, "DevelopsFrom")) {
		lua_createtable(l, PlayerRaces.DevelopsFrom[civilization_id].size(), 0);
		for (size_t i = 1; i <= PlayerRaces.DevelopsFrom[civilization_id].size(); ++i)
		{
			lua_pushstring(l, PlayerRaces.Name[PlayerRaces.DevelopsFrom[civilization_id][i-1]].c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "DevelopsTo")) {
		lua_createtable(l, PlayerRaces.DevelopsTo[civilization_id].size(), 0);
		for (size_t i = 1; i <= PlayerRaces.DevelopsTo[civilization_id].size(); ++i)
		{
			lua_pushstring(l, PlayerRaces.Name[PlayerRaces.DevelopsTo[civilization_id][i-1]].c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "Factions")) {
		bool is_mod = false;
		if (lua_gettop(l) >= 3) {
			is_mod = true;
		}
		
		std::string mod_file;

		if (is_mod) {
			mod_file = LuaToString(l, 3);
		}
		
		std::vector<std::string> factions;
		for (size_t i = 0; i < PlayerRaces.Factions[civilization_id].size(); ++i)
		{
			if (!is_mod || PlayerRaces.Factions[civilization_id][i]->Mod == mod_file) {
				factions.push_back(PlayerRaces.Factions[civilization_id][i]->Ident);
			}
		}
		
		lua_createtable(l, factions.size(), 0);
		for (size_t i = 1; i <= factions.size(); ++i)
		{
			lua_pushstring(l, factions[i-1].c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "Quests")) {
		lua_createtable(l, civilization->Quests.size(), 0);
		for (size_t i = 1; i <= civilization->Quests.size(); ++i)
		{
			lua_pushstring(l, civilization->Quests[i-1]->Ident.c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "ShipNames")) {
		lua_createtable(l, civilization->ShipNames.size(), 0);
		for (size_t i = 1; i <= civilization->ShipNames.size(); ++i)
		{
			lua_pushstring(l, civilization->ShipNames[i-1].c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else {
		LuaError(l, "Invalid field: %s" _C_ data);
	}

	return 0;
}

/**
**  Get a civilization's unit type/upgrade of a certain class.
**
**  @param l  Lua state.
*/
static int CclGetCivilizationClassUnitType(lua_State *l)
{
	LuaCheckArgs(l, 2);
	std::string class_name = LuaToString(l, 1);
	int class_id = GetUnitTypeClassIndexByName(class_name);
	int civilization = PlayerRaces.GetRaceIndexByName(LuaToString(l, 2));
	std::string unit_type_ident;
	if (civilization != -1 && class_id != -1) {
		int unit_type_id = PlayerRaces.GetCivilizationClassUnitType(civilization, class_id);
		if (unit_type_id != -1) {
			unit_type_ident = UnitTypes[unit_type_id]->Ident;
		}
	}
		
	if (unit_type_ident.empty()) { //if wasn't found, see if it is an upgrade class instead
		class_id = GetUpgradeClassIndexByName(class_name);
		if (civilization != -1 && class_id != -1) {
			int upgrade_id = PlayerRaces.GetCivilizationClassUpgrade(civilization, class_id);
			if (upgrade_id != -1) {
				unit_type_ident = AllUpgrades[upgrade_id]->Ident;
			}
		}
	}
	
	if (!unit_type_ident.empty()) {
		lua_pushstring(l, unit_type_ident.c_str());
	} else {
		lua_pushnil(l);
	}

	return 1;
}


/**
**  Get a faction's unit type/upgrade of a certain class.
**
**  @param l  Lua state.
*/
static int CclGetFactionClassUnitType(lua_State *l)
{
	LuaCheckArgs(l, 3);
	std::string class_name = LuaToString(l, 1);
	int class_id = GetUnitTypeClassIndexByName(class_name);
	int civilization = PlayerRaces.GetRaceIndexByName(LuaToString(l, 2));
	int faction = PlayerRaces.GetFactionIndexByName(civilization, LuaToString(l, 3));
	std::string unit_type_ident;
	if (civilization != -1 && class_id != -1) {
		int unit_type_id = PlayerRaces.GetFactionClassUnitType(civilization, faction, class_id);
		if (unit_type_id != -1) {
			unit_type_ident = UnitTypes[unit_type_id]->Ident;
		}
	}
		
	if (unit_type_ident.empty()) { //if wasn't found, see if it is an upgrade class instead
		class_id = GetUpgradeClassIndexByName(class_name);
		if (civilization != -1 && class_id != -1) {
			int upgrade_id = PlayerRaces.GetFactionClassUpgrade(civilization, faction, class_id);
			if (upgrade_id != -1) {
				unit_type_ident = AllUpgrades[upgrade_id]->Ident;
			}
		}
	}
	
	if (!unit_type_ident.empty()) {
		lua_pushstring(l, unit_type_ident.c_str());
	} else {
		lua_pushnil(l);
	}

	return 1;
}

/**
**  Define a faction.
**
**  @param l  Lua state.
*/
static int CclDefineFaction(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string faction_name = LuaToString(l, 1);
	CFaction *faction = NULL;
	int civilization = -1;
	std::string parent_faction;
	
	int faction_id = -1;
	for (int i = 0; i < MAX_RACES; ++i) {
		faction_id = PlayerRaces.GetFactionIndexByName(i, faction_name);
		if (faction_id != -1) { // redefinition
			faction = const_cast<CFaction *>(&(*PlayerRaces.Factions[i][faction_id]));
			civilization = faction->Civilization;
			if (faction->ParentFaction != -1) {
				parent_faction = PlayerRaces.Factions[i][faction->ParentFaction]->Ident;
			}
			break;
		}
	}
	
	if (faction_id == -1) {
		faction = new CFaction;
		faction->Ident = faction_name;
	}
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Civilization")) {
			if (civilization == -1) { //don't change the civilization in redefinitions
				civilization = PlayerRaces.GetRaceIndexByName(LuaToString(l, -1));
				
				faction->ID = PlayerRaces.Factions[civilization].size();
				PlayerRaces.Factions[civilization].push_back(faction);
				SetFactionStringToIndex(civilization, faction->Ident, faction->ID);
				faction->Civilization = civilization;
			}
		} else if (!strcmp(value, "Name")) {
			faction->Name = LuaToString(l, -1);
		} else if (!strcmp(value, "Description")) {
			faction->Description = LuaToString(l, -1);
		} else if (!strcmp(value, "Quote")) {
			faction->Quote = LuaToString(l, -1);
		} else if (!strcmp(value, "Background")) {
			faction->Background = LuaToString(l, -1);
		} else if (!strcmp(value, "Type")) {
			std::string faction_type_name = LuaToString(l, -1);
			int faction_type = GetFactionTypeIdByName(faction_type_name);
			if (faction_type != -1) {
				faction->Type = faction_type;
			} else {
				LuaError(l, "Faction type \"%s\" doesn't exist." _C_ faction_type_name.c_str());
			}
		} else if (!strcmp(value, "Colors")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			faction->Colors.clear(); //remove previously defined colors
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				std::string color_name = LuaToString(l, -1, k + 1);
				int color = GetPlayerColorIndexByName(color_name);
				if (color != -1) {
					faction->Colors.push_back(color);
				} else {
					LuaError(l, "Player color \"%s\" doesn't exist." _C_ color_name.c_str());
				}
			}
		} else if (!strcmp(value, "DefaultTier")) {
			std::string faction_tier_name = LuaToString(l, -1);
			int faction_tier = GetFactionTierIdByName(faction_tier_name);
			if (faction_tier != -1) {
				faction->DefaultTier = faction_tier;
			} else {
				LuaError(l, "Faction tier \"%s\" doesn't exist." _C_ faction_tier_name.c_str());
			}
		} else if (!strcmp(value, "DefaultGovernmentType")) {
			std::string government_type_name = LuaToString(l, -1);
			int government_type = GetGovernmentTypeIdByName(government_type_name);
			if (government_type != -1) {
				faction->DefaultGovernmentType = government_type;
			} else {
				LuaError(l, "Government type \"%s\" doesn't exist." _C_ government_type_name.c_str());
			}
		} else if (!strcmp(value, "DefaultAI")) {
			faction->DefaultAI = LuaToString(l, -1);
		} else if (!strcmp(value, "ParentFaction")) {
			parent_faction = LuaToString(l, -1);
		} else if (!strcmp(value, "Language")) {
			std::string language_name = LuaToString(l, -1);
			int language = PlayerRaces.GetLanguageIndexByIdent(language_name);
			
			if (language != -1) {
				faction->Language = language;
				PlayerRaces.Languages[language]->UsedByCivilizationOrFaction = true;
			} else if (language_name.empty()) {
				faction->Language = language; // to allow redefinitions to remove the language setting
			} else {
				LuaError(l, "Language \"%s\" not found." _C_ language_name.c_str());
			}
		} else if (!strcmp(value, "Playable")) {
			faction->Playable = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "DefaultStartPos")) {
			CclGetPos(l, &faction->DefaultStartPos.x, &faction->DefaultStartPos.y);
		} else if (!strcmp(value, "DevelopsTo")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				faction->DevelopsTo.push_back(LuaToString(l, -1, k + 1));
			}
		} else if (!strcmp(value, "SplitsTo")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				faction->SplitsTo.push_back(LuaToString(l, -1, k + 1));
			}
		} else if (!strcmp(value, "Titles")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				int government_type = GetGovernmentTypeIdByName(LuaToString(l, -1, k + 1));
				++k;
				int faction_tier = GetFactionTierIdByName(LuaToString(l, -1, k + 1));
				++k;
				faction->Titles[government_type][faction_tier] = LuaToString(l, -1, k + 1);
			}
		} else if (!strcmp(value, "MinisterTitles")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				int title = GetCharacterTitleIdByName(LuaToString(l, -1, k + 1));
				++k;
				int gender = GetGenderIdByName(LuaToString(l, -1, k + 1));
				++k;
				int government_type = GetGovernmentTypeIdByName(LuaToString(l, -1, k + 1));
				++k;
				int faction_tier = GetFactionTierIdByName(LuaToString(l, -1, k + 1));
				++k;
				faction->MinisterTitles[title][gender][government_type][faction_tier] = LuaToString(l, -1, k + 1);
			}
		} else if (!strcmp(value, "FactionUpgrade")) {
			faction->FactionUpgrade = LuaToString(l, -1);
		} else if (!strcmp(value, "ButtonIcons")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				std::string button_action_name = LuaToString(l, -1, j + 1);
				int button_action = GetButtonActionIdByName(button_action_name);
				if (button_action != -1) {
					++j;
					faction->ButtonIcons[button_action].Name = LuaToString(l, -1, j + 1);
					faction->ButtonIcons[button_action].Icon = NULL;
					faction->ButtonIcons[button_action].Load();
				} else {
					LuaError(l, "Button action \"%s\" doesn't exist." _C_ button_action_name.c_str());
				}
			}
		} else if (!strcmp(value, "UIFillers")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			
			faction->UIFillers.clear();
			
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				CFiller filler = CFiller();
				std::string filler_file = LuaToString(l, -1, j + 1);
				if (filler_file.empty()) {
					LuaError(l, "Filler graphic file is empty.");
				}
				filler.G = CGraphic::New(filler_file);
				++j;
				filler.X = LuaToNumber(l, -1, j + 1);
				++j;
				filler.Y = LuaToNumber(l, -1, j + 1);
				faction->UIFillers.push_back(filler);
			}
		} else if (!strcmp(value, "PersonalNames")) {
			faction->PersonalNames.clear();
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				int gender_id = GetGenderIdByName(LuaToString(l, -1, j + 1));
				if (gender_id == -1) {
					gender_id = NoGender;
				} else {
					++j;
				}
				
				faction->PersonalNames[gender_id].push_back(LuaToString(l, -1, j + 1));
			}
		} else if (!strcmp(value, "FamilyNames")) {
			faction->FamilyNames.clear();
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				faction->FamilyNames.push_back(LuaToString(l, -1, j + 1));
			}
		} else if (!strcmp(value, "SettlementNames")) {
			faction->SettlementNames.clear();
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				faction->SettlementNames.push_back(LuaToString(l, -1, j + 1));
			}
		} else if (!strcmp(value, "ProvinceNames")) {
			faction->ProvinceNames.clear();
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				faction->ProvinceNames.push_back(LuaToString(l, -1, j + 1));
			}
		} else if (!strcmp(value, "ShipNames")) {
			faction->ShipNames.clear();
			const int args = lua_rawlen(l, -1);
			for (int j = 0; j < args; ++j) {
				faction->ShipNames.push_back(LuaToString(l, -1, j + 1));
			}
		} else if (!strcmp(value, "HistoricalFactionDerivations")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				int year = LuaToNumber(l, -1, j + 1);
				++j;
				std::string predecessor_civilization_name = LuaToString(l, -1, j + 1);
				int predecessor_civilization = PlayerRaces.GetRaceIndexByName(predecessor_civilization_name.c_str());
				++j;
				std::string predecessor_faction_name = LuaToString(l, -1, j + 1);
				int predecessor_faction = PlayerRaces.GetFactionIndexByName(predecessor_civilization, predecessor_faction_name);
				if (predecessor_faction == -1) {
					LuaError(l, "Faction \"%s\" doesn't exist." _C_ predecessor_faction_name.c_str());
				}
				faction->HistoricalFactionDerivations[year] = PlayerRaces.Factions[predecessor_civilization][predecessor_faction];
			}
		} else if (!strcmp(value, "HistoricalTechnologies")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				std::string technology_ident = LuaToString(l, -1, j + 1);
				++j;
				int year = LuaToNumber(l, -1, j + 1);
				faction->HistoricalTechnologies[technology_ident] = year;
			}
		} else if (!strcmp(value, "HistoricalTiers")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				int year = LuaToNumber(l, -1, j + 1);
				++j;
				std::string faction_tier_name = LuaToString(l, -1, j + 1);
				int faction_tier = GetFactionTierIdByName(faction_tier_name);
				if (faction_tier == -1) {
					LuaError(l, "Faction tier \"%s\" doesn't exist." _C_ faction_tier_name.c_str());
				}
				faction->HistoricalTiers[year] = faction_tier;
			}
		} else if (!strcmp(value, "HistoricalGovernmentTypes")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				int year = LuaToNumber(l, -1, j + 1);
				++j;
				std::string government_type_name = LuaToString(l, -1, j + 1);
				int government_type = GetGovernmentTypeIdByName(government_type_name);
				if (government_type == -1) {
					LuaError(l, "Government type \"%s\" doesn't exist." _C_ government_type_name.c_str());
				}
				faction->HistoricalGovernmentTypes[year] = government_type;
			}
		} else if (!strcmp(value, "HistoricalDiplomacyStates")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				int year = LuaToNumber(l, -1, j + 1);
				++j;
				
				std::string diplomacy_state_civilization_name = LuaToString(l, -1, j + 1);
				int diplomacy_state_civilization = PlayerRaces.GetRaceIndexByName(diplomacy_state_civilization_name.c_str());
				++j;
				std::string diplomacy_state_faction_name = LuaToString(l, -1, j + 1);
				int diplomacy_state_faction = PlayerRaces.GetFactionIndexByName(diplomacy_state_civilization, diplomacy_state_faction_name);
				if (diplomacy_state_faction == -1) {
					LuaError(l, "Faction \"%s\" doesn't exist." _C_ diplomacy_state_faction_name.c_str());
				}
				++j;

				std::string diplomacy_state_name = LuaToString(l, -1, j + 1);
				int diplomacy_state = GetDiplomacyStateIdByName(diplomacy_state_name);
				if (diplomacy_state == -1) {
					LuaError(l, "Diplomacy state \"%s\" doesn't exist." _C_ diplomacy_state_name.c_str());
				}
				faction->HistoricalDiplomacyStates[std::pair<int, CFaction *>(year, PlayerRaces.Factions[diplomacy_state_civilization][diplomacy_state_faction])] = diplomacy_state;
			}
		} else if (!strcmp(value, "HistoricalCapitals")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				int year = LuaToNumber(l, -1, j + 1);
				++j;
				std::string province_name = LuaToString(l, -1, j + 1);
				faction->HistoricalCapitals[year] = province_name;
			}
		} else if (!strcmp(value, "Mod")) {
			faction->Mod = LuaToString(l, -1);
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	if (civilization != -1 && !parent_faction.empty()) { //process this here, since we have no guarantee that the civilization will get processed before the parent faction
		faction->ParentFaction = PlayerRaces.GetFactionIndexByName(civilization, parent_faction);
		
		if (faction->ParentFaction == -1) { //if a parent faction was set but wasn't found, give an error
			LuaError(l, "Faction %s doesn't exist" _C_ parent_faction.c_str());
		}
		
		if (faction->ParentFaction != -1 && faction->FactionUpgrade.empty()) { //if the faction has no faction upgrade, inherit that of its parent faction
			faction->FactionUpgrade = PlayerRaces.Factions[civilization][faction->ParentFaction]->FactionUpgrade;
		}
		
		if (faction->ParentFaction != -1) { //inherit button icons from parent civilization, for button actions which none are specified
			for (std::map<int, IconConfig>::iterator iterator = PlayerRaces.Factions[civilization][faction->ParentFaction]->ButtonIcons.begin(); iterator != PlayerRaces.Factions[civilization][faction->ParentFaction]->ButtonIcons.end(); ++iterator) {
				if (faction->ButtonIcons.find(iterator->first) == faction->ButtonIcons.end()) {
					faction->ButtonIcons[iterator->first] = iterator->second;
				}
			}
			
			if (faction->Language == -1) {
				if (faction->PersonalNames.size() == 0) {
					for (std::map<int, std::vector<std::string>>::iterator iterator = PlayerRaces.Factions[civilization][faction->ParentFaction]->PersonalNames.begin(); iterator != PlayerRaces.Factions[civilization][faction->ParentFaction]->PersonalNames.end(); ++iterator) {
						for (size_t i = 0; i < iterator->second.size(); ++i) {
							faction->PersonalNames[iterator->first].push_back(iterator->second[i]);				
						}
					}
				}

				if (faction->FamilyNames.size() == 0) {
					for (size_t i = 0; i < PlayerRaces.Factions[civilization][faction->ParentFaction]->FamilyNames.size(); ++i) {
						faction->FamilyNames.push_back(PlayerRaces.Factions[civilization][faction->ParentFaction]->FamilyNames[i]);
					}
				}

				if (faction->SettlementNames.size() == 0) {
					for (size_t i = 0; i < PlayerRaces.Factions[civilization][faction->ParentFaction]->SettlementNames.size(); ++i) {
						faction->SettlementNames.push_back(PlayerRaces.Factions[civilization][faction->ParentFaction]->SettlementNames[i]);
					}
				}

				if (faction->ProvinceNames.size() == 0) {
					for (size_t i = 0; i < PlayerRaces.Factions[civilization][faction->ParentFaction]->ProvinceNames.size(); ++i) {
						faction->ProvinceNames.push_back(PlayerRaces.Factions[civilization][faction->ParentFaction]->ProvinceNames[i]);
					}
				}

				if (faction->ShipNames.size() == 0) {
					for (size_t i = 0; i < PlayerRaces.Factions[civilization][faction->ParentFaction]->ShipNames.size(); ++i) {
						faction->ShipNames.push_back(PlayerRaces.Factions[civilization][faction->ParentFaction]->ShipNames[i]);
					}
				}
			}
			
			for (std::map<std::string, int>::iterator iterator = PlayerRaces.Factions[civilization][faction->ParentFaction]->HistoricalTechnologies.begin(); iterator != PlayerRaces.Factions[civilization][faction->ParentFaction]->HistoricalTechnologies.end(); ++iterator) {
				if (faction->HistoricalTechnologies.find(iterator->first) == faction->HistoricalTechnologies.end()) {
					faction->HistoricalTechnologies[iterator->first] = iterator->second;
				}
			}
		}
	} else if (parent_faction.empty()) {
		faction->ParentFaction = -1; // to allow redefinitions to remove the parent faction setting
	}
	
	return 0;
}

/**
**  Define a religion.
**
**  @param l  Lua state.
*/
static int CclDefineReligion(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string religion_ident = LuaToString(l, 1);
	CReligion *religion = NULL;
	int religion_id = PlayerRaces.GetReligionIndexByIdent(religion_ident);
	if (religion_id != -1) {
		religion = PlayerRaces.Religions[religion_id];
	} else {
		religion = new CReligion;
		PlayerRaces.Religions.push_back(religion);
	}
	
	religion->Ident = religion_ident;
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Name")) {
			religion->Name = LuaToString(l, -1);
		} else if (!strcmp(value, "Description")) {
			religion->Description = LuaToString(l, -1);
		} else if (!strcmp(value, "Background")) {
			religion->Background = LuaToString(l, -1);
		} else if (!strcmp(value, "Quote")) {
			religion->Quote = LuaToString(l, -1);
		} else if (!strcmp(value, "CulturalDeities")) {
			religion->CulturalDeities = LuaToBoolean(l, -1);
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	return 0;
}

/**
**  Define a deity domain.
**
**  @param l  Lua state.
*/
static int CclDefineDeityDomain(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string deity_domain_ident = LuaToString(l, 1);
	CDeityDomain *deity_domain = NULL;
	int deity_domain_id = PlayerRaces.GetDeityDomainIndexByIdent(deity_domain_ident);
	if (deity_domain_id != -1) {
		deity_domain = PlayerRaces.DeityDomains[deity_domain_id];
	} else {
		deity_domain = new CDeityDomain;
		PlayerRaces.DeityDomains.push_back(deity_domain);
	}
	
	deity_domain->Ident = deity_domain_ident;
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Name")) {
			deity_domain->Name = LuaToString(l, -1);
		} else if (!strcmp(value, "Abilities")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				CUpgrade *ability = CUpgrade::Get(LuaToString(l, -1, j + 1));
				if (!ability || !ability->Ability) {
					LuaError(l, "Ability doesn't exist.");
				}

				deity_domain->Abilities.push_back(ability);
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	return 0;
}

/**
**  Define a deity.
**
**  @param l  Lua state.
*/
static int CclDefineDeity(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string deity_ident = LuaToString(l, 1);
	CDeity *deity = NULL;
	int deity_id = PlayerRaces.GetDeityIndexByIdent(deity_ident);
	if (deity_id != -1) {
		deity = PlayerRaces.Deities[deity_id];
	} else {
		deity = new CDeity;
		PlayerRaces.Deities.push_back(deity);
	}
	
	deity->Ident = deity_ident;
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Name")) {
			deity->Name = LuaToString(l, -1);
		} else if (!strcmp(value, "Pantheon")) {
			deity->Pantheon = LuaToString(l, -1);
		} else if (!strcmp(value, "Upgrade")) {
			deity->UpgradeIdent = LuaToString(l, -1);
		} else if (!strcmp(value, "Gender")) {
			deity->Gender = GetGenderIdByName(LuaToString(l, -1));
		} else if (!strcmp(value, "Major")) {
			deity->Major = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Description")) {
			deity->Description = LuaToString(l, -1);
		} else if (!strcmp(value, "Background")) {
			deity->Background = LuaToString(l, -1);
		} else if (!strcmp(value, "Quote")) {
			deity->Quote = LuaToString(l, -1);
		} else if (!strcmp(value, "HomePlane")) {
			CPlane *plane = GetPlane(LuaToString(l, -1));
			if (!plane) {
				LuaError(l, "Plane doesn't exist.");
			}
			deity->HomePlane = plane;
		} else if (!strcmp(value, "Icon")) {
			deity->Icon.Name = LuaToString(l, -1);
			deity->Icon.Icon = NULL;
			deity->Icon.Load();
			deity->Icon.Icon->Load();
		} else if (!strcmp(value, "Civilizations")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				int civilization = PlayerRaces.GetRaceIndexByName(LuaToString(l, -1, j + 1));
				if (civilization == -1) {
					LuaError(l, "Civilization doesn't exist.");
				}

				deity->Civilizations.push_back(civilization);
			}
		} else if (!strcmp(value, "Religions")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				int religion_id = PlayerRaces.GetReligionIndexByIdent(LuaToString(l, -1, j + 1));
				if (religion_id == -1) {
					LuaError(l, "Religion doesn't exist.");
				}

				deity->Religions.push_back(PlayerRaces.Religions[religion_id]);
			}
		} else if (!strcmp(value, "Domains")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				int domain_id = PlayerRaces.GetDeityDomainIndexByIdent(LuaToString(l, -1, j + 1));
				if (domain_id == -1) {
					LuaError(l, "Domain doesn't exist.");
				}

				deity->Domains.push_back(PlayerRaces.DeityDomains[domain_id]);
			}
		} else if (!strcmp(value, "Abilities")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				CUpgrade *ability = CUpgrade::Get(LuaToString(l, -1, j + 1));
				if (!ability || !ability->Ability) {
					LuaError(l, "Ability doesn't exist.");
				}

				if (std::find(deity->Abilities.begin(), deity->Abilities.end(), ability) == deity->Abilities.end()) {
					deity->Abilities.push_back(ability);
				}
			}
		} else if (!strcmp(value, "Feasts")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				std::string feast = LuaToString(l, -1, j + 1);

				deity->Feasts.push_back(feast);
			}
		} else if (!strcmp(value, "CulturalNames")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				int civilization = PlayerRaces.GetRaceIndexByName(LuaToString(l, -1, j + 1));
				if (civilization == -1) {
					LuaError(l, "Civilization doesn't exist.");
				}
				++j;
				
				std::string cultural_name = LuaToString(l, -1, j + 1);
				deity->CulturalNames[civilization] = cultural_name;
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	if (deity->Major && deity->Domains.size() > 3) { // major deities can only have up to three domains
		deity->Domains.resize(3);
	} else if (!deity->Major && deity->Domains.size() > 1) { // minor deities can only have one domain
		deity->Domains.resize(1);
	}
	
	for (size_t i = 0; i < deity->Domains.size(); ++i) {
		for (size_t j = 0; j < deity->Domains[i]->Abilities.size(); ++j) {
			if (std::find(deity->Abilities.begin(), deity->Abilities.end(), deity->Domains[i]->Abilities[j]) == deity->Abilities.end()) {
				deity->Abilities.push_back(deity->Domains[i]->Abilities[j]);
			}
		}
	}
	
	return 0;
}

/**
**  Define a language.
**
**  @param l  Lua state.
*/
static int CclDefineLanguage(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string language_ident = LuaToString(l, 1);
	CLanguage *language = NULL;
	int language_id = PlayerRaces.GetLanguageIndexByIdent(language_ident);
	if (language_id == -1) {
		language = new CLanguage;
		PlayerRaces.Languages.push_back(language);
	} else {
		language = const_cast<CLanguage *>(&(*PlayerRaces.Languages[language_id]));
	}
	
	language->Ident = language_ident;
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Name")) {
			language->Name = LuaToString(l, -1);
		} else if (!strcmp(value, "Family")) {
			language->Family = LuaToString(l, -1);
		} else if (!strcmp(value, "GenerateMissingWords")) {
			language->GenerateMissingWords = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "SkipNameTypeInheritance")) {
			language->SkipNameTypeInheritance = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "DialectOf")) {
			int parent_language = PlayerRaces.GetLanguageIndexByIdent(LuaToString(l, -1));
			if (parent_language != -1) {
				language->DialectOf = PlayerRaces.Languages[parent_language];
				PlayerRaces.Languages[parent_language]->Dialects.push_back(language);
			} else {
				LuaError(l, "Language not found.");
			}
		} else if (!strcmp(value, "NounEndings")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				std::string grammatical_number_name = LuaToString(l, -1, k + 1);
				int grammatical_number = GetGrammaticalNumberIdByName(grammatical_number_name);
				if (grammatical_number == -1) {
					LuaError(l, "Grammatical number \"%s\" doesn't exist." _C_ grammatical_number_name.c_str());
				}
				++k;
				
				std::string grammatical_case_name = LuaToString(l, -1, k + 1);
				int grammatical_case = GetGrammaticalCaseIdByName(grammatical_case_name);
				if (grammatical_case == -1) {
					LuaError(l, "Grammatical case \"%s\" doesn't exist." _C_ grammatical_case_name.c_str());
				}
				++k;
				
				int word_junction_type = WordJunctionTypeNoWordJunction;
				if (GetWordJunctionTypeIdByName(LuaToString(l, -1, k + 1)) != -1) {
					std::string word_junction_type_name = LuaToString(l, -1, k + 1);
					int word_junction_type = GetWordJunctionTypeIdByName(word_junction_type_name);
					if (word_junction_type == -1) {
						LuaError(l, "Word junction type \"%s\" doesn't exist." _C_ word_junction_type_name.c_str());
					}
					++k;
				}

				language->NounEndings[grammatical_number][grammatical_case][word_junction_type] = LuaToString(l, -1, k + 1);
			}
		} else if (!strcmp(value, "AdjectiveEndings")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				std::string article_type_name = LuaToString(l, -1, k + 1);
				int article_type = GetArticleTypeIdByName(article_type_name);
				if (article_type == -1) {
					LuaError(l, "Article type \"%s\" doesn't exist." _C_ article_type_name.c_str());
				}
				++k;

				std::string grammatical_case_name = LuaToString(l, -1, k + 1);
				int grammatical_case = GetGrammaticalCaseIdByName(grammatical_case_name);
				if (grammatical_case == -1) {
					LuaError(l, "Grammatical case \"%s\" doesn't exist." _C_ grammatical_case_name.c_str());
				}
				++k;

				std::string grammatical_number_name = LuaToString(l, -1, k + 1);
				int grammatical_number = GetGrammaticalNumberIdByName(grammatical_number_name);
				if (grammatical_number == -1) {
					LuaError(l, "Grammatical number \"%s\" doesn't exist." _C_ grammatical_number_name.c_str());
				}
				++k;
				
				std::string grammatical_gender_name = LuaToString(l, -1, k + 1);
				int grammatical_gender = GetGrammaticalGenderIdByName(grammatical_gender_name);
				if (grammatical_gender == -1) {
					LuaError(l, "Grammatical gender \"%s\" doesn't exist." _C_ grammatical_gender_name.c_str());
				}
				++k;
				
				language->AdjectiveEndings[article_type][grammatical_case][grammatical_number][grammatical_gender] = LuaToString(l, -1, k + 1);
			}
		} else if (!strcmp(value, "NameTranslations")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				std::string translation_from = LuaToString(l, -1, k + 1); //name to be translated
				++k;
				std::string translation_to = LuaToString(l, -1, k + 1); //name translation
				language->NameTranslations[translation_from].push_back(translation_to);
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	return 0;
}
//Wyrmgus end

/**
**  Get the civilizations.
**
**  @param l  Lua state.
*/
static int CclGetCivilizations(lua_State *l)
{
	LuaCheckArgs(l, 0);

	int civilization_count = 0;
	for (int i = 0; i < MAX_RACES; ++i) {
		if (!PlayerRaces.Name[i].empty()) { //require visibility to ignore the neutral civilization
			civilization_count += 1;
		}
	}

	lua_createtable(l, civilization_count, 0);
	for (int i = 1; i <= civilization_count; ++i)
	{
		lua_pushstring(l, PlayerRaces.Name[i-1].c_str());
		lua_rawseti(l, -2, i);
	}
	
	return 1;
}

/**
**  Get a civilization's factions.
**
**  @param l  Lua state.
*/
static int CclGetCivilizationFactionNames(lua_State *l)
{
	LuaCheckArgs(l, 1);
	int civilization = PlayerRaces.GetRaceIndexByName(LuaToString(l, 1));
	lua_pop(l, 1);

	lua_createtable(l, PlayerRaces.Factions[civilization].size(), 0);
	for (size_t i = 1; i <= PlayerRaces.Factions[civilization].size(); ++i)
	{
		lua_pushstring(l, PlayerRaces.Factions[civilization][i-1]->Ident.c_str());
		lua_rawseti(l, -2, i);
	}
	
	return 1;
}

/**
**  Get the factions.
**
**  @param l  Lua state.
*/
static int CclGetFactions(lua_State *l)
{
	int civilization = -1;
	if (lua_gettop(l) >= 1) {
		civilization = PlayerRaces.GetRaceIndexByName(LuaToString(l, 1));
	}
	
	std::vector<std::string> factions;
	if (civilization != -1) {
		for (size_t i = 0; i < PlayerRaces.Factions[civilization].size(); ++i)
		{
			factions.push_back(PlayerRaces.Factions[civilization][i]->Ident);
		}
	} else {
		for (int i = 0; i < MAX_RACES; ++i)
		{
			for (size_t j = 0; j < PlayerRaces.Factions[i].size(); ++j)
			{
				factions.push_back(PlayerRaces.Factions[i][j]->Ident);
			}
		}
	}
		
	lua_createtable(l, factions.size(), 0);
	for (size_t i = 1; i <= factions.size(); ++i)
	{
		lua_pushstring(l, factions[i-1].c_str());
		lua_rawseti(l, -2, i);
	}
	
	return 1;
}

/**
**  Get the player colors.
**
**  @param l  Lua state.
*/
static int CclGetPlayerColors(lua_State *l)
{
	std::vector<std::string> player_colors;
	for (int i = 0; i < PlayerColorMax; ++i)
	{
		if (!PlayerColorNames[i].empty()) {
			player_colors.push_back(PlayerColorNames[i]);
		}
	}
		
	lua_createtable(l, player_colors.size(), 0);
	for (size_t i = 1; i <= player_colors.size(); ++i)
	{
		lua_pushstring(l, player_colors[i-1].c_str());
		lua_rawseti(l, -2, i);
	}
	
	return 1;
}

/**
**  Get faction data.
**
**  @param l  Lua state.
*/
static int CclGetFactionData(lua_State *l)
{
	LuaCheckArgs(l, 3);
	const int civilization = PlayerRaces.GetRaceIndexByName(LuaToString(l, 1));
	if (civilization == -1) {
		LuaError(l, "Civilization doesn't exist.");
	}
	std::string faction_name = LuaToString(l, 2);
	int faction = PlayerRaces.GetFactionIndexByName(civilization, faction_name);
	if (faction == -1) {
		LuaError(l, "Faction \"%s\" doesn't exist." _C_ faction_name.c_str());
	}
	
	const char *data = LuaToString(l, 3);

	if (!strcmp(data, "Name")) {
		lua_pushstring(l, PlayerRaces.Factions[civilization][faction]->Name.c_str());
		return 1;
	} else if (!strcmp(data, "Description")) {
		lua_pushstring(l, PlayerRaces.Factions[civilization][faction]->Description.c_str());
		return 1;
	} else if (!strcmp(data, "Quote")) {
		lua_pushstring(l, PlayerRaces.Factions[civilization][faction]->Quote.c_str());
		return 1;
	} else if (!strcmp(data, "Background")) {
		lua_pushstring(l, PlayerRaces.Factions[civilization][faction]->Background.c_str());
		return 1;
	} else if (!strcmp(data, "Type")) {
		lua_pushstring(l, GetFactionTypeNameById(PlayerRaces.Factions[civilization][faction]->Type).c_str());
		return 1;
	} else if (!strcmp(data, "Color")) {
		if (PlayerRaces.Factions[civilization][faction]->Colors.size() > 0) {
			lua_pushstring(l, PlayerColorNames[PlayerRaces.Factions[civilization][faction]->Colors[0]].c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Playable")) {
		lua_pushboolean(l, PlayerRaces.Factions[civilization][faction]->Playable);
		return 1;
	} else if (!strcmp(data, "Language")) {
		int language = PlayerRaces.GetFactionLanguage(civilization, faction);
		if (language != -1) {
			lua_pushstring(l, PlayerRaces.Languages[language]->Ident.c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "FactionUpgrade")) {
		lua_pushstring(l, PlayerRaces.Factions[civilization][faction]->FactionUpgrade.c_str());
		return 1;
	} else if (!strcmp(data, "ParentFaction")) {
		if (PlayerRaces.Factions[civilization][faction]->ParentFaction != -1) {
			lua_pushstring(l, PlayerRaces.Factions[civilization][PlayerRaces.Factions[civilization][faction]->ParentFaction]->Ident.c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else {
		LuaError(l, "Invalid field: %s" _C_ data);
	}

	return 0;
}

static int CclGetFactionDevelopsTo(lua_State *l)
{
	LuaCheckArgs(l, 3);
	int civilization = PlayerRaces.GetRaceIndexByName(LuaToString(l, 1));
	int faction = PlayerRaces.GetFactionIndexByName(civilization, LuaToString(l, 2));
	int current_civilization = PlayerRaces.GetRaceIndexByName(LuaToString(l, 3));

	int faction_count = 0;
	std::string develops_to_factions[FactionMax];
	for (size_t i = 0; i < PlayerRaces.Factions[civilization][faction]->DevelopsTo.size(); ++i) {
		if (PlayerRaces.GetFactionIndexByName(current_civilization, PlayerRaces.Factions[civilization][faction]->DevelopsTo[i]) != -1) {
			develops_to_factions[faction_count] = PlayerRaces.Factions[civilization][faction]->DevelopsTo[i];
			faction_count += 1;
		}
	}

	lua_createtable(l, faction_count, 0);
	for (int i = 1; i <= faction_count; ++i)
	{
		lua_pushstring(l, develops_to_factions[i-1].c_str());
		lua_rawseti(l, -2, i);
	}
	
	return 1;
}
//Wyrmgus end

/**
**  Define player colors
**
**  @param l  Lua state.
*/
static int CclDefinePlayerColors(lua_State *l)
{
	LuaCheckArgs(l, 1);
	if (!lua_istable(l, 1)) {
		LuaError(l, "incorrect argument");
	}

	const int args = lua_rawlen(l, 1);
	for (int i = 0; i < args; ++i) {
		PlayerColorNames[i / 2] = LuaToString(l, 1, i + 1);
		++i;
		lua_rawgeti(l, 1, i + 1);
		if (!lua_istable(l, -1)) {
			LuaError(l, "incorrect argument");
		}
		const int numcolors = lua_rawlen(l, -1);
		if (numcolors != PlayerColorIndexCount) {
			LuaError(l, "You should use %d colors (See DefinePlayerColorIndex())" _C_ PlayerColorIndexCount);
		}
		for (int j = 0; j < numcolors; ++j) {
			lua_rawgeti(l, -1, j + 1);
			PlayerColorsRGB[i / 2][j].Parse(l);
			lua_pop(l, 1);
		}
	}

	return 0;
}

/**
**  Make new player colors
**
**  @param l  Lua state.
*/
static int CclNewPlayerColors(lua_State *l)
{
	LuaCheckArgs(l, 0);
	SetPlayersPalette();

	return 0;
}

/**
**  Define player color indexes
**
**  @param l  Lua state.
*/
static int CclDefinePlayerColorIndex(lua_State *l)
{
	LuaCheckArgs(l, 2);
	PlayerColorIndexStart = LuaToNumber(l, 1);
	PlayerColorIndexCount = LuaToNumber(l, 2);

	//Wyrmgus start
//	for (int i = 0; i < PlayerMax; ++i) {
	for (int i = 0; i < PlayerColorMax; ++i) {
	//Wyrmgus end
		PlayerColorsRGB[i].clear();
		PlayerColorsRGB[i].resize(PlayerColorIndexCount);
		PlayerColors[i].clear();
		PlayerColors[i].resize(PlayerColorIndexCount, 0);
	}
	return 0;
}

//Wyrmgus start
/**
**  Define conversible player colors.
**
**  @param l  Lua state.
*/
static int CclDefineConversiblePlayerColors(lua_State *l)
{
	ConversiblePlayerColors.clear();
	
	const unsigned int args = lua_gettop(l);
	for (unsigned int i = 0; i < args; ++i) {
		std::string player_color_name = LuaToString(l, i + 1);
		int player_color = GetPlayerColorIndexByName(player_color_name);
		if (player_color != -1) {
			ConversiblePlayerColors.push_back(player_color);
		} else {
			LuaError(l, "Player color \"%s\" doesn't exist." _C_ player_color_name.c_str());
		}
	}
	
	return 0;
}

/**
**  Define skin colors
**
**  @param l  Lua state.
*/
static int CclDefineSkinColors(lua_State *l)
{
	LuaCheckArgs(l, 1);
	if (!lua_istable(l, 1)) {
		LuaError(l, "incorrect argument");
	}

	bool first = true;
	int skin_color_count = 0;
	SkinColorNames[0] = "default";
	const int args = lua_rawlen(l, 1);
	for (int i = 0; i < args; ++i) {
		SkinColorNames[(i / 2) + 1] = LuaToString(l, 1, i + 1);
		++i;
		lua_rawgeti(l, 1, i + 1);
		if (!lua_istable(l, -1)) {
			LuaError(l, "incorrect argument");
		}
		const int numcolors = lua_rawlen(l, -1);
		if (first) {
			skin_color_count = numcolors;
			first = false;
		}
		if (numcolors != skin_color_count) {
			LuaError(l, "You should use the same quantity of colors for all skin colors");
		}
		SkinColorsRGB[(i / 2) + 1].clear();
		SkinColorsRGB[(i / 2) + 1].resize(skin_color_count);
		for (int j = 0; j < numcolors; ++j) {
			lua_rawgeti(l, -1, j + 1);
			SkinColorsRGB[(i / 2) + 1][j].Parse(l);
			lua_pop(l, 1);
		}
	}

	return 0;
}

/**
**  Define hair colors
**
**  @param l  Lua state.
*/
static int CclDefineHairColors(lua_State *l)
{
	LuaCheckArgs(l, 1);
	if (!lua_istable(l, 1)) {
		LuaError(l, "incorrect argument");
	}

	bool first = true;
	int hair_color_count = 0;
	HairColorNames[0] = "default";
	const int args = lua_rawlen(l, 1);
	for (int i = 0; i < args; ++i) {
		HairColorNames[(i / 2) + 1] = LuaToString(l, 1, i + 1);
		++i;
		lua_rawgeti(l, 1, i + 1);
		if (!lua_istable(l, -1)) {
			LuaError(l, "incorrect argument");
		}
		const int numcolors = lua_rawlen(l, -1);
		if (first) {
			hair_color_count = numcolors;
			first = false;
		}
		if (numcolors != hair_color_count) {
			LuaError(l, "You should use the same quantity of colors for all hair colors");
		}
		HairColorsRGB[(i / 2) + 1].clear();
		HairColorsRGB[(i / 2) + 1].resize(hair_color_count);
		for (int j = 0; j < numcolors; ++j) {
			lua_rawgeti(l, -1, j + 1);
			HairColorsRGB[(i / 2) + 1][j].Parse(l);
			lua_pop(l, 1);
		}
	}

	return 0;
}
//Wyrmgus end

// ----------------------------------------------------------------------------

/**
**  Get player data.
**
**  @param l  Lua state.
*/
static int CclGetPlayerData(lua_State *l)
{
	if (lua_gettop(l) < 2) {
		LuaError(l, "incorrect argument");
	}
	lua_pushvalue(l, 1);
	const CPlayer *p = CclGetPlayer(l);
	lua_pop(l, 1);
	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Name")) {
		lua_pushstring(l, p->Name.c_str());
		return 1;
	//Wyrmgus start
	} else if (!strcmp(data, "Faction")) {
		if (p->Race != -1 && p->Faction != -1) {
			lua_pushstring(l, PlayerRaces.Factions[p->Race][p->Faction]->Ident.c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	//Wyrmgus end
	} else if (!strcmp(data, "RaceName")) {
		lua_pushstring(l, PlayerRaces.Name[p->Race].c_str());
		return 1;
	//Wyrmgus start
	} else if (!strcmp(data, "Color")) {
		bool found_color = false;
		for (int i = 0; i < PlayerColorMax; ++i) {
			if (PlayerColors[i][0] == p->Color) {
				lua_pushstring(l, PlayerColorNames[i].c_str());
				found_color = true;
				break;
			}		
		}
		if (!found_color) {
			LuaError(l, "Player %d has no color." _C_ p->Index);
		}
		return 1;
	//Wyrmgus end
	} else if (!strcmp(data, "Resources")) {
		LuaCheckArgs(l, 3);

		const std::string res = LuaToString(l, 3);
		const int resId = GetResourceIdByName(l, res.c_str());
		lua_pushnumber(l, p->Resources[resId] + p->StoredResources[resId]);
		return 1;
	} else if (!strcmp(data, "StoredResources")) {
		LuaCheckArgs(l, 3);

		const std::string res = LuaToString(l, 3);
		const int resId = GetResourceIdByName(l, res.c_str());
		lua_pushnumber(l, p->StoredResources[resId]);
		return 1;
	} else if (!strcmp(data, "MaxResources")) {
		LuaCheckArgs(l, 3);

		const std::string res = LuaToString(l, 3);
		const int resId = GetResourceIdByName(l, res.c_str());
		lua_pushnumber(l, p->MaxResources[resId]);
		return 1;
	} else if (!strcmp(data, "UnitTypesCount")) {
		LuaCheckArgs(l, 3);
		CUnitType *type = CclGetUnitType(l);
		Assert(type);
		lua_pushnumber(l, p->UnitTypesCount[type->Slot]);
		return 1;
	} else if (!strcmp(data, "UnitTypesAiActiveCount")) {
		LuaCheckArgs(l, 3);
		CUnitType *type = CclGetUnitType(l);
		Assert(type);
		lua_pushnumber(l, p->UnitTypesAiActiveCount[type->Slot]);
		return 1;
	//Wyrmgus start
	} else if (!strcmp(data, "UnitTypesNonHeroCount")) {
		LuaCheckArgs(l, 3);
		CUnitType *type = CclGetUnitType(l);
		Assert(type);
		lua_pushnumber(l, p->UnitTypesNonHeroCount[type->Slot]);
		return 1;
	} else if (!strcmp(data, "UnitTypesStartingNonHeroCount")) {
		LuaCheckArgs(l, 3);
		CUnitType *type = CclGetUnitType(l);
		Assert(type);
		lua_pushnumber(l, p->UnitTypesStartingNonHeroCount[type->Slot]);
		return 1;
	} else if (!strcmp(data, "Heroes")) {
		lua_createtable(l, p->Heroes.size(), 0);
		for (size_t i = 1; i <= p->Heroes.size(); ++i)
		{
			lua_pushstring(l, p->Heroes[i-1].c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	//Wyrmgus end
	} else if (!strcmp(data, "AiEnabled")) {
		lua_pushboolean(l, p->AiEnabled);
		return 1;
	} else if (!strcmp(data, "TotalNumUnits")) {
		lua_pushnumber(l, p->GetUnitCount());
		return 1;
	//Wyrmgus start
	} else if (!strcmp(data, "TotalNumUnitsConstructed")) {
		lua_pushnumber(l, p->GetUnitCount() - p->NumBuildingsUnderConstruction);
		return 1;
	//Wyrmgus end
	} else if (!strcmp(data, "NumBuildings")) {
		lua_pushnumber(l, p->NumBuildings);
		return 1;
	//Wyrmgus start
	} else if (!strcmp(data, "NumBuildingsUnderConstruction")) {
		lua_pushnumber(l, p->NumBuildingsUnderConstruction);
		return 1;
	//Wyrmgus end
	} else if (!strcmp(data, "Supply")) {
		lua_pushnumber(l, p->Supply);
		return 1;
	} else if (!strcmp(data, "Demand")) {
		lua_pushnumber(l, p->Demand);
		return 1;
	} else if (!strcmp(data, "UnitLimit")) {
		lua_pushnumber(l, p->UnitLimit);
		return 1;
	} else if (!strcmp(data, "BuildingLimit")) {
		lua_pushnumber(l, p->BuildingLimit);
		return 1;
	} else if (!strcmp(data, "TotalUnitLimit")) {
		lua_pushnumber(l, p->TotalUnitLimit);
		return 1;
	} else if (!strcmp(data, "Score")) {
		lua_pushnumber(l, p->Score);
		return 1;
	} else if (!strcmp(data, "TotalUnits")) {
		lua_pushnumber(l, p->TotalUnits);
		return 1;
	} else if (!strcmp(data, "TotalBuildings")) {
		lua_pushnumber(l, p->TotalBuildings);
		return 1;
	} else if (!strcmp(data, "TotalResources")) {
		LuaCheckArgs(l, 3);

		const std::string res = LuaToString(l, 3);
		const int resId = GetResourceIdByName(l, res.c_str());
		lua_pushnumber(l, p->TotalResources[resId]);
		return 1;
	} else if (!strcmp(data, "TotalRazings")) {
		lua_pushnumber(l, p->TotalRazings);
		return 1;
	} else if (!strcmp(data, "TotalKills")) {
		lua_pushnumber(l, p->TotalKills);
		return 1;
	//Wyrmgus start
	} else if (!strcmp(data, "UnitTypeKills")) {
		LuaCheckArgs(l, 3);
		CUnitType *type = CclGetUnitType(l);
		Assert(type);
		lua_pushnumber(l, p->UnitTypeKills[type->Slot]);
		return 1;
	//Wyrmgus end
	} else if (!strcmp(data, "SpeedResourcesHarvest")) {
		LuaCheckArgs(l, 3);

		const std::string res = LuaToString(l, 3);
		const int resId = GetResourceIdByName(l, res.c_str());
		lua_pushnumber(l, p->SpeedResourcesHarvest[resId]);
		return 1;
	} else if (!strcmp(data, "SpeedResourcesReturn")) {
		LuaCheckArgs(l, 3);

		const std::string res = LuaToString(l, 3);
		const int resId = GetResourceIdByName(l, res.c_str());
		lua_pushnumber(l, p->SpeedResourcesReturn[resId]);
		return 1;
	} else if (!strcmp(data, "SpeedBuild")) {
		lua_pushnumber(l, p->SpeedBuild);
		return 1;
	} else if (!strcmp(data, "SpeedTrain")) {
		lua_pushnumber(l, p->SpeedTrain);
		return 1;
	} else if (!strcmp(data, "SpeedUpgrade")) {
		lua_pushnumber(l, p->SpeedUpgrade);
		return 1;
	} else if (!strcmp(data, "SpeedResearch")) {
		lua_pushnumber(l, p->SpeedResearch);
		return 1;
	} else if (!strcmp(data, "Allow")) {
		LuaCheckArgs(l, 3);
		const char *ident = LuaToString(l, 3);
		if (!strncmp(ident, "unit-", 5)) {
			int id = UnitTypeIdByIdent(ident);
			if (UnitIdAllowed(Players[p->Index], id) > 0) {
				lua_pushstring(l, "A");
			} else if (UnitIdAllowed(Players[p->Index], id) == 0) {
				lua_pushstring(l, "F");
			}
		} else if (!strncmp(ident, "upgrade-", 8)) {
			if (UpgradeIdentAllowed(Players[p->Index], ident) == 'A') {
				lua_pushstring(l, "A");
			} else if (UpgradeIdentAllowed(Players[p->Index], ident) == 'R') {
				lua_pushstring(l, "R");
			} else if (UpgradeIdentAllowed(Players[p->Index], ident) == 'F') {
				lua_pushstring(l, "F");
			}
		} else {
			DebugPrint(" wrong ident %s\n" _C_ ident);
		}
		return 1;
	//Wyrmgus start
	} else if (!strcmp(data, "HasContactWith")) {
		LuaCheckArgs(l, 3);
		int second_player = LuaToNumber(l, 3);
		lua_pushboolean(l, p->HasContactWith(Players[second_player]));
		return 1;
	} else if (!strcmp(data, "HasQuest")) {
		LuaCheckArgs(l, 3);
		CQuest *quest = GetQuest(LuaToString(l, 3));
		if (std::find(p->CurrentQuests.begin(), p->CurrentQuests.end(), quest) != p->CurrentQuests.end()) {
			lua_pushboolean(l, true);
		} else {
			lua_pushboolean(l, false);
		}
		return 1;
	} else if (!strcmp(data, "CompletedQuest")) {
		LuaCheckArgs(l, 3);
		CQuest *quest = GetQuest(LuaToString(l, 3));
		if (std::find(p->CompletedQuests.begin(), p->CompletedQuests.end(), quest) != p->CompletedQuests.end()) {
			lua_pushboolean(l, true);
		} else {
			lua_pushboolean(l, false);
		}
		return 1;
	//Wyrmgus end
	} else {
		LuaError(l, "Invalid field: %s" _C_ data);
	}

	return 0;
}

/**
**  Set player data.
**
**  @param l  Lua state.
*/
static int CclSetPlayerData(lua_State *l)
{
	if (lua_gettop(l) < 3) {
		LuaError(l, "incorrect argument");
	}
	lua_pushvalue(l, 1);
	CPlayer *p = CclGetPlayer(l);
	lua_pop(l, 1);
	const char *data = LuaToString(l, 2);
	
	//Wyrmgus start
	//if player is unused, return
	if (p->Type == PlayerNobody && Editor.Running == EditorNotRunning) {
		return 0;
	}
	//Wyrmgus end

	if (!strcmp(data, "Name")) {
		p->SetName(LuaToString(l, 3));
	} else if (!strcmp(data, "RaceName")) {
		if (GameRunning) {
			p->SetFaction("");
		}

		const char *racename = LuaToString(l, 3);
		//Wyrmgus start
//		p->Race = PlayerRaces.GetRaceIndexByName(racename);
		p->SetCivilization(PlayerRaces.GetRaceIndexByName(racename));
		//Wyrmgus end
		
		if (p->Race == -1) {
			LuaError(l, "invalid race name '%s'" _C_ racename);
		}
	//Wyrmgus start
	} else if (!strcmp(data, "Faction")) {
		std::string faction_name = LuaToString(l, 3);
		if (faction_name == "random") {
			p->SetRandomFaction();
		} else {
			p->SetFaction(faction_name);
		}
	//Wyrmgus end
	} else if (!strcmp(data, "Resources")) {
		LuaCheckArgs(l, 4);

		const std::string res = LuaToString(l, 3);
		const int resId = GetResourceIdByName(l, res.c_str());
		p->SetResource(resId, LuaToNumber(l, 4));
	} else if (!strcmp(data, "StoredResources")) {
		LuaCheckArgs(l, 4);

		const std::string res = LuaToString(l, 3);
		const int resId = GetResourceIdByName(l, res.c_str());
		p->SetResource(resId, LuaToNumber(l, 4), STORE_BUILDING);
		// } else if (!strcmp(data, "UnitTypesCount")) {
		// } else if (!strcmp(data, "AiEnabled")) {
		// } else if (!strcmp(data, "TotalNumUnits")) {
		// } else if (!strcmp(data, "NumBuildings")) {
		// } else if (!strcmp(data, "Supply")) {
		// } else if (!strcmp(data, "Demand")) {
	} else if (!strcmp(data, "UnitLimit")) {
		p->UnitLimit = LuaToNumber(l, 3);
	} else if (!strcmp(data, "BuildingLimit")) {
		p->BuildingLimit = LuaToNumber(l, 3);
	} else if (!strcmp(data, "TotalUnitLimit")) {
		p->TotalUnitLimit = LuaToNumber(l, 3);
	} else if (!strcmp(data, "Score")) {
		p->Score = LuaToNumber(l, 3);
	} else if (!strcmp(data, "TotalUnits")) {
		p->TotalUnits = LuaToNumber(l, 3);
	} else if (!strcmp(data, "TotalBuildings")) {
		p->TotalBuildings = LuaToNumber(l, 3);
	} else if (!strcmp(data, "TotalResources")) {
		LuaCheckArgs(l, 4);

		const std::string res = LuaToString(l, 3);
		const int resId = GetResourceIdByName(l, res.c_str());
		p->TotalResources[resId] = LuaToNumber(l, 4);
	} else if (!strcmp(data, "TotalRazings")) {
		p->TotalRazings = LuaToNumber(l, 3);
	} else if (!strcmp(data, "TotalKills")) {
		p->TotalKills = LuaToNumber(l, 3);
	} else if (!strcmp(data, "SpeedResourcesHarvest")) {
		LuaCheckArgs(l, 4);

		const std::string res = LuaToString(l, 3);
		const int resId = GetResourceIdByName(l, res.c_str());
		p->SpeedResourcesHarvest[resId] = LuaToNumber(l, 4);
	} else if (!strcmp(data, "SpeedResourcesReturn")) {
		LuaCheckArgs(l, 4);

		const std::string res = LuaToString(l, 3);
		const int resId = GetResourceIdByName(l, res.c_str());
		p->SpeedResourcesReturn[resId] = LuaToNumber(l, 4);
	} else if (!strcmp(data, "SpeedBuild")) {
		p->SpeedBuild = LuaToNumber(l, 3);
	} else if (!strcmp(data, "SpeedTrain")) {
		p->SpeedTrain = LuaToNumber(l, 3);
	} else if (!strcmp(data, "SpeedUpgrade")) {
		p->SpeedUpgrade = LuaToNumber(l, 3);
	} else if (!strcmp(data, "SpeedResearch")) {
		p->SpeedResearch = LuaToNumber(l, 3);
	} else if (!strcmp(data, "Allow")) {
		LuaCheckArgs(l, 4);
		const char *ident = LuaToString(l, 3);
		const std::string acquire = LuaToString(l, 4);

		if (!strncmp(ident, "upgrade-", 8)) {
			if (acquire == "R" && UpgradeIdentAllowed(*p, ident) != 'R') {
				UpgradeAcquire(*p, CUpgrade::Get(ident));
			} else if (acquire == "F" || acquire == "A") {
				if (UpgradeIdentAllowed(*p, ident) == 'R') {
					UpgradeLost(*p, CUpgrade::Get(ident)->ID);
				}
				AllowUpgradeId(*p, UpgradeIdByIdent(ident), acquire[0]);
			}
		//Wyrmgus start
		} else if (!strncmp(ident, "unit-", 5)) {
			const int UnitMax = 65536; /// How many units supported
			int id = UnitTypeIdByIdent(ident);
			if (acquire == "A" || acquire == "R") {
				AllowUnitId(*p, id, UnitMax);
			} else if (acquire == "F") {
				AllowUnitId(*p, id, 0);
			}
		//Wyrmgus end
		} else {
			LuaError(l, " wrong ident %s\n" _C_ ident);
		}
	//Wyrmgus start
	} else if (!strcmp(data, "AcceptQuest")) {
		CQuest *quest = GetQuest(LuaToString(l, 3));
		if (quest) {
			p->AcceptQuest(quest);
		}
	//Wyrmgus end
	} else {
		LuaError(l, "Invalid field: %s" _C_ data);
	}

	return 0;
}

/**
**  Set ai player algo.
**
**  @param l  Lua state.
*/
static int CclSetAiType(lua_State *l)
{
	CPlayer *p;

	if (lua_gettop(l) < 2) {
		LuaError(l, "incorrect argument");
	}
	lua_pushvalue(l, 1);
	p = CclGetPlayer(l);
	lua_pop(l, 1);

	p->AiName = LuaToString(l, 2);

	return 0;
}

//Wyrmgus start
static int CclGetLanguages(lua_State *l)
{
	bool only_used = false;
	if (lua_gettop(l) >= 1) {
		only_used = LuaToBoolean(l, 1);
	}
	
	std::vector<std::string> languages;
	for (size_t i = 0; i != PlayerRaces.Languages.size(); ++i) {
		if (!only_used || PlayerRaces.Languages[i]->UsedByCivilizationOrFaction) {
			languages.push_back(PlayerRaces.Languages[i]->Ident);
		}
	}
		
	lua_createtable(l, languages.size(), 0);
	for (size_t i = 1; i <= languages.size(); ++i)
	{
		lua_pushstring(l, languages[i-1].c_str());
		lua_rawseti(l, -2, i);
	}
	return 1;
}

/**
**  Get language data.
**
**  @param l  Lua state.
*/
static int CclGetLanguageData(lua_State *l)
{
	if (lua_gettop(l) < 2) {
		LuaError(l, "incorrect argument");
	}
	std::string language_name = LuaToString(l, 1);
	int language_id = PlayerRaces.GetLanguageIndexByIdent(language_name);
	if (language_id == -1) {
		LuaError(l, "Language \"%s\" doesn't exist." _C_ language_name.c_str());
	}
	const CLanguage *language = PlayerRaces.Languages[language_id];
	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Name")) {
		lua_pushstring(l, language->Name.c_str());
		return 1;
	} else if (!strcmp(data, "Family")) {
		lua_pushstring(l, language->Family.c_str());
		return 1;
	} else if (!strcmp(data, "Words")) {
		lua_createtable(l, language->LanguageWords.size(), 0);
		for (size_t i = 1; i <= language->LanguageWords.size(); ++i)
		{
			lua_pushstring(l, language->LanguageWords[i-1]->Word.c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "ModWords")) {
		if (lua_gettop(l) < 3) {
			LuaError(l, "incorrect argument");
		}
		
		std::string mod_file = LuaToString(l, 3);
		
		std::vector<std::string> mod_words;
		for (size_t i = 0; i < language->ModWords.size(); ++i)
		{
			if (language->ModWords[i]->Mod == mod_file) {
				mod_words.push_back(language->ModWords[i]->Word);
			}
		}
		
		lua_createtable(l, mod_words.size(), 0);
		for (size_t i = 1; i <= mod_words.size(); ++i)
		{
			lua_pushstring(l, mod_words[i-1].c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else {
		LuaError(l, "Invalid field: %s" _C_ data);
	}

	return 0;
}

/**
**  Get language word data.
**
**  @param l  Lua state.
*/
static int CclGetLanguageWordData(lua_State *l)
{
	if (lua_gettop(l) < 3) {
		LuaError(l, "incorrect argument");
	}
	std::string language_name = LuaToString(l, 1);
	int language_id = PlayerRaces.GetLanguageIndexByIdent(language_name);
	if (language_id == -1) {
		LuaError(l, "Language \"%s\" doesn't exist." _C_ language_name.c_str());
	}
	
	std::string word_name = LuaToString(l, 2);
	std::vector<std::string> word_meanings;
	LanguageWord *word = PlayerRaces.Languages[language_id]->GetWord(word_name, -1, word_meanings);
	if (word == NULL) {
		LuaError(l, "Word \"%s\" doesn't exist for the \"%s\" language." _C_ word_name.c_str() _C_ language_name.c_str());
	}
	
	const char *data = LuaToString(l, 3);

	if (!strcmp(data, "Type")) {
		if (word->Type != -1) {
			lua_pushstring(l, GetWordTypeNameById(word->Type).c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Meaning")) {
		for (size_t i = 0; i < word->Meanings.size(); ++i) {
			lua_pushstring(l, word->Meanings[i].c_str());
			return 1;
		}
		lua_pushstring(l, "");
		return 1;
	} else if (!strcmp(data, "Gender")) {
		if (word->Gender != -1) {
			lua_pushstring(l, GetGrammaticalGenderNameById(word->Gender).c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "NameTypes")) {
		std::vector<std::string> name_types;
		
		for (int i = 0; i < MaxGrammaticalNumbers; ++i) {
			for (int j = 0; j < MaxGrammaticalCases; ++j) {
				for (int k = 0; k < MaxGrammaticalTenses; ++k) {
					for (std::map<std::string, int>::iterator iterator = word->NameTypes[i][j][k].begin(); iterator != word->NameTypes[i][j][k].end(); ++iterator) {
						if (iterator->second > 0) {
							if (std::find(name_types.begin(), name_types.end(), iterator->first) == name_types.end()) {
								name_types.push_back(iterator->first);
							}
						}
					}
				}
			}
		}
		
		lua_createtable(l, name_types.size(), 0);
		for (size_t i = 1; i <= name_types.size(); ++i)
		{
			lua_pushstring(l, name_types[i-1].c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "AffixNameTypes")) {
		if (lua_gettop(l) < 5) {
			LuaError(l, "incorrect argument");
		}
		
		std::string word_junction_type_name = LuaToString(l, 4);
		int word_junction_type = GetWordJunctionTypeIdByName(word_junction_type_name);
		if (word_junction_type == -1) {
			LuaError(l, "Word junction type \"%s\" doesn't exist." _C_ word_junction_type_name.c_str());
		}
				
		std::string affix_type_name = LuaToString(l, 5);
		int affix_type = GetAffixTypeIdByName(affix_type_name);
		if (affix_type == -1) {
			LuaError(l, "Affix type \"%s\" doesn't exist." _C_ affix_type_name.c_str());
		}
		
		std::vector<std::string> name_types;
		
		for (int i = 0; i < MaxGrammaticalNumbers; ++i) {
			for (int j = 0; j < MaxGrammaticalCases; ++j) {
				for (int k = 0; k < MaxGrammaticalTenses; ++k) {
					for (std::map<std::string, int>::iterator iterator = word->AffixNameTypes[word_junction_type][affix_type][i][j][k].begin(); iterator != word->AffixNameTypes[word_junction_type][affix_type][i][j][k].end(); ++iterator) {
						if (iterator->second > 0) {
							if (std::find(name_types.begin(), name_types.end(), iterator->first) == name_types.end()) {
								name_types.push_back(iterator->first);
							}
						}
					}
				}
			}
		}
		
		lua_createtable(l, name_types.size(), 0);
		for (size_t i = 1; i <= name_types.size(); ++i)
		{
			lua_pushstring(l, name_types[i-1].c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else {
		LuaError(l, "Invalid field: %s" _C_ data);
	}

	return 0;
}

static int CclGetReligions(lua_State *l)
{
	lua_createtable(l, PlayerRaces.Religions.size(), 0);
	for (size_t i = 1; i <= PlayerRaces.Religions.size(); ++i)
	{
		lua_pushstring(l, PlayerRaces.Religions[i-1]->Ident.c_str());
		lua_rawseti(l, -2, i);
	}
	return 1;
}

static int CclGetDeityDomains(lua_State *l)
{
	lua_createtable(l, PlayerRaces.DeityDomains.size(), 0);
	for (size_t i = 1; i <= PlayerRaces.DeityDomains.size(); ++i)
	{
		lua_pushstring(l, PlayerRaces.DeityDomains[i-1]->Ident.c_str());
		lua_rawseti(l, -2, i);
	}
	return 1;
}

static int CclGetDeities(lua_State *l)
{
	lua_createtable(l, PlayerRaces.Deities.size(), 0);
	for (size_t i = 1; i <= PlayerRaces.Deities.size(); ++i)
	{
		lua_pushstring(l, PlayerRaces.Deities[i-1]->Ident.c_str());
		lua_rawseti(l, -2, i);
	}
	return 1;
}

/**
**  Get religion data.
**
**  @param l  Lua state.
*/
static int CclGetReligionData(lua_State *l)
{
	if (lua_gettop(l) < 2) {
		LuaError(l, "incorrect argument");
	}
	std::string religion_ident = LuaToString(l, 1);
	int religion_id = PlayerRaces.GetReligionIndexByIdent(religion_ident);
	if (religion_id == -1) {
		LuaError(l, "Religion \"%s\" doesn't exist." _C_ religion_ident.c_str());
	}
	const CReligion *religion = PlayerRaces.Religions[religion_id];
	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Name")) {
		lua_pushstring(l, religion->Name.c_str());
		return 1;
	} else if (!strcmp(data, "Description")) {
		lua_pushstring(l, religion->Description.c_str());
		return 1;
	} else if (!strcmp(data, "Background")) {
		lua_pushstring(l, religion->Background.c_str());
		return 1;
	} else if (!strcmp(data, "Quote")) {
		lua_pushstring(l, religion->Quote.c_str());
		return 1;
	} else if (!strcmp(data, "CulturalDeities")) {
		lua_pushboolean(l, religion->CulturalDeities);
		return 1;
	} else {
		LuaError(l, "Invalid field: %s" _C_ data);
	}

	return 0;
}

/**
**  Get deity domain data.
**
**  @param l  Lua state.
*/
static int CclGetDeityDomainData(lua_State *l)
{
	if (lua_gettop(l) < 2) {
		LuaError(l, "incorrect argument");
	}
	std::string deity_domain_ident = LuaToString(l, 1);
	int deity_domain_id = PlayerRaces.GetDeityDomainIndexByIdent(deity_domain_ident);
	if (deity_domain_id == -1) {
		LuaError(l, "Deity domain \"%s\" doesn't exist." _C_ deity_domain_ident.c_str());
	}
	const CDeityDomain *deity_domain = PlayerRaces.DeityDomains[deity_domain_id];
	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Name")) {
		lua_pushstring(l, deity_domain->Name.c_str());
		return 1;
	} else if (!strcmp(data, "Abilities")) {
		lua_createtable(l, deity_domain->Abilities.size(), 0);
		for (size_t i = 1; i <= deity_domain->Abilities.size(); ++i)
		{
			lua_pushstring(l, deity_domain->Abilities[i-1]->Ident.c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else {
		LuaError(l, "Invalid field: %s" _C_ data);
	}

	return 0;
}

/**
**  Get deity data.
**
**  @param l  Lua state.
*/
static int CclGetDeityData(lua_State *l)
{
	if (lua_gettop(l) < 2) {
		LuaError(l, "incorrect argument");
	}
	std::string deity_ident = LuaToString(l, 1);
	int deity_id = PlayerRaces.GetDeityIndexByIdent(deity_ident);
	if (deity_id == -1) {
		LuaError(l, "Deity \"%s\" doesn't exist." _C_ deity_ident.c_str());
	}
	const CDeity *deity = PlayerRaces.Deities[deity_id];
	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Name")) {
		lua_pushstring(l, deity->Name.c_str());
		return 1;
	} else if (!strcmp(data, "Pantheon")) {
		lua_pushstring(l, deity->Pantheon.c_str());
		return 1;
	} else if (!strcmp(data, "Description")) {
		lua_pushstring(l, deity->Description.c_str());
		return 1;
	} else if (!strcmp(data, "Background")) {
		lua_pushstring(l, deity->Background.c_str());
		return 1;
	} else if (!strcmp(data, "Quote")) {
		lua_pushstring(l, deity->Quote.c_str());
		return 1;
	} else if (!strcmp(data, "Major")) {
		lua_pushboolean(l, deity->Major);
		return 1;
	} else if (!strcmp(data, "HomePlane")) {
		if (deity->HomePlane) {
			lua_pushstring(l, deity->HomePlane->Name.c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Civilizations")) {
		lua_createtable(l, deity->Civilizations.size(), 0);
		for (size_t i = 1; i <= deity->Civilizations.size(); ++i)
		{
			lua_pushstring(l, PlayerRaces.Name[deity->Civilizations[i-1]].c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "Religions")) {
		lua_createtable(l, deity->Religions.size(), 0);
		for (size_t i = 1; i <= deity->Religions.size(); ++i)
		{
			lua_pushstring(l, deity->Religions[i-1]->Ident.c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "Domains")) {
		lua_createtable(l, deity->Domains.size(), 0);
		for (size_t i = 1; i <= deity->Domains.size(); ++i)
		{
			lua_pushstring(l, deity->Domains[i-1]->Ident.c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "Abilities")) {
		lua_createtable(l, deity->Abilities.size(), 0);
		for (size_t i = 1; i <= deity->Abilities.size(); ++i)
		{
			lua_pushstring(l, deity->Abilities[i-1]->Ident.c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else if (!strcmp(data, "CulturalName")) {
		if (lua_gettop(l) < 3) {
			LuaError(l, "incorrect argument");
		}
		
		int civilization = PlayerRaces.GetRaceIndexByName(LuaToString(l, 3));
		if (civilization == -1) {
			LuaError(l, "Civilization doesn't exist.");
		}
		
		if (deity->CulturalNames.find(civilization) != deity->CulturalNames.end()) {
			lua_pushstring(l, deity->CulturalNames.find(civilization)->second.c_str());
		} else {
			lua_pushstring(l, "");
		}
		
		return 1;
	} else {
		LuaError(l, "Invalid field: %s" _C_ data);
	}

	return 0;
}
//Wyrmgus end

// ----------------------------------------------------------------------------

/**
**  Register CCL features for players.
*/
void PlayerCclRegister()
{
	lua_register(Lua, "Player", CclPlayer);
	lua_register(Lua, "ChangeUnitsOwner", CclChangeUnitsOwner);
	lua_register(Lua, "GetThisPlayer", CclGetThisPlayer);
	lua_register(Lua, "SetThisPlayer", CclSetThisPlayer);

	lua_register(Lua, "SetMaxSelectable", CclSetMaxSelectable);

	lua_register(Lua, "SetAllPlayersUnitLimit", CclSetAllPlayersUnitLimit);
	lua_register(Lua, "SetAllPlayersBuildingLimit", CclSetAllPlayersBuildingLimit);
	lua_register(Lua, "SetAllPlayersTotalUnitLimit", CclSetAllPlayersTotalUnitLimit);

	lua_register(Lua, "SetDiplomacy", CclSetDiplomacy);
	lua_register(Lua, "Diplomacy", CclDiplomacy);
	lua_register(Lua, "SetSharedVision", CclSetSharedVision);
	lua_register(Lua, "SharedVision", CclSharedVision);

	lua_register(Lua, "DefineRaceNames", CclDefineRaceNames);
	//Wyrmgus start
	lua_register(Lua, "DefineCivilization", CclDefineCivilization);
	lua_register(Lua, "DefineLanguageWord", CclDefineLanguageWord);
	lua_register(Lua, "GetCivilizationData", CclGetCivilizationData);
	lua_register(Lua, "GetCivilizationClassUnitType", CclGetCivilizationClassUnitType);
	lua_register(Lua, "GetFactionClassUnitType", CclGetFactionClassUnitType);
	lua_register(Lua, "DefineFaction", CclDefineFaction);
	lua_register(Lua, "DefineReligion", CclDefineReligion);
	lua_register(Lua, "DefineDeityDomain", CclDefineDeityDomain);
	lua_register(Lua, "DefineDeity", CclDefineDeity);
	lua_register(Lua, "DefineLanguage", CclDefineLanguage);
	lua_register(Lua, "GetCivilizations", CclGetCivilizations);
	lua_register(Lua, "GetCivilizationFactionNames", CclGetCivilizationFactionNames);
	lua_register(Lua, "GetFactions", CclGetFactions);
	lua_register(Lua, "GetPlayerColors", CclGetPlayerColors);
	lua_register(Lua, "GetFactionData", CclGetFactionData);
	lua_register(Lua, "GetFactionDevelopsTo", CclGetFactionDevelopsTo);
	//Wyrmgus end
	lua_register(Lua, "DefinePlayerColors", CclDefinePlayerColors);
	lua_register(Lua, "DefinePlayerColorIndex", CclDefinePlayerColorIndex);

	lua_register(Lua, "NewColors", CclNewPlayerColors);

	//Wyrmgus start
	lua_register(Lua, "DefineConversiblePlayerColors", CclDefineConversiblePlayerColors);
	
	lua_register(Lua, "DefineSkinColors", CclDefineSkinColors);
	lua_register(Lua, "DefineHairColors", CclDefineHairColors);
	//Wyrmgus end
	
	// player member access functions
	lua_register(Lua, "GetPlayerData", CclGetPlayerData);
	lua_register(Lua, "SetPlayerData", CclSetPlayerData);
	lua_register(Lua, "SetAiType", CclSetAiType);
	//Wyrmgus start
	lua_register(Lua, "GetLanguages", CclGetLanguages);
	lua_register(Lua, "GetLanguageData", CclGetLanguageData);
	lua_register(Lua, "GetLanguageWordData", CclGetLanguageWordData);
	
	lua_register(Lua, "GetReligions", CclGetReligions);
	lua_register(Lua, "GetDeityDomains", CclGetDeityDomains);
	lua_register(Lua, "GetDeities", CclGetDeities);
	lua_register(Lua, "GetReligionData", CclGetReligionData);
	lua_register(Lua, "GetDeityDomainData", CclGetDeityDomainData);
	lua_register(Lua, "GetDeityData", CclGetDeityData);
	//Wyrmgus end
}

//@}
