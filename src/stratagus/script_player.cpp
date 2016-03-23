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
				PlayerRaces.Languages[language]->UsedByCivilizationOrFaction = true;
			} else {
				LuaError(l, "Language not found.");
			}
		} else if (!strcmp(value, "DefaultColor")) {
			PlayerRaces.DefaultColor[civilization] = LuaToString(l, -1);
		} else if (!strcmp(value, "MoveIcon")) {
			PlayerRaces.MoveIcon[civilization].Name = LuaToString(l, -1);
			PlayerRaces.MoveIcon[civilization].Icon = NULL;
			PlayerRaces.MoveIcon[civilization].Load();
		} else if (!strcmp(value, "StopIcon")) {
			PlayerRaces.StopIcon[civilization].Name = LuaToString(l, -1);
			PlayerRaces.StopIcon[civilization].Icon = NULL;
			PlayerRaces.StopIcon[civilization].Load();
		} else if (!strcmp(value, "AttackIcon")) {
			PlayerRaces.AttackIcon[civilization].Name = LuaToString(l, -1);
			PlayerRaces.AttackIcon[civilization].Icon = NULL;
			PlayerRaces.AttackIcon[civilization].Load();
		} else if (!strcmp(value, "PatrolIcon")) {
			PlayerRaces.PatrolIcon[civilization].Name = LuaToString(l, -1);
			PlayerRaces.PatrolIcon[civilization].Icon = NULL;
			PlayerRaces.PatrolIcon[civilization].Load();
		} else if (!strcmp(value, "StandGroundIcon")) {
			PlayerRaces.StandGroundIcon[civilization].Name = LuaToString(l, -1);
			PlayerRaces.StandGroundIcon[civilization].Icon = NULL;
			PlayerRaces.StandGroundIcon[civilization].Load();
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	if (!PlayerRaces.MoveIcon[civilization].Name.empty()) {
		std::string button_definition = "DefineButton({\n";
		button_definition += "\tPos = 1,\n";
		button_definition += "\tLevel = 0,\n";
		button_definition += "\tAction = \"move\",\n";
		button_definition += "\tPopup = \"popup-commands\",\n";
		button_definition += "\tKey = \"m\",\n";
		button_definition += "\tHint = _(\"~!Move\"),\n";
		button_definition += "\tForUnit = {\"" + PlayerRaces.Name[civilization] + "-group\"},\n";
		button_definition += "})";
		CclCommand(button_definition);
	}
	
	if (!PlayerRaces.StopIcon[civilization].Name.empty()) {
		std::string button_definition = "DefineButton({\n";
		button_definition += "\tPos = 2,\n";
		button_definition += "\tLevel = 0,\n";
		button_definition += "\tAction = \"stop\",\n";
		button_definition += "\tPopup = \"popup-commands\",\n";
		button_definition += "\tKey = \"s\",\n";
		button_definition += "\tHint = _(\"~!Stop\"),\n";
		button_definition += "\tForUnit = {\"" + PlayerRaces.Name[civilization] + "-group\"},\n";
		button_definition += "})";
		CclCommand(button_definition);
	}
	
	if (!PlayerRaces.AttackIcon[civilization].Name.empty()) {
		std::string button_definition = "DefineButton({\n";
		button_definition += "\tPos = 3,\n";
		button_definition += "\tLevel = 0,\n";
		button_definition += "\tAction = \"attack\",\n";
		button_definition += "\tPopup = \"popup-commands\",\n";
		button_definition += "\tKey = \"a\",\n";
		button_definition += "\tHint = _(\"~!Attack\"),\n";
		button_definition += "\tForUnit = {\"" + PlayerRaces.Name[civilization] + "-group\"},\n";
		button_definition += "})";
		CclCommand(button_definition);
	}
	
	if (!PlayerRaces.PatrolIcon[civilization].Name.empty()) {
		std::string button_definition = "DefineButton({\n";
		button_definition += "\tPos = 4,\n";
		button_definition += "\tLevel = 0,\n";
		button_definition += "\tAction = \"patrol\",\n";
		button_definition += "\tPopup = \"popup-commands\",\n";
		button_definition += "\tKey = \"p\",\n";
		button_definition += "\tHint = _(\"~!Patrol\"),\n";
		button_definition += "\tForUnit = {\"" + PlayerRaces.Name[civilization] + "-group\"},\n";
		button_definition += "})";
		CclCommand(button_definition);
	}
	
	if (!PlayerRaces.StandGroundIcon[civilization].Name.empty()) {
		std::string button_definition = "DefineButton({\n";
		button_definition += "\tPos = 5,\n";
		button_definition += "\tLevel = 0,\n";
		button_definition += "\tAction = \"stand-ground\",\n";
		button_definition += "\tPopup = \"popup-commands\",\n";
		button_definition += "\tKey = \"t\",\n";
		button_definition += "\tHint = _(\"S~!tand Ground\"),\n";
		button_definition += "\tForUnit = {\"" + PlayerRaces.Name[civilization] + "-group\"},\n";
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
				if (word->NameTypes[grammatical_number][grammatical_case][grammatical_tense].find(type) == word->NameTypes[grammatical_number][grammatical_case][grammatical_tense].end()) {
					word->NameTypes[grammatical_number][grammatical_case][grammatical_tense][type] = 0;
				}
				word->NameTypes[grammatical_number][grammatical_case][grammatical_tense][type] += 1;
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
				if (word->AffixNameTypes[word_junction_type][affix_type][grammatical_number][grammatical_case][grammatical_tense].find(type) == word->AffixNameTypes[word_junction_type][affix_type][grammatical_number][grammatical_case][grammatical_tense].end()) {
					word->AffixNameTypes[word_junction_type][affix_type][grammatical_number][grammatical_case][grammatical_tense][type] = 0;
				}
				word->AffixNameTypes[word_junction_type][affix_type][grammatical_number][grammatical_case][grammatical_tense][type] += 1;
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
	
	if (word->DerivesFrom != NULL) { //if the word is derived from another, add name translations for them going both ways
		PlayerRaces.Languages[word->Language]->NameTranslations[0].push_back(word->DerivesFrom->Word);
		PlayerRaces.Languages[word->Language]->NameTranslations[1].push_back(word->Word);
		
		PlayerRaces.Languages[word->DerivesFrom->Language]->NameTranslations[0].push_back(word->Word);
		PlayerRaces.Languages[word->DerivesFrom->Language]->NameTranslations[1].push_back(word->DerivesFrom->Word);
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
	int civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
	if (civilization == -1) {
		LuaError(l, "Civilization \"%s\" doesn't exist." _C_ civilization_name.c_str());
	}
	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Display")) {
		lua_pushstring(l, PlayerRaces.Display[civilization].c_str());
		return 1;
	} else if (!strcmp(data, "Playable")) {
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
		for (size_t i = 0; i < PlayerRaces.Factions[civilization].size(); ++i)
		{
			if (!is_mod || PlayerRaces.Factions[civilization][i]->Mod == mod_file) {
				factions.push_back(PlayerRaces.Factions[civilization][i]->Name);
			}
		}
		
		lua_createtable(l, factions.size(), 0);
		for (size_t i = 1; i <= factions.size(); ++i)
		{
			lua_pushstring(l, factions[i-1].c_str());
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
				
				faction->ID = PlayerRaces.Factions[civilization].size();
				PlayerRaces.Factions[civilization].push_back(faction);
				SetFactionStringToIndex(civilization, faction->Name, faction->ID);
				faction->Civilization = civilization;
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
			std::string faction_tier_name = LuaToString(l, -1);
			int faction_tier = GetFactionTierIdByName(faction_tier_name);
			if (faction_tier != -1) {
				faction->DefaultTier = faction_tier;
			} else {
				LuaError(l, "Faction tier \"%s\" doesn't exist." _C_ faction_tier_name.c_str());
			}
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
	} else if (parent_faction.empty()) {
		faction->ParentFaction = -1; // to allow redefinitions to remove the parent faction setting
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
				language->NameTranslations[0].push_back(LuaToString(l, -1, k + 1)); //name to be translated
				++k;
				language->NameTranslations[1].push_back(LuaToString(l, -1, k + 1)); //name translation
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
		lua_pushstring(l, PlayerRaces.Factions[civilization][i-1]->Name.c_str());
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
			factions.push_back(PlayerRaces.Factions[civilization][i]->Name);
		}
	} else {
		for (int i = 0; i < MAX_RACES; ++i)
		{
			for (size_t j = 0; j < PlayerRaces.Factions[i].size(); ++j)
			{
				factions.push_back(PlayerRaces.Factions[i][j]->Name);
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

	if (!strcmp(data, "Type")) {
		lua_pushstring(l, PlayerRaces.Factions[civilization][faction]->Type.c_str());
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
			lua_pushstring(l, PlayerRaces.Factions[civilization][PlayerRaces.Factions[civilization][faction]->ParentFaction]->Name.c_str());
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
		if (GameRunning) {
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
				UI.Load();
			}
		} else if (p->Index == 0) {
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

	// player member access functions
	lua_register(Lua, "GetPlayerData", CclGetPlayerData);
	lua_register(Lua, "SetPlayerData", CclSetPlayerData);
	lua_register(Lua, "SetAiType", CclSetAiType);
	//Wyrmgus start
	lua_register(Lua, "GetLanguages", CclGetLanguages);
	lua_register(Lua, "GetLanguageData", CclGetLanguageData);
	lua_register(Lua, "GetLanguageWordData", CclGetLanguageWordData);
	//Wyrmgus end
}

//@}
