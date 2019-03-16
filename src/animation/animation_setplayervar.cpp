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
/**@name animation_setplayervar.cpp - The animation SetPlayerVar. */
//
//      (c) Copyright 2012 by Joris Dauphin
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

#include "animation/animation_setplayervar.h"

#include "unit/unit.h"

#include <stdio.h>

/**
**  Gets the player data.
**
**  @param player  Player number.
**  @param prop    Player's property.
**  @param arg     Additional argument (for resource and unit).
**
**  @return  Returning value (only integer).
*/
int GetPlayerData(const int player, const char *prop, const char *arg)
{
	if (!strcmp(prop, "RaceName")) {
		return CPlayer::Players[player]->Race;
	} else if (!strcmp(prop, "Resources")) {
		const int resId = GetResourceIdByName(arg);
		if (resId == -1) {
			fprintf(stderr, "Invalid resource \"%s\"", arg);
			Exit(1);
		}
		return CPlayer::Players[player]->Resources[resId] + CPlayer::Players[player]->StoredResources[resId];
	} else if (!strcmp(prop, "StoredResources")) {
		const int resId = GetResourceIdByName(arg);
		if (resId == -1) {
			fprintf(stderr, "Invalid resource \"%s\"", arg);
			Exit(1);
		}
		return CPlayer::Players[player]->StoredResources[resId];
	} else if (!strcmp(prop, "MaxResources")) {
		const int resId = GetResourceIdByName(arg);
		if (resId == -1) {
			fprintf(stderr, "Invalid resource \"%s\"", arg);
			Exit(1);
		}
		return CPlayer::Players[player]->MaxResources[resId];
	} else if (!strcmp(prop, "Incomes")) {
		const int resId = GetResourceIdByName(arg);
		if (resId == -1) {
			fprintf(stderr, "Invalid resource \"%s\"", arg);
			Exit(1);
		}
		return CPlayer::Players[player]->Incomes[resId];
	//Wyrmgus start
	} else if (!strcmp(prop, "Prices")) {
		const int resId = GetResourceIdByName(arg);
		if (resId == -1) {
			fprintf(stderr, "Invalid resource \"%s\"", arg);
			Exit(1);
		}
		return CPlayer::Players[player]->GetResourcePrice(resId);
	} else if (!strcmp(prop, "ResourceDemand")) {
		const int resId = GetResourceIdByName(arg);
		if (resId == -1) {
			fprintf(stderr, "Invalid resource \"%s\"", arg);
			Exit(1);
		}
		return CPlayer::Players[player]->ResourceDemand[resId];
	} else if (!strcmp(prop, "StoredResourceDemand")) {
		const int resId = GetResourceIdByName(arg);
		if (resId == -1) {
			fprintf(stderr, "Invalid resource \"%s\"", arg);
			Exit(1);
		}
		return CPlayer::Players[player]->StoredResourceDemand[resId];
	} else if (!strcmp(prop, "EffectiveResourceDemand")) {
		const int resId = GetResourceIdByName(arg);
		if (resId == -1) {
			fprintf(stderr, "Invalid resource \"%s\"", arg);
			Exit(1);
		}
		return CPlayer::Players[player]->GetEffectiveResourceDemand(resId);
	} else if (!strcmp(prop, "EffectiveResourceBuyPrice")) {
		const int resId = GetResourceIdByName(arg);
		if (resId == -1) {
			fprintf(stderr, "Invalid resource \"%s\"", arg);
			Exit(1);
		}
		return CPlayer::Players[player]->GetEffectiveResourceBuyPrice(resId);
	} else if (!strcmp(prop, "EffectiveResourceSellPrice")) {
		const int resId = GetResourceIdByName(arg);
		if (resId == -1) {
			fprintf(stderr, "Invalid resource \"%s\"", arg);
			Exit(1);
		}
		return CPlayer::Players[player]->GetEffectiveResourceSellPrice(resId);
	} else if (!strcmp(prop, "TradeCost")) {
		return CPlayer::Players[player]->TradeCost;
	//Wyrmgus end
	} else if (!strcmp(prop, "UnitTypesCount")) {
		const std::string unit(arg);
		CUnitType *type = UnitTypeByIdent(unit);
		Assert(type);
		return CPlayer::Players[player]->GetUnitTypeCount(type);
	} else if (!strcmp(prop, "UnitTypesUnderConstructionCount")) {
		const std::string unit(arg);
		CUnitType *type = UnitTypeByIdent(unit);
		Assert(type);
		return CPlayer::Players[player]->GetUnitTypeUnderConstructionCount(type);
	} else if (!strcmp(prop, "UnitTypesAiActiveCount")) {
		const std::string unit(arg);
		CUnitType *type = UnitTypeByIdent(unit);
		Assert(type);
		return CPlayer::Players[player]->GetUnitTypeAiActiveCount(type);
	} else if (!strcmp(prop, "AiEnabled")) {
		return CPlayer::Players[player]->AiEnabled;
	} else if (!strcmp(prop, "TotalNumUnits")) {
		return CPlayer::Players[player]->GetUnitCount();
	} else if (!strcmp(prop, "NumBuildings")) {
		return CPlayer::Players[player]->NumBuildings;
	//Wyrmgus start
	} else if (!strcmp(prop, "NumBuildingsUnderConstruction")) {
		return CPlayer::Players[player]->NumBuildingsUnderConstruction;
	//Wyrmgus end
	} else if (!strcmp(prop, "Supply")) {
		return CPlayer::Players[player]->Supply;
	} else if (!strcmp(prop, "Demand")) {
		return CPlayer::Players[player]->Demand;
	} else if (!strcmp(prop, "UnitLimit")) {
		return CPlayer::Players[player]->UnitLimit;
	} else if (!strcmp(prop, "BuildingLimit")) {
		return CPlayer::Players[player]->BuildingLimit;
	} else if (!strcmp(prop, "TotalUnitLimit")) {
		return CPlayer::Players[player]->TotalUnitLimit;
	} else if (!strcmp(prop, "Score")) {
		return CPlayer::Players[player]->Score;
	} else if (!strcmp(prop, "TotalUnits")) {
		return CPlayer::Players[player]->TotalUnits;
	} else if (!strcmp(prop, "TotalBuildings")) {
		return CPlayer::Players[player]->TotalBuildings;
	} else if (!strcmp(prop, "TotalResources")) {
		const int resId = GetResourceIdByName(arg);
		if (resId == -1) {
			fprintf(stderr, "Invalid resource \"%s\"", arg);
			Exit(1);
		}
		return CPlayer::Players[player]->TotalResources[resId];
	} else if (!strcmp(prop, "TotalRazings")) {
		return CPlayer::Players[player]->TotalRazings;
	} else if (!strcmp(prop, "TotalKills")) {
		return CPlayer::Players[player]->TotalKills;
	} else {
		fprintf(stderr, "Invalid field: %s" _C_ prop);
		Exit(1);
	}
	return 0;
}

