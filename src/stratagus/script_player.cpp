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
#include "commands.h"
//Wyrmgus start
#include "editor.h"
#include "font.h"
#include "grand_strategy.h"
#include "item.h"
//Wyrmgus end
#include "map.h"
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
			if (subargs != MaxCosts) {
				LuaError(l, "Wrong number of total-resources: %d" _C_ subargs);
			}
			for (int k = 0; k < subargs; ++k) {
				this->TotalResources[k] = LuaToNumber(l, j + 1, k + 1);
			}
		} else if (!strcmp(value, "speed-resource-harvest")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			if (subargs != MaxCosts) {
				LuaError(l, "Wrong number of speed-resource-harvest: %d" _C_ subargs);
			}
			for (int k = 0; k < subargs; ++k) {
				this->SpeedResourcesHarvest[k] = LuaToNumber(l, j + 1, k + 1);
			}
		} else if (!strcmp(value, "speed-resource-return")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			if (subargs != MaxCosts) {
				LuaError(l, "Wrong number of speed-resource-harvest: %d" _C_ subargs);
			}
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

	Select(pos1, pos2, table, HasSamePlayerAs(Players[oldp]));
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

	//Wyrmgus start
	ThisPlayer = &Players[PlayerNumNeutral]; //ugly hack to make sure the music is stopped
	//load proper UI
	char buf[256];
	snprintf(buf, sizeof(buf), "if (LoadCivilizationUI ~= nil) then LoadCivilizationUI(\"%s\") end;", PlayerRaces.Name[Players[plynr].Race].c_str());
	CclCommand(buf);
	//Wyrmgus end
	
	ThisPlayer = &Players[plynr];

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
				//Wyrmgus start
				} else if (!strcmp(value, "playable")) {
					++k;
					PlayerRaces.Playable[i] = LuaToBoolean(l, j + 1, k + 1);
				} else if (!strcmp(value, "species")) {
					++k;
					PlayerRaces.Species[i] = LuaToString(l, j + 1, k + 1);
				} else if (!strcmp(value, "default-color")) {
					++k;
					PlayerRaces.DefaultColor[i] = LuaToString(l, j + 1, k + 1);
				} else if (!strcmp(value, "parent-civilization")) {
					++k;
					PlayerRaces.ParentCivilization[i] = PlayerRaces.GetRaceIndexByName(LuaToString(l, j + 1, k + 1));
				} else if (!strcmp(value, "language")) {
					++k;
					int language = PlayerRaces.GetLanguageIndexByIdent(LuaToString(l, j + 1, k + 1));
					if (language != -1) {
						PlayerRaces.CivilizationLanguage[i] = language;
					} else {
						LuaError(l, "Language not found.");
					}
				} else if (!strcmp(value, "personal-names")) {
					++k;
					lua_rawgeti(l, j + 1, k + 1);
					if (!lua_istable(l, -1)) {
						LuaError(l, "incorrect argument (expected table)");
					}
					int subsubargs = lua_rawlen(l, -1);
					for (int n = 0; n < subsubargs; ++n) {
						PlayerRaces.PersonalNames[i][n] = LuaToString(l, -1, n + 1);
					}
				} else if (!strcmp(value, "personal-name-prefixes")) {
					++k;
					lua_rawgeti(l, j + 1, k + 1);
					if (!lua_istable(l, -1)) {
						LuaError(l, "incorrect argument (expected table)");
					}
					int subsubargs = lua_rawlen(l, -1);
					for (int n = 0; n < subsubargs; ++n) {
						PlayerRaces.PersonalNamePrefixes[i][n] = LuaToString(l, -1, n + 1);
					}
				} else if (!strcmp(value, "personal-name-suffixes")) {
					++k;
					lua_rawgeti(l, j + 1, k + 1);
					if (!lua_istable(l, -1)) {
						LuaError(l, "incorrect argument (expected table)");
					}
					int subsubargs = lua_rawlen(l, -1);
					for (int n = 0; n < subsubargs; ++n) {
						PlayerRaces.PersonalNameSuffixes[i][n] = LuaToString(l, -1, n + 1);
					}
				} else if (!strcmp(value, "province-names")) {
					++k;
					lua_rawgeti(l, j + 1, k + 1);
					if (!lua_istable(l, -1)) {
						LuaError(l, "incorrect argument (expected table)");
					}
					int subsubargs = lua_rawlen(l, -1);
					for (int n = 0; n < subsubargs; ++n) {
						PlayerRaces.ProvinceNames[i][n] = LuaToString(l, -1, n + 1);
					}
				} else if (!strcmp(value, "province-name-prefixes")) {
					++k;
					lua_rawgeti(l, j + 1, k + 1);
					if (!lua_istable(l, -1)) {
						LuaError(l, "incorrect argument (expected table)");
					}
					int subsubargs = lua_rawlen(l, -1);
					for (int n = 0; n < subsubargs; ++n) {
						PlayerRaces.ProvinceNamePrefixes[i][n] = LuaToString(l, -1, n + 1);
					}
				} else if (!strcmp(value, "province-name-suffixes")) {
					++k;
					lua_rawgeti(l, j + 1, k + 1);
					if (!lua_istable(l, -1)) {
						LuaError(l, "incorrect argument (expected table)");
					}
					int subsubargs = lua_rawlen(l, -1);
					for (int n = 0; n < subsubargs; ++n) {
						PlayerRaces.ProvinceNameSuffixes[i][n] = LuaToString(l, -1, n + 1);
					}
				} else if (!strcmp(value, "settlement-names")) {
					++k;
					lua_rawgeti(l, j + 1, k + 1);
					if (!lua_istable(l, -1)) {
						LuaError(l, "incorrect argument (expected table)");
					}
					int subsubargs = lua_rawlen(l, -1);
					for (int n = 0; n < subsubargs; ++n) {
						PlayerRaces.SettlementNames[i][n] = LuaToString(l, -1, n + 1);
					}
				} else if (!strcmp(value, "settlement-name-prefixes")) {
					++k;
					lua_rawgeti(l, j + 1, k + 1);
					if (!lua_istable(l, -1)) {
						LuaError(l, "incorrect argument (expected table)");
					}
					int subsubargs = lua_rawlen(l, -1);
					for (int n = 0; n < subsubargs; ++n) {
						PlayerRaces.SettlementNamePrefixes[i][n] = LuaToString(l, -1, n + 1);
					}
				} else if (!strcmp(value, "settlement-name-suffixes")) {
					++k;
					lua_rawgeti(l, j + 1, k + 1);
					if (!lua_istable(l, -1)) {
						LuaError(l, "incorrect argument (expected table)");
					}
					int subsubargs = lua_rawlen(l, -1);
					for (int n = 0; n < subsubargs; ++n) {
						PlayerRaces.SettlementNameSuffixes[i][n] = LuaToString(l, -1, n + 1);
					}
				//Wyrmgus end
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
	int civilization;
	if (PlayerRaces.GetRaceIndexByName(civilization_name.c_str()) != -1) { // redefinition
		civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	} else {
		civilization = PlayerRaces.Count++;
		PlayerRaces.Name[civilization] = civilization_name;
		PlayerRaces.Playable[civilization] = true; //civilizations are playable by default
		SetCivilizationStringToIndex(PlayerRaces.Name[civilization], civilization);
	}	
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		if (!strcmp(value, "Display")) {
			PlayerRaces.Display[civilization] = LuaToString(l, -1);
		} else if (!strcmp(value, "Visible")) {
			PlayerRaces.Visible[civilization] = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Playable")) {
			PlayerRaces.Playable[civilization] = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Species")) {
			PlayerRaces.Species[civilization] = LuaToString(l, -1);
		} else if (!strcmp(value, "ParentCivilization")) {
			PlayerRaces.ParentCivilization[civilization] = PlayerRaces.GetRaceIndexByName(LuaToString(l, -1));
		} else if (!strcmp(value, "Language")) {
			int language = PlayerRaces.GetLanguageIndexByIdent(LuaToString(l, -1));
			if (language != -1) {
				PlayerRaces.CivilizationLanguage[civilization] = language;
			} else {
				LuaError(l, "Language not found.");
			}
		} else if (!strcmp(value, "DefaultColor")) {
			PlayerRaces.DefaultColor[civilization] = LuaToString(l, -1);
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	return 0;
}

/**
**  Define a civilization's language
**
**  @param l  Lua state.
*/
static int CclDefineCivilizationLanguage(lua_State *l)
{
	int args = lua_gettop(l);
	int language = PlayerRaces.GetLanguageIndexByIdent(LuaToString(l, 1));
	
	if (language == -1) { //if the language is invalid, don't define its words
		return 0;
	}
	
	for (int j = 1; j < args; ++j) {
		const char *value = LuaToString(l, j + 1);
		if (!strcmp(value, "pronouns")) {
			++j;
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				LanguagePronoun *pronoun = new LanguagePronoun;
				pronoun->Word = LuaToString(l, j + 1, k + 1);
				PlayerRaces.Languages[language]->LanguagePronouns.push_back(pronoun);
				++k;
				lua_rawgeti(l, j + 1, k + 1);
				if (!lua_istable(l, -1)) {
					LuaError(l, "incorrect argument (expected table)");
				}
				int subsubargs = lua_rawlen(l, -1);
				for (int n = 0; n < subsubargs; ++n) {
					const char *value = LuaToString(l, -1, n + 1);
					if (!strcmp(value, "meaning")) {
						++n;
						pronoun->Meanings.push_back(LuaToString(l, -1, n + 1));
					} else if (!strcmp(value, "nominative")) {
						++n;
						pronoun->Nominative = LuaToString(l, -1, n + 1);
					} else if (!strcmp(value, "accusative")) {
						++n;
						pronoun->Accusative = LuaToString(l, -1, n + 1);
					} else if (!strcmp(value, "dative")) {
						++n;
						pronoun->Dative = LuaToString(l, -1, n + 1);
					} else if (!strcmp(value, "genitive")) {
						++n;
						pronoun->Genitive = LuaToString(l, -1, n + 1);
					} else {
						LuaError(l, "Unsupported tag: %s" _C_ value);
					}
				}
			}
		} else if (!strcmp(value, "adverbs")) {
			++j;
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				LanguageAdverb *adverb = new LanguageAdverb;
				adverb->Word = LuaToString(l, j + 1, k + 1);
				PlayerRaces.Languages[language]->LanguageAdverbs.push_back(adverb);
				++k;
				lua_rawgeti(l, j + 1, k + 1);
				if (!lua_istable(l, -1)) {
					LuaError(l, "incorrect argument (expected table)");
				}
				int subsubargs = lua_rawlen(l, -1);
				for (int n = 0; n < subsubargs; ++n) {
					const char *value = LuaToString(l, -1, n + 1);
					if (!strcmp(value, "meaning")) {
						++n;
						adverb->Meanings.push_back(LuaToString(l, -1, n + 1));
					} else if (!strcmp(value, "adjective")) {
						++n;
						adverb->Adjective = LuaToString(l, -1, n + 1);
					} else {
						LuaError(l, "Unsupported tag: %s" _C_ value);
					}
				}
			}
		} else if (!strcmp(value, "conjunctions")) {
			++j;
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				LanguageConjunction *conjunction = new LanguageConjunction;
				conjunction->Word = LuaToString(l, j + 1, k + 1);
				PlayerRaces.Languages[language]->LanguageConjunctions.push_back(conjunction);
				++k;
				lua_rawgeti(l, j + 1, k + 1);
				if (!lua_istable(l, -1)) {
					LuaError(l, "incorrect argument (expected table)");
				}
				int subsubargs = lua_rawlen(l, -1);
				for (int n = 0; n < subsubargs; ++n) {
					const char *value = LuaToString(l, -1, n + 1);
					if (!strcmp(value, "meaning")) {
						++n;
						conjunction->Meanings.push_back(LuaToString(l, -1, n + 1));
					} else {
						LuaError(l, "Unsupported tag: %s" _C_ value);
					}
				}
			}
		} else if (!strcmp(value, "numerals")) {
			++j;
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				LanguageNumeral *numeral = new LanguageNumeral;
				numeral->Word = LuaToString(l, j + 1, k + 1);
				PlayerRaces.Languages[language]->LanguageNumerals.push_back(numeral);
				++k;
				lua_rawgeti(l, j + 1, k + 1);
				if (!lua_istable(l, -1)) {
					LuaError(l, "incorrect argument (expected table)");
				}
				int subsubargs = lua_rawlen(l, -1);
				for (int n = 0; n < subsubargs; ++n) {
					const char *value = LuaToString(l, -1, n + 1);
					if (!strcmp(value, "number")) {
						++n;
						numeral->Number = LuaToNumber(l, -1, n + 1);
					} else {
						LuaError(l, "Unsupported tag: %s" _C_ value);
					}
				}
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}

	return 0;
}

/**
**  Define a noun for a particular civilization's language.
**
**  @param l  Lua state.
*/
static int CclDefineLanguageNoun(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	LanguageNoun *noun = new LanguageNoun;
	noun->Word = LuaToString(l, 1);
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Language")) {
			int language = PlayerRaces.GetLanguageIndexByIdent(LuaToString(l, -1));
			
			if (language != -1) {
				PlayerRaces.Languages[language]->LanguageNouns.push_back(noun);
			} else {
				LuaError(l, "Language not found.");
			}
		} else if (!strcmp(value, "Meanings")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				noun->Meanings.push_back(LuaToString(l, -1, k + 1));
			}
		} else if (!strcmp(value, "SingularNominative")) {
			noun->SingularNominative = LuaToString(l, -1);
		} else if (!strcmp(value, "SingularAccusative")) {
			noun->SingularAccusative = LuaToString(l, -1);
		} else if (!strcmp(value, "SingularDative")) {
			noun->SingularDative = LuaToString(l, -1);
		} else if (!strcmp(value, "SingularGenitive")) {
			noun->SingularGenitive = LuaToString(l, -1);
		} else if (!strcmp(value, "PluralNominative")) {
			noun->PluralNominative = LuaToString(l, -1);
		} else if (!strcmp(value, "PluralAccusative")) {
			noun->PluralAccusative = LuaToString(l, -1);
		} else if (!strcmp(value, "PluralDative")) {
			noun->PluralDative = LuaToString(l, -1);
		} else if (!strcmp(value, "PluralGenitive")) {
			noun->PluralGenitive = LuaToString(l, -1);
		} else if (!strcmp(value, "Gender")) {
			noun->Gender = LuaToString(l, -1);
		} else if (!strcmp(value, "Uncountable")) {
			noun->Uncountable = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "NameSingular")) {
			noun->NameSingular = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "NamePlural")) {
			noun->NamePlural = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "TypeName")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				noun->TypeName.push_back(LuaToString(l, -1, k + 1));
			}
		} else if (!strcmp(value, "PrefixSingular")) {
			noun->PrefixSingular = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "PrefixPlural")) {
			noun->PrefixPlural = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "PrefixTypeName")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				noun->PrefixTypeName.push_back(LuaToString(l, -1, k + 1));
			}
		} else if (!strcmp(value, "SeparatePrefixTypeName")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				noun->SeparatePrefixTypeName.push_back(LuaToString(l, -1, k + 1));
			}
		} else if (!strcmp(value, "SuffixSingular")) {
			noun->SuffixSingular = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "SuffixPlural")) {
			noun->SuffixPlural = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "SuffixTypeName")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				noun->SuffixTypeName.push_back(LuaToString(l, -1, k + 1));
			}
		} else if (!strcmp(value, "SeparateSuffixTypeName")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				noun->SeparateSuffixTypeName.push_back(LuaToString(l, -1, k + 1));
			}
		} else if (!strcmp(value, "InfixSingular")) {
			noun->InfixSingular = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "InfixPlural")) {
			noun->InfixPlural = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "InfixTypeName")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				noun->InfixTypeName.push_back(LuaToString(l, -1, k + 1));
			}
		} else if (!strcmp(value, "SeparateInfixTypeName")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				noun->SeparateInfixTypeName.push_back(LuaToString(l, -1, k + 1));
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	return 0;
}