/**
**  Sets the player data.
*/
static void SetPlayerData(const int player, const char *prop, const char *arg, int value)
{
	if (!strcmp(prop, "RaceName")) {
		//Wyrmgus start
//		CPlayer::Players[player]->Race = value;
		CPlayer::Players[player]->SetCivilization(value);
		//Wyrmgus end
	} else if (!strcmp(prop, "Resources")) {
		const int resId = GetResourceIdByName(arg);
		if (resId == -1) {
			fprintf(stderr, "Invalid resource \"%s\"", arg);
			Exit(1);
		}
		CPlayer::Players[player]->SetResource(resId, value, STORE_BOTH);
	} else if (!strcmp(prop, "StoredResources")) {
		const int resId = GetResourceIdByName(arg);
		if (resId == -1) {
			fprintf(stderr, "Invalid resource \"%s\"", arg);
			Exit(1);
		}
		CPlayer::Players[player]->SetResource(resId, value, STORE_BUILDING);
	} else if (!strcmp(prop, "UnitLimit")) {
		CPlayer::Players[player]->UnitLimit = value;
	} else if (!strcmp(prop, "BuildingLimit")) {
		CPlayer::Players[player]->BuildingLimit = value;
	} else if (!strcmp(prop, "TotalUnitLimit")) {
		CPlayer::Players[player]->TotalUnitLimit = value;
	} else if (!strcmp(prop, "Score")) {
		CPlayer::Players[player]->Score = value;
	} else if (!strcmp(prop, "TotalUnits")) {
		CPlayer::Players[player]->TotalUnits = value;
	} else if (!strcmp(prop, "TotalBuildings")) {
		CPlayer::Players[player]->TotalBuildings = value;
	} else if (!strcmp(prop, "TotalResources")) {
		const int resId = GetResourceIdByName(arg);
		if (resId == -1) {
			fprintf(stderr, "Invalid resource \"%s\"", arg);
			Exit(1);
		}
		CPlayer::Players[player]->TotalResources[resId] = value;
	} else if (!strcmp(prop, "TotalRazings")) {
		CPlayer::Players[player]->TotalRazings = value;
	} else if (!strcmp(prop, "TotalKills")) {
		CPlayer::Players[player]->TotalKills = value;
	} else {
		fprintf(stderr, "Invalid field: %s" _C_ prop);
		Exit(1);
	}
}