/**
**  Define a verb for a particular civilization's language.
**
**  @param l  Lua state.
*/
static int CclDefineLanguageVerb(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	LanguageVerb *verb = new LanguageVerb;
	verb->Word = LuaToString(l, 1);
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Language")) {
			int language = PlayerRaces.GetLanguageIndexByIdent(LuaToString(l, -1));
			
			if (language != -1) {
				PlayerRaces.Languages[language]->LanguageVerbs.push_back(verb);
			} else {
				LuaError(l, "Language not found.");
			}
		} else if (!strcmp(value, "Meanings")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				verb->Meanings.push_back(LuaToString(l, -1, k + 1));
			}
		} else if (!strcmp(value, "Infinitive")) {
			verb->Infinitive = LuaToString(l, -1);
		} else if (!strcmp(value, "SingularFirstPersonPresent")) {
			verb->SingularFirstPersonPresent = LuaToString(l, -1);
		} else if (!strcmp(value, "SingularSecondPersonPresent")) {
			verb->SingularSecondPersonPresent = LuaToString(l, -1);
		} else if (!strcmp(value, "SingularThirdPersonPresent")) {
			verb->SingularThirdPersonPresent = LuaToString(l, -1);
		} else if (!strcmp(value, "PluralFirstPersonPresent")) {
			verb->PluralFirstPersonPresent = LuaToString(l, -1);
		} else if (!strcmp(value, "PluralSecondPersonPresent")) {
			verb->PluralSecondPersonPresent = LuaToString(l, -1);
		} else if (!strcmp(value, "PluralThirdPersonPresent")) {
			verb->PluralThirdPersonPresent = LuaToString(l, -1);
		} else if (!strcmp(value, "SingularFirstPersonPresentSubjunctive")) {
			verb->SingularFirstPersonPresentSubjunctive = LuaToString(l, -1);
		} else if (!strcmp(value, "SingularSecondPersonPresentSubjunctive")) {
			verb->SingularSecondPersonPresentSubjunctive = LuaToString(l, -1);
		} else if (!strcmp(value, "SingularThirdPersonPresentSubjunctive")) {
			verb->SingularThirdPersonPresentSubjunctive = LuaToString(l, -1);
		} else if (!strcmp(value, "PluralFirstPersonPresentSubjunctive")) {
			verb->PluralFirstPersonPresentSubjunctive = LuaToString(l, -1);
		} else if (!strcmp(value, "PluralSecondPersonPresentSubjunctive")) {
			verb->PluralSecondPersonPresentSubjunctive = LuaToString(l, -1);
		} else if (!strcmp(value, "PluralThirdPersonPresentSubjunctive")) {
			verb->PluralThirdPersonPresentSubjunctive = LuaToString(l, -1);
		} else if (!strcmp(value, "SingularFirstPersonPast")) {
			verb->SingularFirstPersonPast = LuaToString(l, -1);
		} else if (!strcmp(value, "SingularSecondPersonPast")) {
			verb->SingularSecondPersonPast = LuaToString(l, -1);
		} else if (!strcmp(value, "SingularThirdPersonPast")) {
			verb->SingularThirdPersonPast = LuaToString(l, -1);
		} else if (!strcmp(value, "PluralFirstPersonPast")) {
			verb->PluralFirstPersonPast = LuaToString(l, -1);
		} else if (!strcmp(value, "PluralSecondPersonPast")) {
			verb->PluralSecondPersonPast = LuaToString(l, -1);
		} else if (!strcmp(value, "PluralThirdPersonPast")) {
			verb->PluralThirdPersonPast = LuaToString(l, -1);
		} else if (!strcmp(value, "SingularFirstPersonFuture")) {
			verb->SingularFirstPersonFuture = LuaToString(l, -1);
		} else if (!strcmp(value, "SingularSecondPersonFuture")) {
			verb->SingularSecondPersonFuture = LuaToString(l, -1);
		} else if (!strcmp(value, "SingularThirdPersonFuture")) {
			verb->SingularThirdPersonFuture = LuaToString(l, -1);
		} else if (!strcmp(value, "PluralFirstPersonFuture")) {
			verb->PluralFirstPersonFuture = LuaToString(l, -1);
		} else if (!strcmp(value, "PluralSecondPersonFuture")) {
			verb->PluralSecondPersonFuture = LuaToString(l, -1);
		} else if (!strcmp(value, "PluralThirdPersonFuture")) {
			verb->PluralThirdPersonFuture = LuaToString(l, -1);
		} else if (!strcmp(value, "ParticiplePresent")) {
			verb->ParticiplePresent = LuaToString(l, -1);
		} else if (!strcmp(value, "ParticiplePast")) {
			verb->ParticiplePast = LuaToString(l, -1);
		} else if (!strcmp(value, "TypeName")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				verb->TypeName.push_back(LuaToString(l, -1, k + 1));
			}
		} else if (!strcmp(value, "PrefixTypeName")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				verb->PrefixTypeName.push_back(LuaToString(l, -1, k + 1));
			}
		} else if (!strcmp(value, "SeparatePrefixTypeName")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				verb->SeparatePrefixTypeName.push_back(LuaToString(l, -1, k + 1));
			}
		} else if (!strcmp(value, "SuffixTypeName")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				verb->SuffixTypeName.push_back(LuaToString(l, -1, k + 1));
			}
		} else if (!strcmp(value, "SeparateSuffixTypeName")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				verb->SeparateSuffixTypeName.push_back(LuaToString(l, -1, k + 1));
			}
		} else if (!strcmp(value, "InfixTypeName")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				verb->InfixTypeName.push_back(LuaToString(l, -1, k + 1));
			}
		} else if (!strcmp(value, "SeparateInfixTypeName")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				verb->SeparateInfixTypeName.push_back(LuaToString(l, -1, k + 1));
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	return 0;
}

/**
**  Define an adjective for a particular civilization's language.
**
**  @param l  Lua state.
*/
static int CclDefineLanguageAdjective(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	LanguageAdjective *adjective = new LanguageAdjective;
	adjective->Word = LuaToString(l, 1);
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Language")) {
			int language = PlayerRaces.GetLanguageIndexByIdent(LuaToString(l, -1));

			if (language != -1) {
				PlayerRaces.Languages[language]->LanguageAdjectives.push_back(adjective);
			} else {
				LuaError(l, "Language not found.");
			}
		} else if (!strcmp(value, "Meanings")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				adjective->Meanings.push_back(LuaToString(l, -1, k + 1));
			}
		} else if (!strcmp(value, "Positive")) {
			adjective->Positive = LuaToString(l, -1);
		} else if (!strcmp(value, "Comparative")) {
			adjective->Comparative = LuaToString(l, -1);
		} else if (!strcmp(value, "Superlative")) {
			adjective->Superlative = LuaToString(l, -1);
		} else if (!strcmp(value, "PositivePlural")) {
			adjective->PositivePlural = LuaToString(l, -1);
		} else if (!strcmp(value, "ComparativePlural")) {
			adjective->ComparativePlural = LuaToString(l, -1);
		} else if (!strcmp(value, "SuperlativePlural")) {
			adjective->SuperlativePlural = LuaToString(l, -1);
		} else if (!strcmp(value, "TypeName")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				adjective->TypeName.push_back(LuaToString(l, -1, k + 1));
			}
		} else if (!strcmp(value, "PrefixTypeName")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				adjective->PrefixTypeName.push_back(LuaToString(l, -1, k + 1));
			}
		} else if (!strcmp(value, "SeparatePrefixTypeName")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				adjective->SeparatePrefixTypeName.push_back(LuaToString(l, -1, k + 1));
			}
		} else if (!strcmp(value, "SuffixTypeName")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				adjective->SuffixTypeName.push_back(LuaToString(l, -1, k + 1));
			}
		} else if (!strcmp(value, "SeparateSuffixTypeName")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				adjective->SeparateSuffixTypeName.push_back(LuaToString(l, -1, k + 1));
			}
		} else if (!strcmp(value, "InfixTypeName")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				adjective->InfixTypeName.push_back(LuaToString(l, -1, k + 1));
			}
		} else if (!strcmp(value, "SeparateInfixTypeName")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				adjective->SeparateInfixTypeName.push_back(LuaToString(l, -1, k + 1));
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	return 0;
}