/* virtual */ void CAnimation_SetPlayerVar::Action(CUnit &unit, int &/*move*/, int /*scale*/) const
{
	Assert(unit.Anim.Anim == this);

	const char *var = this->varStr.c_str();
	const char *arg = this->argStr.c_str();
	const int playerId = ParseAnimInt(unit, this->playerStr.c_str());
	int rop = ParseAnimInt(unit, this->valueStr.c_str());
	int data = GetPlayerData(playerId, var, arg);

	switch (this->mod) {
		case modAdd:
			data += rop;
			break;
		case modSub:
			data -= rop;
			break;
		case modMul:
			data *= rop;
			break;
		case modDiv:
			if (!rop) {
				fprintf(stderr, "Division by zero in AnimationSetPlayerVar\n");
				Exit(1);
			}
			data /= rop;
			break;
		case modMod:
			if (!rop) {
				fprintf(stderr, "Division by zero in AnimationSetPlayerVar\n");
				Exit(1);
			}
			data %= rop;
			break;
		case modAnd:
			data &= rop;
			break;
		case modOr:
			data |= rop;
			break;
		case modXor:
			data ^= rop;
			break;
		case modNot:
			data = !data;
			break;
		default:
			data = rop;
	}
	rop = data;
	SetPlayerData(playerId, var, arg, rop);
}

/*
**  s = "player var mod value [arg2]"
*/
/* virtual */ void CAnimation_SetPlayerVar::Init(const char *s, lua_State *)
{
	const std::string str(s);
	const size_t len = str.size();

	size_t begin = 0;
	size_t end = str.find(' ', begin);
	this->playerStr.assign(str, begin, end - begin);

	begin = std::min(len, str.find_first_not_of(' ', end));
	end = std::min(len, str.find(' ', begin));
	this->varStr.assign(str, begin, end - begin);

	begin = std::min(len, str.find_first_not_of(' ', end));
	end = std::min(len, str.find(' ', begin));
	const std::string modStr(str, begin, end - begin);
	if (modStr == "=") {
		this->mod = modSet;
	} else if (modStr == "+=") {
		this->mod = modAdd;
	} else if (modStr == "-=") {
		this->mod = modSub;
	} else if (modStr == "*=") {
		this->mod = modMul;
	} else if (modStr == "/=") {
		this->mod = modDiv;
	} else if (modStr == "%=") {
		this->mod = modMod;
	} else if (modStr == "&=") {
		this->mod = modAnd;
	} else if (modStr == "|=") {
		this->mod = modOr;
	} else if (modStr == "^=") {
		this->mod = modXor;
	} else if (modStr == "!") {
		this->mod = modNot;
	} else {
		this->mod = (SetVar_ModifyTypes)(atoi(modStr.c_str()));
	}

	begin = std::min(len, str.find_first_not_of(' ', end));
	end = std::min(len, str.find(' ', begin));
	this->valueStr.assign(str, begin, end - begin);

	begin = std::min(len, str.find_first_not_of(' ', end));
	end = std::min(len, str.find(' ', begin));
	this->argStr.assign(str, begin, end - begin);
}