/**
**  Define a pronoun for a civilization's language.
**
**  @param l  Lua state.
*/
static int CclDefineLanguagePronoun(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	LanguagePronoun *pronoun = new LanguagePronoun;
	pronoun->Word = LuaToString(l, 1);
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Language")) {
			int language = PlayerRaces.GetLanguageIndexByIdent(LuaToString(l, -1));
			
			if (language != -1) {
				PlayerRaces.Languages[language]->LanguagePronouns.push_back(pronoun);
			} else {
				LuaError(l, "Language not found.");
			}
		} else if (!strcmp(value, "Meanings")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				pronoun->Meanings.push_back(LuaToString(l, -1, k + 1));
			}
		} else if (!strcmp(value, "Nominative")) {
			pronoun->Nominative = LuaToString(l, -1);
		} else if (!strcmp(value, "Accusative")) {
			pronoun->Accusative = LuaToString(l, -1);
		} else if (!strcmp(value, "Dative")) {
			pronoun->Dative = LuaToString(l, -1);
		} else if (!strcmp(value, "Genitive")) {
			pronoun->Genitive = LuaToString(l, -1);
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	return 0;
}

/**
**  Define an adverb for a civilization's language.
**
**  @param l  Lua state.
*/
static int CclDefineLanguageAdverb(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	LanguageAdverb *adverb = new LanguageAdverb;
	adverb->Word = LuaToString(l, 1);
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Language")) {
			int language = PlayerRaces.GetLanguageIndexByIdent(LuaToString(l, -1));
			
			if (language != -1) {
				PlayerRaces.Languages[language]->LanguageAdverbs.push_back(adverb);
			} else {
				LuaError(l, "Language not found.");
			}
		} else if (!strcmp(value, "Meanings")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				adverb->Meanings.push_back(LuaToString(l, -1, k + 1));
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	return 0;
}

/**
**  Define an conjunction for a civilization's language.
**
**  @param l  Lua state.
*/
static int CclDefineLanguageConjunction(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	LanguageConjunction *conjunction = new LanguageConjunction;
	conjunction->Word = LuaToString(l, 1);
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Language")) {
			int language = PlayerRaces.GetLanguageIndexByIdent(LuaToString(l, -1));
			
			if (language != -1) {
				PlayerRaces.Languages[language]->LanguageConjunctions.push_back(conjunction);
			} else {
				LuaError(l, "Language not found.");
			}
		} else if (!strcmp(value, "Meanings")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				conjunction->Meanings.push_back(LuaToString(l, -1, k + 1));
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	return 0;
}

/**
**  Define an adposition for a language.
**
**  @param l  Lua state.
*/
static int CclDefineLanguageAdposition(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	LanguageAdposition *adposition = new LanguageAdposition;
	adposition->Word = LuaToString(l, 1);
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Language")) {
			int language = PlayerRaces.GetLanguageIndexByIdent(LuaToString(l, -1));
			
			if (language != -1) {
				PlayerRaces.Languages[language]->LanguageAdpositions.push_back(adposition);
			} else {
				LuaError(l, "Language not found.");
			}
		} else if (!strcmp(value, "Meanings")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				adposition->Meanings.push_back(LuaToString(l, -1, k + 1));
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	return 0;
}

/**
**  Define an article for a language.
**
**  @param l  Lua state.
*/
static int CclDefineLanguageArticle(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	LanguageArticle *article = new LanguageArticle;
	article->Word = LuaToString(l, 1);
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Language")) {
			int language = PlayerRaces.GetLanguageIndexByIdent(LuaToString(l, -1));
			
			if (language != -1) {
				PlayerRaces.Languages[language]->LanguageArticles.push_back(article);
			} else {
				LuaError(l, "Language not found.");
			}
		} else if (!strcmp(value, "Meanings")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				article->Meanings.push_back(LuaToString(l, -1, k + 1));
			}
		} else if (!strcmp(value, "Nominative")) {
			article->Nominative = LuaToString(l, -1);
		} else if (!strcmp(value, "Accusative")) {
			article->Accusative = LuaToString(l, -1);
		} else if (!strcmp(value, "Dative")) {
			article->Dative = LuaToString(l, -1);
		} else if (!strcmp(value, "Genitive")) {
			article->Genitive = LuaToString(l, -1);
		} else if (!strcmp(value, "Gender")) {
			article->Gender = LuaToString(l, -1);
		} else if (!strcmp(value, "Definite")) {
			article->Definite = LuaToBoolean(l, -1);
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	return 0;
}

/**
**  Define a numeral for a civilization's language.
**
**  @param l  Lua state.
*/
static int CclDefineLanguageNumeral(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	LanguageNumeral *numeral = new LanguageNumeral;
	numeral->Word = LuaToString(l, 1);
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Language")) {
			int language = PlayerRaces.GetLanguageIndexByIdent(LuaToString(l, -1));
			
			if (language != -1) {
				PlayerRaces.Languages[language]->LanguageNumerals.push_back(numeral);
			} else {
				LuaError(l, "Language not found.");
			}
		} else if (!strcmp(value, "Number")) {
			numeral->Number = LuaToNumber(l, -1);
		} else if (!strcmp(value, "PrefixTypeName")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				numeral->PrefixTypeName.push_back(LuaToString(l, -1, k + 1));
			}
		} else if (!strcmp(value, "SeparatePrefixTypeName")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				numeral->SeparatePrefixTypeName.push_back(LuaToString(l, -1, k + 1));
			}
		} else if (!strcmp(value, "SuffixTypeName")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				numeral->SuffixTypeName.push_back(LuaToString(l, -1, k + 1));
			}
		} else if (!strcmp(value, "SeparateSuffixTypeName")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				numeral->SeparateSuffixTypeName.push_back(LuaToString(l, -1, k + 1));
			}
		} else if (!strcmp(value, "InfixTypeName")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				numeral->InfixTypeName.push_back(LuaToString(l, -1, k + 1));
			}
		} else if (!strcmp(value, "SeparateInfixTypeName")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				numeral->SeparateInfixTypeName.push_back(LuaToString(l, -1, k + 1));
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
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
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	if (civilization == -1) {
		LuaError(l, "Civilization \"%s\" doesn't exist." _C_ civilization_name.c_str());
	}
	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Playable")) {
		lua_pushboolean(l, PlayerRaces.Playable[civilization]);
		return 1;
	} else if (!strcmp(data, "Species")) {
		lua_pushstring(l, PlayerRaces.Species[civilization].c_str());
		return 1;
	} else if (!strcmp(data, "ParentCivilization")) {
		int parent_civilization = PlayerRaces.ParentCivilization[civilization];
		if (parent_civilization != -1) {
			lua_pushstring(l, PlayerRaces.Name[parent_civilization].c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Language")) {
		int language = PlayerRaces.GetCivilizationLanguage(civilization);
		if (language != -1) {
			lua_pushstring(l, PlayerRaces.Languages[language]->Ident.c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "DefaultColor")) {
		lua_pushstring(l, PlayerRaces.DefaultColor[civilization].c_str());
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
				parent_faction = PlayerRaces.Factions[i][faction->ParentFaction]->Name;
			}
			break;
		}
	}
	
	if (faction_id == -1) {
		faction = new CFaction;
		faction->Name = faction_name;
	}
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Civilization")) {
			if (civilization == -1) { //don't change the civilization in redefinitions
				civilization = PlayerRaces.GetRaceIndexByName(LuaToString(l, -1));
				
				for (int i = 0; i < FactionMax; ++i) {
					if (!PlayerRaces.Factions[civilization][i] || PlayerRaces.Factions[civilization][i]->Name.empty()) {
						PlayerRaces.Factions[civilization][i] = faction;
						SetFactionStringToIndex(civilization, PlayerRaces.Factions[civilization][i]->Name, i);
						faction->Civilization = civilization;
						faction->ID = i;
						break;
					}
				}
			}
		} else if (!strcmp(value, "Type")) {
			faction->Type = LuaToString(l, -1);
		} else if (!strcmp(value, "Colors")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			faction->Colors.clear(); //remove previously defined colors
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				std::string color_name = LuaToString(l, -1, k + 1);
				for (int c = 0; c < PlayerColorMax; ++c) {
					if (PlayerColorNames[c] == color_name) {
						faction->Colors.push_back(c);
						break;
					}
				}
			}
		} else if (!strcmp(value, "DefaultTier")) {
			faction->DefaultTier = GetFactionTierIdByName(LuaToString(l, -1));
		} else if (!strcmp(value, "ParentFaction")) {
			parent_faction = LuaToString(l, -1);
		} else if (!strcmp(value, "Language")) {
			int language = PlayerRaces.GetLanguageIndexByIdent(LuaToString(l, -1));
			
			if (language != -1) {
				faction->Language = language;
			} else {
				LuaError(l, "Language not found.");
			}
		} else if (!strcmp(value, "Playable")) {
			faction->Playable = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "DevelopsTo")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				faction->DevelopsTo.push_back(LuaToString(l, -1, k + 1));
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
		} else if (!strcmp(value, "FactionUpgrade")) {
			faction->FactionUpgrade = LuaToString(l, -1);
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

	std::string deity_name = LuaToString(l, 1);
	CDeity *deity = NULL;
	int deity_id = -1;
	for (int i = 0; i < MAX_RACES; ++i) {
		deity_id = PlayerRaces.GetDeityIndexByName(i, deity_name);
		if (deity_id != -1) {
			deity = const_cast<CDeity *>(&(*PlayerRaces.Deities[i][deity_id]));
			break;
		}
	}
	if (deity_id == -1) {
		deity = new CDeity;
	}
	
	deity->Name = deity_name;
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Civilization")) {
			if (deity->Civilization != -1) { //if already has a civilization defined, remove the deity from that civilization
				PlayerRaces.Deities[deity->Civilization].erase(std::remove(PlayerRaces.Deities[deity->Civilization].begin(), PlayerRaces.Deities[deity->Civilization].end(), deity), PlayerRaces.Deities[deity->Civilization].end());
			}
			deity->Civilization = PlayerRaces.GetRaceIndexByName(LuaToString(l, -1));
			PlayerRaces.Deities[deity->Civilization].push_back(deity);
		} else if (!strcmp(value, "Portfolio")) {
			deity->Portfolio = LuaToString(l, -1);
		} else if (!strcmp(value, "ParentDeity")) {
			deity->ParentDeity = LuaToString(l, -1);
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
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
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
		} else if (!strcmp(value, "NameTranslations")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int k = 0; k < subargs; ++k) {
				language->NameTranslations[k / 2][0] = LuaToString(l, -1, k + 1); //name to be translated
				++k;
				language->NameTranslations[(k - 1) / 2][1] = LuaToString(l, -1, k + 1); //name translation
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

	int FactionCount = 0;
	for (int i = 0; i < FactionMax; ++i) {
		if (PlayerRaces.Factions[civilization][i] && !PlayerRaces.Factions[civilization][i]->Name.empty()) {
			FactionCount += 1;
		}
	}

	lua_createtable(l, FactionCount, 0);
	for (int i = 1; i <= FactionCount; ++i)
	{
		lua_pushstring(l, PlayerRaces.Factions[civilization][i-1]->Name.c_str());
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
	const char *faction_name = LuaToString(l, 2);
	int civilization_faction = 0;
	for (int i = 0; i < FactionMax; ++i) {
		if (PlayerRaces.Factions[civilization][i] && PlayerRaces.Factions[civilization][i]->Name.compare(faction_name) == 0) {
			civilization_faction = i;
			break;
		}
	}
	const char *data = LuaToString(l, 3);

	if (!strcmp(data, "Type")) {
		lua_pushstring(l, PlayerRaces.Factions[civilization][civilization_faction]->Type.c_str());
		return 1;
	} else if (!strcmp(data, "Color")) {
		if (PlayerRaces.Factions[civilization][civilization_faction]->Colors.size() > 0) {
			lua_pushstring(l, PlayerColorNames[PlayerRaces.Factions[civilization][civilization_faction]->Colors[0]].c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Playable")) {
		lua_pushboolean(l, PlayerRaces.Factions[civilization][civilization_faction]->Playable);
		return 1;
	} else if (!strcmp(data, "Language")) {
		int language = PlayerRaces.GetFactionLanguage(civilization, civilization_faction);
		if (language != -1) {
			lua_pushstring(l, PlayerRaces.Languages[language]->Ident.c_str());
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
			lua_pushstring(l, PlayerRaces.Factions[p->Race][p->Faction]->Name.c_str());
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
	} else if (!strcmp(data, "NumBuildings")) {
		lua_pushnumber(l, p->NumBuildings);
		return 1;
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
		if (ThisPlayer) { //a way to check if this is in-game or not
			p->SetFaction("");
		}

		const char *racename = LuaToString(l, 3);
		p->Race = PlayerRaces.GetRaceIndexByName(racename);
		
		if (p->Race == -1) {
			LuaError(l, "invalid race name '%s'" _C_ racename);
		}
		
		//Wyrmgus start
		//if the civilization of the person player changed, update the UI
		if (ThisPlayer) {
			if (ThisPlayer->Index == p->Index) {
				LoadCursors(PlayerRaces.Name[p->Race]);
				UI.Load();
			}
		} else if (p->Index == 0) {
			LoadCursors(PlayerRaces.Name[p->Race]);
			UI.Load();
		}
		SetDefaultTextColors(UI.NormalFontColor, UI.ReverseFontColor);
		//Wyrmgus end
		
		if (GrandStrategy && ThisPlayer) {
			p->SetRandomFaction();
		}
	//Wyrmgus start
	} else if (!strcmp(data, "Faction")) {
		std::string faction_name = LuaToString(l, 3);
		if (faction_name == "Random") {
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
	lua_register(Lua, "DefineCivilizationLanguage", CclDefineCivilizationLanguage);
	lua_register(Lua, "DefineLanguageNoun", CclDefineLanguageNoun);
	lua_register(Lua, "DefineLanguageVerb", CclDefineLanguageVerb);
	lua_register(Lua, "DefineLanguageAdjective", CclDefineLanguageAdjective);
	lua_register(Lua, "DefineLanguagePronoun", CclDefineLanguagePronoun);
	lua_register(Lua, "DefineLanguageAdverb", CclDefineLanguageAdverb);
	lua_register(Lua, "DefineLanguageConjunction", CclDefineLanguageConjunction);
	lua_register(Lua, "DefineLanguageAdposition", CclDefineLanguageAdposition);
	lua_register(Lua, "DefineLanguageArticle", CclDefineLanguageArticle);
	lua_register(Lua, "DefineLanguageNumeral", CclDefineLanguageNumeral);
	lua_register(Lua, "GetCivilizationData", CclGetCivilizationData);
	lua_register(Lua, "GetCivilizationClassUnitType", CclGetCivilizationClassUnitType);
	lua_register(Lua, "GetFactionClassUnitType", CclGetFactionClassUnitType);
	lua_register(Lua, "DefineFaction", CclDefineFaction);
	lua_register(Lua, "DefineDeity", CclDefineDeity);
	lua_register(Lua, "DefineLanguage", CclDefineLanguage);
	lua_register(Lua, "GetCivilizations", CclGetCivilizations);
	lua_register(Lua, "GetCivilizationFactionNames", CclGetCivilizationFactionNames);
	lua_register(Lua, "GetFactionData", CclGetFactionData);
	lua_register(Lua, "GetFactionDevelopsTo", CclGetFactionDevelopsTo);
	//Wyrmgus end
	lua_register(Lua, "DefinePlayerColors", CclDefinePlayerColors);
	lua_register(Lua, "DefinePlayerColorIndex", CclDefinePlayerColorIndex);

	lua_register(Lua, "NewColors", CclNewPlayerColors);

	// player member access functions
	lua_register(Lua, "GetPlayerData", CclGetPlayerData);
	lua_register(Lua, "SetPlayerData", CclSetPlayerData);
	lua_register(Lua, "SetAiType", CclSetAiType);
}

//@}
