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
/**@name script_ai.cpp - The AI ccl functions. */
//
//      (c) Copyright 2000-2019 by Lutz Sammer, Ludovic Pollet,
//      Jimmy Salmon and Andrettin
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

#include "ai/ai.h"
#include "ai_local.h"

#include "civilization.h"
#include "map/map.h"
#include "map/site.h"
#include "pathfinder/pathfinder.h"
#include "player.h"
#include "script.h"
#include "ui/button_action.h"
#include "ui/interface.h"
#include "unit/unit.h"
#include "unit/unit_class.h"
#include "unit/unit_manager.h"
#include "unit/unit_type.h"
#include "upgrade/upgrade.h"

/**
**  Insert new unit-type element.
**
**  @param table  Table with elements.
**  @param n      Index to insert new into table
**  @param base   Base type to insert into table.
*/
static void AiHelperInsert(std::vector<std::vector<CUnitType *> > &table,
						   unsigned int n, CUnitType &base)
{
	if (n >= table.size()) {
		table.resize(n + 1);
	}
	// Look if already known
	std::vector<CUnitType *>::const_iterator it = std::find(table[n].begin(), table[n].end(), &base);
	if (it != table[n].end()) {
		return;
	}
	table[n].push_back(&base);
}

//Wyrmgus start
/**
**  Insert new upgrade element.
**
**  @param table  Table with elements.
**  @param n      Index to insert new into table
**  @param base   Base type to insert into table.
*/
static void AiHelperInsert(std::vector<std::vector<const CUpgrade *> > &table,
						   unsigned int n, const CUpgrade &base)
{
	if (n >= table.size()) {
		table.resize(n + 1);
	}
	// Look if already known
	std::vector<const CUpgrade *>::const_iterator it = std::find(table[n].begin(), table[n].end(), &base);
	if (it != table[n].end()) {
		return;
	}
	table[n].push_back(&base);
}

/**
**  Insert new index element.
**
**  @param table  Table with elements.
**  @param n      Index to insert new into table
**  @param base   Base index to insert into table.
*/
static void AiHelperInsert(std::vector<std::vector<int> > &table,
						   unsigned int n, int base)
{
	if (n >= table.size()) {
		table.resize(n + 1);
	}
	// Look if already known
	std::vector<int>::const_iterator it = std::find(table[n].begin(), table[n].end(), base);
	if (it != table[n].end()) {
		return;
	}
	table[n].push_back(base);
}
//Wyrmgus end

/**
**  Transform list of unit separed with coma to a true list.
*/
static std::vector<CUnitType *> getUnitTypeFromString(const std::string &list)
{
	std::vector<CUnitType *> res;

	if (list == "*") {
		return CUnitType::GetAll();
	}
	size_t begin = 1;
	size_t end = list.find(",", begin);
	while (end != std::string::npos) {
		std::string unitName = list.substr(begin, end - begin);
		begin = end + 1;
		end = list.find(",", begin);
		if (!unitName.empty()) {
			Assert(unitName[0] != ',');
			res.push_back(CUnitType::Get(unitName.c_str(), false));
		}
	}
	return res;
}

/**
**  Get list of unittype which can be repared.
*/
static std::vector<CUnitType *> getReparableUnits()
{
	std::vector<CUnitType *> res;

	for (CUnitType *unit_type : CUnitType::GetAll()) {
		if (unit_type->GetRepairHP() > 0) {
			res.push_back(unit_type);
		}
	}
	return res;
}

/**
**  Get sorted list of unittype with Supply not null.
**
**  @note Better (supply / cost) first.
*/
static std::vector<CUnitType *> getSupplyUnits()
{
	std::vector<CUnitType *> res;
	std::vector<CUnitType *> sorted_res;

	for (CUnitType *unit_type : CUnitType::GetAll()) {
		if (unit_type->BoolFlag[TOWNHALL_INDEX].value) {
			continue;
		}

		if (unit_type->DefaultStat.Variables[SUPPLY_INDEX].Value > 0) { //supply units are identified as being those with a default stat supply of 1 or more; so if a unit has a supply default stat of 0, but through an upgrade ends up having 1 or more supply, it won't be included here
			res.push_back(unit_type);
		}
	}
	// Now, sort them, best first.
	while (!res.empty()) {
		float bestscore = 0;
		CUnitType *besttype = nullptr;

		for (std::vector<CUnitType *>::const_iterator i = res.begin(); i != res.end(); ++i) {
			CUnitType &type = **i;
			unsigned int cost = 0;

			for (size_t j = 0; j < CResource::GetAll().size(); ++j) {
				cost += type.DefaultStat.Costs[j]; //this cannot be MapDefaultStat because this function is called when the AiHelper is defined, rather than when a game is started
			}
			const float score = ((float) type.DefaultStat.Variables[SUPPLY_INDEX].Value) / cost;
			if (score > bestscore) {
				bestscore = score;
				besttype = &type;
			}
		}
		sorted_res.push_back(besttype);
		for (std::vector<CUnitType *>::iterator i = res.begin(); i != res.end(); ++i) {
			if (*i == besttype) {
				i = res.erase(i);
				break;
			}
		}
	}
	return sorted_res;
}

/**
**  Get sorted list of unittype with CanHarvest not null.
**
**  @note Better (MaxOnBoard / cost) first.
*/
static std::vector<CUnitType *> getMineUnits()
{
	std::vector<CUnitType *> res;

	for (CUnitType *unit_type : CUnitType::GetAll()) {
		if (unit_type->GivesResource > 0 && unit_type->BoolFlag[CANHARVEST_INDEX].value) {
			res.push_back(unit_type);
		}
	}
#if 0
	std::vector<CUnitType *> sorted_res;
	// Now, sort them, best first.
	while (!res.empty()) {
		float bestscore;
		CUnitType *besttype;

		bestscore = 0;
		for (std::vector<CUnitType *>::const_iterator i = res.begin(); i != res.end(); ++i) {
			CUnitType *type = *i;
			float score;
			unsigned int cost = 0;

			for (size_t j = 0; j < CResource::GetAll().size(); ++j) {
				cost += type->_Costs[j];
			}
			score = ((float) type->MaxOnBoard) / cost;
			if (score > bestscore) {
				bestscore = score;
				besttype = type;
			}
		}
		sorted_res.push_back(besttype);
		for (std::vector<CUnitType *>::iterator i = res.begin(); i != res.end(); ++i) {
			if (*i == besttype) {
				i = res.erase(i);
				break;
			}
		}
	}
	return sorted_res;
#else
	return res;
#endif

}

//Wyrmgus start
/**
**  Get list of unit types which are markets.
*/
static std::vector<CUnitType *> GetMarketUnits()
{
	std::vector<CUnitType *> res;

	for (CUnitType *unit_type : CUnitType::GetAll()) {
		if (unit_type->BoolFlag[MARKET_INDEX].value) {
			res.push_back(unit_type);
		}
	}
	return res;
}

/**
**  Get list of unit types which are transporters.
*/
static std::vector<CUnitType *> GetNavalTransporterUnits()
{
	std::vector<CUnitType *> res;

	for (CUnitType *unit_type : CUnitType::GetAll()) {
		if (unit_type->CanTransport() && (unit_type->UnitType == UnitTypeNaval || unit_type->UnitType == UnitTypeFly || unit_type->UnitType == UnitTypeFlyLow)) { //if the unit is a transporter that can travel through water (not necessarily a ship, can also fly)
			res.push_back(unit_type);
		}
	}
	return res;
}
//Wyrmgus end

/**
**  Init AiHelper.
**
**  @param aiHelper  variable to initialise.
**
**  @todo missing Equiv initialisation.
*/
static void InitAiHelper(AiHelper &aiHelper)
{
	//Wyrmgus start
	//free AI helper (except for equivs) before initializing, in case this is a re-definition
	AiHelpers.Train.clear();
	AiHelpers.Build.clear();
	AiHelpers.Upgrade.clear();
	AiHelpers.Research.clear();
	AiHelpers.Repair.clear();
	AiHelpers.UnitLimit.clear();
	AiHelpers.Mines.clear();
	AiHelpers.Depots.clear();
	AiHelpers.SellMarkets.clear();
	AiHelpers.BuyMarkets.clear();
	AiHelpers.ProducedResources.clear();
	AiHelpers.ResearchedUpgrades.clear();
	AiHelpers.UpgradesTo.clear();
	AiHelpers.ExperienceUpgrades.clear();
	AiHelpers.LearnableAbilities.clear();
	AiHelpers.NavalTransporters.clear();
	//Wyrmgus end
	
	extern std::vector<ButtonAction *> UnitButtonTable;

	std::vector<CUnitType *> reparableUnits = getReparableUnits();
	std::vector<CUnitType *> supplyUnits = getSupplyUnits();
	std::vector<CUnitType *> mineUnits = getMineUnits();
	//Wyrmgus start
	std::vector<CUnitType *> market_units = GetMarketUnits();
	std::vector<CUnitType *> naval_transporter_units = GetNavalTransporterUnits();
	//Wyrmgus end

	for (std::vector<CUnitType *>::const_iterator i = supplyUnits.begin(); i != supplyUnits.end(); ++i) {
		AiHelperInsert(aiHelper.UnitLimit, 0, **i);
	}
	//Wyrmgus start
	for (std::vector<CUnitType *>::const_iterator i = market_units.begin(); i != market_units.end(); ++i) {
		AiHelperInsert(aiHelper.SellMarkets, 0, **i);
	}
	for (std::vector<CUnitType *>::const_iterator i = naval_transporter_units.begin(); i != naval_transporter_units.end(); ++i) {
		AiHelperInsert(aiHelper.NavalTransporters, 0, **i);
	}
	//Wyrmgus end

	for (int i = 1; i < MaxCosts; ++i) {
		for (std::vector<CUnitType *>::const_iterator j = mineUnits.begin(); j != mineUnits.end(); ++j) {
			if ((*j)->GivesResource == i) {
				AiHelperInsert(aiHelper.Mines, i, **j);
			}
		}
		for (CUnitType *unit_type : CUnitType::GetAll()) {
			if (unit_type->CanStore[i] > 0) {
				AiHelperInsert(aiHelper.Depots, i, *unit_type);
			}
		}
	}

	for (size_t i = 0; i != UnitButtonTable.size(); ++i) {
		const ButtonAction &button = *UnitButtonTable[i];
		std::vector<CUnitType *> unitmask = getUnitTypeFromString(button.UnitMask);
		
		//add the unit types belonging to the unit classes to which the button is available for as well
		for (const UnitClass *unit_class : button.GetForUnitClasses()) {
			for (CUnitType *unit_type : unit_class->GetUnitTypes()) {
				unitmask.push_back(unit_type);
			}
		}

		switch (button.GetAction()) {
			case ButtonRepair :
				for (std::vector<CUnitType *>::const_iterator j = unitmask.begin(); j != unitmask.end(); ++j) {
					for (std::vector<CUnitType *>::const_iterator k = reparableUnits.begin(); k != reparableUnits.end(); ++k) {
						AiHelperInsert(aiHelper.Repair, (*k)->GetIndex(), **j);
					}
				}
				break;
			case ButtonBuild: {
				CUnitType *buildingType = CUnitType::Get(button.ValueStr);

				for (std::vector<CUnitType *>::const_iterator j = unitmask.begin(); j != unitmask.end(); ++j) {
					AiHelperInsert(aiHelper.Build, buildingType->GetIndex(), (**j));
				}
				break;
			}
			case ButtonTrain : {
				CUnitType *trainingType = CUnitType::Get(button.ValueStr);

				for (std::vector<CUnitType *>::const_iterator j = unitmask.begin(); j != unitmask.end(); ++j) {
					AiHelperInsert(aiHelper.Train, trainingType->GetIndex(), (**j));
				}
				break;
			}
			case ButtonUpgradeTo : {
				CUnitType *upgradeToType = CUnitType::Get(button.ValueStr);

				for (std::vector<CUnitType *>::const_iterator j = unitmask.begin(); j != unitmask.end(); ++j) {
					AiHelperInsert(aiHelper.Upgrade, upgradeToType->GetIndex(), **j);
				}
				
				for (std::vector<CUnitType *>::const_iterator j = unitmask.begin(); j != unitmask.end(); ++j) {
					AiHelperInsert(aiHelper.UpgradesTo, (**j).GetIndex(), *upgradeToType);
				}
				break;
			}
			case ButtonResearch : {
				const CUpgrade *research_upgrade = CUpgrade::Get(button.ValueStr);

				for (std::vector<CUnitType *>::const_iterator j = unitmask.begin(); j != unitmask.end(); ++j) {
					AiHelperInsert(aiHelper.Research, research_upgrade->GetIndex(), **j);
				}
				
				for (std::vector<CUnitType *>::const_iterator j = unitmask.begin(); j != unitmask.end(); ++j) {
					AiHelperInsert(aiHelper.ResearchedUpgrades, (**j).GetIndex(), *research_upgrade);
				}
				break;
			}
			//Wyrmgus start
			case ButtonSellResource : {
				int resource = GetResourceIdByName(button.ValueStr.c_str());

				for (std::vector<CUnitType *>::const_iterator j = unitmask.begin(); j != unitmask.end(); ++j) {
					AiHelperInsert(aiHelper.SellMarkets, resource - 1, **j);
				}
				break;
			}
			case ButtonBuyResource : {
				int resource = GetResourceIdByName(button.ValueStr.c_str());

				for (std::vector<CUnitType *>::const_iterator j = unitmask.begin(); j != unitmask.end(); ++j) {
					AiHelperInsert(aiHelper.BuyMarkets, resource - 1, **j);
				}
				break;
			}
			case ButtonProduceResource : {
				int resource = GetResourceIdByName(button.ValueStr.c_str());

				for (std::vector<CUnitType *>::const_iterator j = unitmask.begin(); j != unitmask.end(); ++j) {
					AiHelperInsert(aiHelper.ProducedResources, (**j).GetIndex(), resource);
				}
				break;
			}
			case ButtonExperienceUpgradeTo : {
				CUnitType *upgradeToType = CUnitType::Get(button.ValueStr);

				for (std::vector<CUnitType *>::const_iterator j = unitmask.begin(); j != unitmask.end(); ++j) {
					AiHelperInsert(aiHelper.ExperienceUpgrades, (**j).GetIndex(), *upgradeToType);
				}
				break;
			}
			case ButtonLearnAbility : {
				const CUpgrade *ability = CUpgrade::Get(button.ValueStr);

				if (ability->IsAbility()) {
					for (std::vector<CUnitType *>::const_iterator j = unitmask.begin(); j != unitmask.end(); ++j) {
						AiHelperInsert(aiHelper.LearnableAbilities, (**j).GetIndex(), *ability);
					}
				}
				break;
			}
			//Wyrmgus end
			default:
				break;
		}
	}
}

/**
**  Define helper for AI.
**
**  @param l  Lua state.
**
**  @todo  FIXME: the first unit could be a list see ../doc/ccl/ai.html
*/
static int CclDefineAiHelper(lua_State *l)
{
	InitAiHelper(AiHelpers);

	const int args = lua_gettop(l);
	for (int j = 0; j < args; ++j) {
		if (!lua_istable(l, j + 1)) {
			LuaError(l, "incorrect argument");
		}
		const int subargs = lua_rawlen(l, j + 1);
		const char *value = LuaToString(l, j + 1, 1);
		if (!strcmp(value, "build")
			|| !strcmp(value, "train")
			|| !strcmp(value, "upgrade")
			|| !strcmp(value, "research")
			|| !strcmp(value, "unit-limit")
			//Wyrmgus start
//			|| !strcmp(value, "repair")) {
			|| !strcmp(value, "repair")
			|| !strcmp(value, "experience-upgrade")
			|| !strcmp(value, "learnable-ability")
		) {
			//Wyrmgus end
#ifdef DEBUG
			fprintf(stderr, "DefineAiHelper: Relation is computed from buttons, you may remove safely the block beginning with '\"%s\"'\n", value);
#endif
			continue;
		} else if (!strcmp(value, "unit-equiv")) {
		} else {
			LuaError(l, "unknown tag: %s" _C_ value);
		}
		// Get the base unit type, which could handle the action.
		const char *baseTypeName = LuaToString(l, j + 1, 2);
		const CUnitType *base = CUnitType::Get(baseTypeName);
		if (!base) {
			LuaError(l, "unknown unittype: %s" _C_ baseTypeName);
		}

		// Get the unit types, which could be produced
		for (int k = 2; k < subargs; ++k) {
			const char *equivTypeName = LuaToString(l, j + 1, k + 1);
			CUnitType *type = CUnitType::Get(equivTypeName);
			if (!type) {
				LuaError(l, "unknown unittype: %s" _C_ equivTypeName);
			}
			AiHelperInsert(AiHelpers.Equiv, base->GetIndex(), *type);
			AiNewUnitTypeEquiv(*base, *type);
		}
	}
	return 0;
}

static CAiType *GetAiTypesByName(const char *name)
{
	for (size_t i = 0; i < AiTypes.size(); ++i) {
		CAiType *ait = AiTypes[i];
		if (ait->Name == name) {
			return ait;
		}
	}
	return nullptr;
}

/**
**  Define an AI engine.
**
**  @param l  Lua state.
**
**  @return   FIXME: docu
*/
static int CclDefineAi(lua_State *l)
{
	LuaCheckArgs(l, 4);
	if (!lua_isfunction(l, 4)) {
		LuaError(l, "incorrect argument");
	}

	CAiType *aitype = new CAiType;

	// AI Name
	const char *aiName = LuaToString(l, 1);
	aitype->Name = aiName;

#ifdef DEBUG
	if (GetAiTypesByName(aiName)) {
		DebugPrint("Warning two or more AI's with the same name '%s'\n" _C_ aiName);
	}
#endif
	AiTypes.insert(AiTypes.begin(), aitype);

	// AI Race
	const char *value = LuaToString(l, 2);
	if (*value != '*') {
		aitype->Race = value;
	} else {
		aitype->Race.clear();
	}

	// AI Class
	aitype->Class = LuaToString(l, 3);

	// AI Script
	lua_getglobal(l, "_ai_scripts_");
	if (lua_isnil(l, -1)) {
		lua_pop(l, 1);
		lua_newtable(l);
		lua_setglobal(l, "_ai_scripts_");
		lua_getglobal(l, "_ai_scripts_");
	}
	aitype->Script = aitype->Name + aitype->Race + aitype->Class;
	lua_pushstring(l, aitype->Script.c_str());
	lua_pushvalue(l, 4);
	lua_rawset(l, 5);
	lua_pop(l, 1);

	return 0;
}

/*----------------------------------------------------------------------------
--  AI script functions
----------------------------------------------------------------------------*/

/**
**  Append unit-type to request table.
**
**  @param type   Unit-type to be appended.
**  @param count  How many unit-types to build.
*/
static void InsertUnitTypeRequests(CUnitType *type, int count)
{
	AiRequestType ait;

	ait.Type = type;
	ait.Count = count;

	AiPlayer->UnitTypeRequests.push_back(ait);
}

/**
**  Find unit-type in request table.
**
**  @param type  Unit-type to be found.
*/
static AiRequestType *FindInUnitTypeRequests(const CUnitType *type)
{
	const size_t n = AiPlayer->UnitTypeRequests.size();

	for (size_t i = 0; i < n; ++i) {
		if (AiPlayer->UnitTypeRequests[i].Type == type) {
			return &AiPlayer->UnitTypeRequests[i];
		}
	}
	return nullptr;
}

//Wyrmgus start
/**
**  Remove unit-type in request table.
**
**  @param type  Unit-type to be removed.
*/
static void RemoveFromUnitTypeRequests(const CUnitType *type)
{
	std::vector<AiRequestType>::iterator i;

	for (i = AiPlayer->UnitTypeRequests.begin(); i != AiPlayer->UnitTypeRequests.end(); ++i) {
		if (type == (*i).Type) {
			AiPlayer->UnitTypeRequests.erase(i);
			break;
		}
	}
}
//Wyrmgus end

/**
**  Find unit-type in upgrade-to table.
**
**  @param type  Unit-type to be found.
*/
static int FindInUpgradeToRequests(const CUnitType *type)
{
	const size_t n = AiPlayer->UpgradeToRequests.size();
	for (size_t i = 0; i < n; ++i) {
		if (AiPlayer->UpgradeToRequests[i] == type) {
			return 1;
		}
	}
	return 0;
}

/**
**  Append unit-type to request table.
**
**  @param type  Unit-type to be appended.
*/
static void InsertUpgradeToRequests(CUnitType *type)
{
	AiPlayer->UpgradeToRequests.push_back(type);
}

/**
**  Append unit-type to request table.
**
**  @param upgrade  Upgrade to be appended.
*/
static void InsertResearchRequests(const CUpgrade *upgrade)
{
	AiPlayer->ResearchRequests.push_back(upgrade);
}

//----------------------------------------------------------------------------

/**
**  Get the race of the current AI player.
**
**  @param l  Lua state.
*/
static int CclAiGetRace(lua_State *l)
{
	LuaCheckArgs(l, 0);
	const CCivilization *civilization = CCivilization::Get(AiPlayer->Player->Race);
	if (civilization != nullptr) {
		lua_pushstring(l, civilization->GetIdent().utf8().get_data());
	} else {
		lua_pushstring(l, "");
	}
	return 1;
}

/**
**  Get the number of cycles to sleep.
**
**  @param l  Lua state
**
**  @return   Number of return values
*/
static int CclAiGetSleepCycles(lua_State *l)
{
	LuaCheckArgs(l, 0);
	lua_pushnumber(l, AiSleepCycles);
	return 1;
}

//----------------------------------------------------------------------------

/**
**  Set debugging flag of AI script
**
**  @param l  Lua state
**
**  @return   Number of return values
*/
static int CclAiDebug(lua_State *l)
{
	LuaCheckArgs(l, 1);
	AiPlayer->ScriptDebug = LuaToBoolean(l, 1);
	return 0;
}

/**
**  Activate AI debugging for the given player(s)
**  Player can be a number for a specific player
**    "self" for current human player (ai me)
**    "none" to disable
**
**  @param l  Lua State
**
**  @return   Number of return values
*/
static int CclAiDebugPlayer(lua_State *l)
{
	const int args = lua_gettop(l);
	for (int j = 0; j < args; ++j) {
		const char *item;

		if (lua_isstring(l, j + 1)) {
			item = LuaToString(l, j + 1);
		} else {
			item = nullptr;
		}
		if (item && !strcmp(item, "none")) {
			for (int i = 0; i != NumPlayers; ++i) {
				if (!CPlayer::Players[i]->AiEnabled || !CPlayer::Players[i]->Ai) {
					continue;
				}
				CPlayer::Players[i]->Ai->ScriptDebug = 0;
			}
		} else {
			int playerid;
			if (item && !strcmp(item, "self")) {
				if (!CPlayer::GetThisPlayer()) {
					continue;
				}
				playerid = CPlayer::GetThisPlayer()->GetIndex();
			} else {
				playerid = LuaToNumber(l, j + 1);
			}

			if (!CPlayer::Players[playerid]->AiEnabled || !CPlayer::Players[playerid]->Ai) {
				continue;
			}
			CPlayer::Players[playerid]->Ai->ScriptDebug = 1;
		}
	}
	return 0;
}

/**
**  Need a unit.
**
**  @param l  Lua state.
**
**  @return   Number of return values
*/
static int CclAiNeed(lua_State *l)
{
	LuaCheckArgs(l, 1);
	//Wyrmgus start
	if (lua_isnil(l, 1)) {
		lua_pushboolean(l, 0);
		return 1;
	}
	//Wyrmgus end
	
	InsertUnitTypeRequests(CclGetUnitType(l), 1);

	lua_pushboolean(l, 0);
	return 1;
}

/**
**  Set the number of units.
**
**  @param l  Lua state
**
**  @return   Number of return values
*/
static int CclAiSet(lua_State *l)
{
	LuaCheckArgs(l, 2);
	//Wyrmgus start
	if (lua_isnil(l, 1)) {
		lua_pushboolean(l, 0);
		return 1;
	}
	//Wyrmgus end
	lua_pushvalue(l, 1);
	CUnitType *type = CclGetUnitType(l);
	lua_pop(l, 1);

	AiRequestType *autt = FindInUnitTypeRequests(type);
	//Wyrmgus start
	int set_count = LuaToNumber(l, 2);
//	if (autt) {
	if (autt && set_count <= 0) {
		RemoveFromUnitTypeRequests(type);
	} else if (autt) {
	//Wyrmgus end
		autt->Count = LuaToNumber(l, 2);
		// FIXME: 0 should remove it.
	//Wyrmgus start
//	} else {
	} else if (set_count > 0) {
	//Wyrmgus end
		InsertUnitTypeRequests(type, LuaToNumber(l, 2));
	}
	lua_pushboolean(l, 0);
	return 1;
}

/**
**  Wait for a unit.
**
**  @param l  Lua State.
**
**  @return   Number of return values
*/
static int CclAiWait(lua_State *l)
{
	LuaCheckArgs(l, 1);
	//Wyrmgus start
	if (lua_isnil(l, 1)) {
		lua_pushboolean(l, 0);
		return 1;
	}
	//Wyrmgus end
	//Wyrmgus start
	/*
	const CUnitType *type = CclGetUnitType(l);
	const int *unit_types_count = AiPlayer->Player->UnitTypesAiActiveCount;
	const AiRequestType *autt = FindInUnitTypeRequests(type);
	if (!autt) {
		// Look if we have this unit-type.
		if (unit_types_count[type->GetIndex()]) {
			lua_pushboolean(l, 0);
			return 1;
		}

		// Look if we have equivalent unit-types.
		if (type->GetIndex() < (int) AiHelpers.Equiv.size()) {
			for (size_t j = 0; j < AiHelpers.Equiv[type->GetIndex()].size(); ++j) {
				if (unit_types_count[AiHelpers.Equiv[type->GetIndex()][j]->Slot]) {
					lua_pushboolean(l, 0);
					return 1;
				}
			}
		}
		// Look if we have an upgrade-to request.
		if (FindInUpgradeToRequests(type)) {
			lua_pushboolean(l, 1);
			return 1;
		}
		DebugPrint("Broken? waiting on %s which wasn't requested.\n" _C_ type->Ident.c_str());
		lua_pushboolean(l, 0);
		return 1;
	}
	//
	// Add equivalent units
	//
	unsigned int n = unit_types_count[type->GetIndex()];
	if (type->GetIndex() < (int)AiHelpers.Equiv.size()) {
		for (size_t j = 0; j < AiHelpers.Equiv[type->GetIndex()].size(); ++j) {
			n += unit_types_count[AiHelpers.Equiv[type->GetIndex()][j]->Slot];
		}
	}
	// units available?
	if (n >= autt->Count) {
		lua_pushboolean(l, 0);
		return 1;
	}
	*/
	const char *ident = LuaToString(l, 1);
	if (!strncmp(ident, "unit-", 5)) {
		const CUnitType *type = CUnitType::Get(ident);
		const AiRequestType *autt = FindInUnitTypeRequests(type);
		if (!autt) {
			// Look if we have this unit-type.
			if (AiPlayer->Player->GetUnitTypeAiActiveCount(type)) {
				lua_pushboolean(l, 0);
				return 1;
			}

			// Look if we have equivalent unit-types.
			if (type->GetIndex() < (int)AiHelpers.Equiv.size()) {
				for (size_t j = 0; j < AiHelpers.Equiv[type->GetIndex()].size(); ++j) {
					if (AiPlayer->Player->GetUnitTypeAiActiveCount(AiHelpers.Equiv[type->GetIndex()][j])) {
						lua_pushboolean(l, 0);
						return 1;
					}
				}
			}
			// Look if we have an upgrade-to request.
			if (FindInUpgradeToRequests(type)) {
				lua_pushboolean(l, 1);
				return 1;
			}
			lua_pushboolean(l, 1);
			return 1;
		}
		//
		// Add equivalent units
		//
		unsigned int n = AiPlayer->Player->GetUnitTypeAiActiveCount(type);
		if (type->GetIndex() < (int)AiHelpers.Equiv.size()) {
			for (size_t j = 0; j < AiHelpers.Equiv[type->GetIndex()].size(); ++j) {
				n += AiPlayer->Player->GetUnitTypeAiActiveCount(AiHelpers.Equiv[type->GetIndex()][j]);
			}
		}
		// units available?
		if (n >= autt->Count) {
			lua_pushboolean(l, 0);
			return 1;
		}
	} else if (!strncmp(ident, "upgrade-", 8)) {
		CUpgrade *upgrade = CUpgrade::Get(ident);
		if (UpgradeIdentAllowed(*AiPlayer->Player, ident) == 'R') {
			lua_pushboolean(l, 0);
			return 1;
		}
	}
	//Wyrmgus end
	lua_pushboolean(l, 1);
	return 1;
}

/**
**  Define a force, a groups of units.
**
**  @param l  Lua state.
*/
static int CclAiForce(lua_State *l)
{
	bool resetForce = false;
	const int arg = lua_gettop(l);
	Assert(0 < arg && arg <= 3);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument");
	}
	if (arg == 3) {
		resetForce = LuaToBoolean(l, 3);
	}
	int force = LuaToNumber(l, 1);
	if (force < 0 || force >= AI_MAX_FORCE_INTERNAL) {
		LuaError(l, "Force out of range: %d" _C_ force);
	}
	AiForce &aiforce = AiPlayer->Force[AiPlayer->Force.getScriptForce(force)];
	if (resetForce) {
		aiforce.Reset(true);
	}
	AiForceRole role = aiforce.Role;
	aiforce.State = AiForceAttackingState_Waiting;
	aiforce.Role = role;

	int args = lua_rawlen(l, 2);
	for (int j = 0; j < args; ++j) {
		lua_rawgeti(l, 2, j + 1);
		CUnitType *type = CclGetUnitType(l);
		lua_pop(l, 1);
		++j;
		int count = LuaToNumber(l, 2, j + 1);

		if (!type) { // bulletproof
			continue;
		}

		//use the equivalent unit type
		type = CUnitType::Get(UnitTypeEquivs[type->GetIndex()]);

		if (resetForce) {
			// Append it.
			AiUnitType newaiut;
			newaiut.Want = count;
			newaiut.Type = type;
			aiforce.UnitTypes.push_back(newaiut);
		} else {
			// Look if already in force.
			size_t i;
			for (i = 0; i < aiforce.UnitTypes.size(); ++i) {
				AiUnitType *aiut = &aiforce.UnitTypes[i];
				if (aiut->Type == type) { // found
					if (count) {
						aiut->Want = count;
					} else {
						aiforce.UnitTypes.erase(aiforce.UnitTypes.begin() + i);
					}
					break;
				}
			}
			// New type append it.
			if (i == aiforce.UnitTypes.size()) {
				AiUnitType newaiut;
				newaiut.Want = count;
				newaiut.Type = type;
				aiforce.UnitTypes.push_back(newaiut);
			}
		}

	}
	AiAssignFreeUnitsToForce(AiPlayer->Force.getScriptForce(force));
	lua_pushboolean(l, 0);
	return 1;
}

/**
**  Define the role of a force.
**
**  @param l  Lua state.
*/
static int CclAiForceRole(lua_State *l)
{
	LuaCheckArgs(l, 2);
	int force = LuaToNumber(l, 1);
	if (force < 0 || force >= AI_MAX_FORCE_INTERNAL) {
		LuaError(l, "Force %i out of range" _C_ force);
	}

	AiForce &aiforce = AiPlayer->Force[AiPlayer->Force.getScriptForce(force)];

	const char *flag = LuaToString(l, 2);
	if (!strcmp(flag, "attack")) {
		aiforce.Role = AiForceRoleAttack;
	} else if (!strcmp(flag, "defend")) {
		aiforce.Role = AiForceRoleDefend;
	} else {
		LuaError(l, "Unknown force role '%s'" _C_ flag);
	}
	lua_pushboolean(l, 0);
	return 1;
}

/**
**  Release force.
**
**  @param l  Lua state.
*/
static int CclAiReleaseForce(lua_State *l)
{
	LuaCheckArgs(l, 1);
	int force = LuaToNumber(l, 1);
	if (force < 0 || force >= AI_MAX_FORCE_INTERNAL) {
		LuaError(l, "Force out of range: %d" _C_ force);
	}
	for (int i = AI_MAX_FORCE_INTERNAL; i < AI_MAX_FORCES; ++i) {
		if (AiPlayer->Force[i].FormerForce != -1 && AiPlayer->Force[i].FormerForce == force) {
			while (AiPlayer->Force[i].Size()) {
				CUnit &aiunit = *AiPlayer->Force[i].Units[AiPlayer->Force[i].Size() - 1];
				aiunit.GroupId = 0;
				AiPlayer->Force[i].Units.Remove(&aiunit);
			}
			AiPlayer->Force[i].Reset(false);
		}
	}

	lua_pushboolean(l, 0);
	return 1;
}

/**
**  Check if a force ready.
**
**  @param l  Lua state.
*/
static int CclAiCheckForce(lua_State *l)
{
	LuaCheckArgs(l, 1);
	int force = LuaToNumber(l, 1);
	if (force < 0 || force >= AI_MAX_FORCE_INTERNAL) {
		lua_pushfstring(l, "Force out of range: %d", force);
	}
	if (AiPlayer->Force[AiPlayer->Force.getScriptForce(force)].Completed) {
		lua_pushboolean(l, 1);
		return 1;
	}
	lua_pushboolean(l, 0);
	return 1;
}

/**
**  Wait for a force ready.
**
**  @param l  Lua state.
*/
static int CclAiWaitForce(lua_State *l)
{
	LuaCheckArgs(l, 1);
	int force = LuaToNumber(l, 1);
	if (force < 0 || force >= AI_MAX_FORCE_INTERNAL) {
		lua_pushfstring(l, "Force out of range: %d", force);
	}
	if (AiPlayer->Force[AiPlayer->Force.getScriptForce(force)].Completed) {
		lua_pushboolean(l, 0);
		return 1;
	}
#if 0
	// Debuging
	AiRemoveDeadUnitInForces();
	Assert(!AiPlayer->Force.getScriptForce(f).Completed);
#endif
	lua_pushboolean(l, 1);
	return 1;
}

/**
**  Attack with force.
**
**  @param l  Lua state.
*/
static int CclAiAttackWithForce(lua_State *l)
{
	LuaCheckArgs(l, 1);
	int force = LuaToNumber(l, 1);
	if (force < 0 || force >= AI_MAX_FORCE_INTERNAL) {
		LuaError(l, "Force out of range: %d" _C_ force);
	}
	AiAttackWithForce(force);
	lua_pushboolean(l, 0);
	return 1;
}

/**
**  Wait for a forces ready.
**
**  @param l  Lua state.
*/
static int CclAiWaitForces(lua_State *l)
{
	LuaCheckArgs(l, 1);
	if (!lua_istable(l, 1)) {
		LuaError(l, "incorrect argument");
	}
	const int args = lua_rawlen(l, 1);
	for (int i = 0; i < args; ++i) {
		const int force = LuaToNumber(l, 1, i + 1);

		if (force < 0 || force >= AI_MAX_FORCE_INTERNAL) {
			lua_pushfstring(l, "Force out of range: %d", force);
		}
		if (!AiPlayer->Force[AiPlayer->Force.getScriptForce(force)].Completed) {
			lua_pushboolean(l, 1);
			return 1;
		}
	}
	lua_pushboolean(l, 0);
	return 1;
}

/**
**  Attack with forces.
**
**  @param l  Lua state.
*/
static int CclAiAttackWithForces(lua_State *l)
{
	int Forces[AI_MAX_FORCE_INTERNAL + 1];

	LuaCheckArgs(l, 1);
	if (!lua_istable(l, 1)) {
		LuaError(l, "incorrect argument");
	}
	int args = lua_rawlen(l, 1);
	for (int i = 0; i < args; ++i) {
		const int force = LuaToNumber(l, 1, i + 1);

		if (force < 0 || force >= AI_MAX_FORCE_INTERNAL) {
			lua_pushfstring(l, "Force out of range: %d", force);
		}
		Forces[i] = AiPlayer->Force.getScriptForce(force);
	}
	Forces[args] = -1;
	AiAttackWithForces(Forces);
	lua_pushboolean(l, 0);
	return 1;
}

/**
**  Sleep n cycles.
**
**  @param l  Lua state.
*/
static int CclAiSleep(lua_State *l)
{
	LuaCheckArgs(l, 1);
	int i = LuaToNumber(l, 1);
	if (AiPlayer->SleepCycles || i == 0) {
		if (AiPlayer->SleepCycles < GameCycle) {
			AiPlayer->SleepCycles = 0;
			lua_pushboolean(l, 0);
			return 1;
		}
	} else {
		AiPlayer->SleepCycles = GameCycle + i;
	}
	lua_pushboolean(l, 1);
	return 1;
}

/**
**  Research an upgrade.
**
**  @param l  Lua state.
*/
static int CclAiResearch(lua_State *l)
{
	LuaCheckArgs(l, 1);
	//Wyrmgus start
	if (lua_isnil(l, 1)) {
		lua_pushboolean(l, 0);
		return 1;
	}
	//Wyrmgus end
	const char *str = LuaToString(l, 1);
	const CUpgrade *upgrade;

	if (str) {
		upgrade = CUpgrade::Get(str);
	} else {
		LuaError(l, "Upgrade needed");
		upgrade = nullptr;
	}
	//Wyrmgus start
//	InsertResearchRequests(upgrade);
	if (UpgradeIdentAllowed(*AiPlayer->Player, str) != 'R') {
		InsertResearchRequests(upgrade);
	}
	//Wyrmgus end
	lua_pushboolean(l, 0);
	return 1;
}

/**
**  Upgrade an unit to an new unit-type.
**
**  @param l  Lua state.
*/
static int CclAiUpgradeTo(lua_State *l)
{
	LuaCheckArgs(l, 1);
	//Wyrmgus start
	if (lua_isnil(l, 1)) {
		lua_pushboolean(l, 0);
		return 1;
	}
	//Wyrmgus end
	CUnitType *type = CclGetUnitType(l);
	InsertUpgradeToRequests(type);

	lua_pushboolean(l, 0);
	return 1;
}

/**
**  Return the player of the running AI.
**
**  @param l  Lua state.
**
**  @return  Player number of the AI.
*/
static int CclAiPlayer(lua_State *l)
{
	LuaCheckArgs(l, 0);
	lua_pushnumber(l, AiPlayer->Player->GetIndex());
	return 1;
}

/**
**  Set AI player resource reserve.
**
**  @param l  Lua state.
**
**  @return     Old resource vector
*/
static int CclAiSetReserve(lua_State *l)
{
	LuaCheckArgs(l, 1);
	if (!lua_istable(l, 1)) {
		LuaError(l, "incorrect argument");
	}
	lua_newtable(l);
	for (size_t i = 0; i < CResource::GetAll().size(); ++i) {
		lua_pushnumber(l, AiPlayer->Reserve[i]);
		lua_rawseti(l, -2, i + 1);
	}
	for (size_t i = 0; i < CResource::GetAll().size(); ++i) {
		AiPlayer->Reserve[i] = LuaToNumber(l, 1, i + 1);
	}
	return 1;
}

/**
**  Set AI player resource collect percent.
**
**  @param l  Lua state.
*/
static int CclAiSetCollect(lua_State *l)
{
	//Wyrmgus start
	/*
	LuaCheckArgs(l, 1);
	if (!lua_istable(l, 1)) {
		LuaError(l, "incorrect argument");
	}
	for (int i = 0; i < MaxCosts; ++i) {
		AiPlayer->Collect[i] = LuaToNumber(l, 1, i + 1);
	}
	*/
	const unsigned int args = lua_gettop(l);

	if (args & 1) {
		LuaError(l, "incorrect argument");
	}
	for (unsigned int j = 0; j < args; ++j) {
		const std::string resource = LuaToString(l, j + 1);
		const int resId = GetResourceIdByName(l, resource.c_str());

		++j;
		AiPlayer->Collect[resId] = LuaToNumber(l, j + 1);
	}
	//Wyrmgus end
	return 0;
}

/**
**  Set AI player build.
**
**  @param l  Lua state.
*/
static int CclAiSetBuildDepots(lua_State *l)
{
	LuaCheckArgs(l, 1);
	if (!lua_isboolean(l, 1)) {
		LuaError(l, "incorrect argument");
	}
	AiPlayer->BuildDepots = LuaToBoolean(l, 1);
	return 0;
}

/**
**  Dump some AI debug information.
**
**  @param l  Lua state.
*/
static int CclAiDump(lua_State *l)
{
	LuaCheckArgs(l, 0);
	for (int p = 0; p < PlayerMax - 1; ++p) {
		CPlayer &aip = *CPlayer::Players[p];
		if (aip.AiEnabled) {
			//
			// Script
			//

			printf("------\n");
			for (size_t i = 0; i < CResource::GetAll().size(); ++i) {
				printf("%s(%4d, %4d/%4d) ", DefaultResourceNames[i].c_str(),
					   aip.Resources[i], aip.StoredResources[i], aip.MaxResources[i]);
			}
			printf("\n");
			printf("Player %d:", aip.GetIndex());
#if 0
			gh_display(gh_car(AiPlayer->Script));
#endif
			//
			// Requests
			//
			size_t n = aip.Ai->UnitTypeRequests.size();
			printf("UnitTypeRequests(%u):\n", static_cast<unsigned int>(n));
			for (size_t i = 0; i < n; ++i) {
				printf("%s ", aip.Ai->UnitTypeRequests[i].Type->Ident.c_str());
			}
			printf("\n");
			n = aip.Ai->UpgradeToRequests.size();
			printf("UpgradeToRequests(%u):\n", static_cast<unsigned int>(n));
			for (size_t i = 0; i < n; ++i) {
				printf("%s ", aip.Ai->UpgradeToRequests[i]->Ident.c_str());
			}
			printf("\n");
			n = aip.Ai->ResearchRequests.size();
			printf("ResearchRequests(%u):\n", static_cast<unsigned int>(n));
			for (size_t i = 0; i < n; ++i) {
				printf("%s ", aip.Ai->ResearchRequests[i]->Ident.c_str());
			}
			printf("\n");

			// Building queue
			printf("Building queue:\n");
			for (size_t i = 0; i < aip.Ai->UnitTypeBuilt.size(); ++i) {
				const AiBuildQueue &queue = aip.Ai->UnitTypeBuilt[i];
				printf("%s(%d/%d) ", queue.Type->Ident.c_str(), queue.Made, queue.Want);
			}
			printf("\n");

			// PrintForce
			for (size_t i = 0; i < aip.Ai->Force.Size(); ++i) {
				printf("Force(%u%s%s):\n", static_cast<unsigned int>(i),
					   aip.Ai->Force[i].Completed ? ",complete" : ",recruit",
					   aip.Ai->Force[i].Attacking ? ",attack" : "");
				for (size_t j = 0; j < aip.Ai->Force[i].UnitTypes.size(); ++j) {
					const AiUnitType &aut = aip.Ai->Force[i].UnitTypes[j];
					printf("%s(%d) ", aut.Type->Ident.c_str(), aut.Want);
				}
				printf("\n");
			}
			printf("\n");
		}
	}
	lua_pushboolean(l, 0);
	return 1;
}

/**
**  Parse AiBuildQueue builing list
**
**  @param l     Lua state.
**  @param ai  PlayerAi pointer which should be filled with the data.
*/
static void CclParseBuildQueue(lua_State *l, PlayerAi *ai, int offset)
{
	if (!lua_istable(l, offset)) {
		LuaError(l, "incorrect argument");
	}

	Vector2i pos(-1, -1);
	//Wyrmgus start
	int z = -1;
	int landmass = 0;
	CSite *settlement = nullptr;
	//Wyrmgus end

	const int args = lua_rawlen(l, offset);
	for (int k = 0; k < args; ++k) {
		const char *value = LuaToString(l, offset, k + 1);
		++k;

		if (!strcmp(value, "onpos")) {
			pos.x = LuaToNumber(l, offset, k + 1);
			++k;
			pos.y = LuaToNumber(l, offset, k + 1);
		//Wyrmgus start
		} else if (!strcmp(value, "map-layer")) {
			z = LuaToNumber(l, offset, k + 1);
		} else if (!strcmp(value, "landmass")) {
			landmass = LuaToNumber(l, offset, k + 1);
		} else if (!strcmp(value, "settlement")) {
			settlement = CSite::Get(LuaToString(l, offset, k + 1));
		//Wyrmgus end
		} else {
			//ident = LuaToString(l, j + 1, k + 1);
			//++k;
			const int made = LuaToNumber(l, offset, k + 1);
			++k;
			const int want = LuaToNumber(l, offset, k + 1);

			AiBuildQueue queue;
			queue.Type = CUnitType::Get(value);
			queue.Want = want;
			queue.Made = made;
			queue.Pos = pos;
			//Wyrmgus start
			queue.MapLayer = z;
			queue.Landmass = landmass;
			queue.Settlement = settlement;
			//Wyrmgus end

			ai->UnitTypeBuilt.push_back(queue);
			pos.x = -1;
			pos.y = -1;
			//Wyrmgus start
			z = -1;
			landmass = 0;
			settlement = nullptr;
			//Wyrmgus end
		}
	}
}

/**
** Define an AI player.
**
**  @param l  Lua state.
*/
static int CclDefineAiPlayer(lua_State *l)
{
	const unsigned int playerIdx = LuaToNumber(l, 0 + 1);

	Assert(playerIdx <= PlayerMax);
	DebugPrint("%p %d\n" _C_(void *)CPlayer::Players[playerIdx]->Ai _C_ CPlayer::Players[playerIdx]->AiEnabled);
	// FIXME: lose this:
	// Assert(!CPlayer::Players[playerIdx]->Ai && CPlayer::Players[playerIdx]->AiEnabled);

	PlayerAi *ai = CPlayer::Players[playerIdx]->Ai = new PlayerAi;
	ai->Player = CPlayer::Players[playerIdx];

	// Parse the list: (still everything could be changed!)
	const int args = lua_gettop(l);
	for (int j = 1; j < args; ++j) {
		const char *value = LuaToString(l, j + 1);
		++j;

		if (!strcmp(value, "ai-type")) {
			const char *aiName = LuaToString(l, j + 1);
			CAiType *ait = GetAiTypesByName(aiName);
			if (ait == nullptr) {
				LuaError(l, "ai-type not found: %s" _C_ aiName);
			}
			ai->AiType = ait;
			ai->Script = ait->Script;
		} else if (!strcmp(value, "script")) {
			ai->Script = LuaToString(l, j + 1);
		} else if (!strcmp(value, "script-debug")) {
			ai->ScriptDebug = LuaToBoolean(l, j + 1);
		} else if (!strcmp(value, "sleep-cycles")) {
			ai->SleepCycles = LuaToNumber(l, j + 1);
		//Wyrmgus start
		} else if (!strcmp(value, "scouting")) {
			ai->Scouting = true;
			--j;
		//Wyrmgus end
		} else if (!strcmp(value, "force")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			const int cclforceIdx = LuaToNumber(l, j + 1, 1);
			const int forceIdx = ai->Force.FindFreeForce(AiForceRoleDefault);

			for (int k = 1; k < subargs; ++k) {
				const char *value = LuaToString(l, j + 1, k + 1);
				++k;
				if (!strcmp(value, "complete")) {
					ai->Force[forceIdx].Completed = true;
					--k;
				} else if (!strcmp(value, "recruit")) {
					ai->Force[forceIdx].Completed = false;
					--k;
				} else if (!strcmp(value, "attack")) {
					ai->Force[forceIdx].Attacking = true;
					--k;
				} else if (!strcmp(value, "defend")) {
					ai->Force[forceIdx].Defending = true;
					--k;
				} else if (!strcmp(value, "role")) {
					value = LuaToString(l, j + 1, k + 1);
					if (!strcmp(value, "attack")) {
						ai->Force[forceIdx].Role = AiForceRoleAttack;
					} else if (!strcmp(value, "defend")) {
						ai->Force[forceIdx].Role = AiForceRoleDefend;
					} else {
						LuaError(l, "Unsupported force tag: %s" _C_ value);
					}
				} else if (!strcmp(value, "types")) {
					lua_rawgeti(l, j + 1, k + 1);
					if (!lua_istable(l, -1)) {
						LuaError(l, "incorrect argument");
					}
					const int subsubargs = lua_rawlen(l, -1);
					for (int subk = 0; subk < subsubargs; ++subk) {
						const int num = LuaToNumber(l, -1, subk + 1);
						++subk;
						const char *ident = LuaToString(l, -1, subk + 1);
						AiUnitType queue;

						queue.Want = num;
						queue.Type = CUnitType::Get(ident);
						ai->Force[forceIdx].UnitTypes.push_back(queue);
					}
					lua_pop(l, 1);
				} else if (!strcmp(value, "units")) {
					lua_rawgeti(l, j + 1, k + 1);
					if (!lua_istable(l, -1)) {
						LuaError(l, "incorrect argument");
					}
					const int subsubargs = lua_rawlen(l, -1);
					for (int subk = 0; subk < subsubargs; ++subk) {
						const int num = LuaToNumber(l, -1, subk + 1);
						++subk;
#if 0
						const char *ident = LuaToString(l, -1, subk + 1);
#endif
						ai->Force[forceIdx].Units.Insert(&UnitManager.GetSlotUnit(num));
					}
					lua_pop(l, 1);
				} else if (!strcmp(value, "state")) {
					ai->Force[forceIdx].State = AiForceAttackingState(LuaToNumber(l, j + 1, k + 1));
				} else if (!strcmp(value, "goalx")) {
					ai->Force[forceIdx].GoalPos.x = LuaToNumber(l, j + 1, k + 1);
				} else if (!strcmp(value, "goaly")) {
					ai->Force[forceIdx].GoalPos.y = LuaToNumber(l, j + 1, k + 1);
				} else if (!strcmp(value, "must-transport")) {
					// Keep for backward compatibility
				} else {
					LuaError(l, "Unsupported tag: %s" _C_ value);
				}
			}
		} else if (!strcmp(value, "reserve")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				const char *type = LuaToString(l, j + 1, k + 1);
				++k;
				int num = LuaToNumber(l, j + 1, k + 1);
				const int resId = GetResourceIdByName(l, type);
				ai->Reserve[resId] = num;
			}
		} else if (!strcmp(value, "used")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				const char *type = LuaToString(l, j + 1, k + 1);
				++k;
				const int num = LuaToNumber(l, j + 1, k + 1);
				const int resId = GetResourceIdByName(l, type);
				ai->Used[resId] = num;
			}
		} else if (!strcmp(value, "needed")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				const char *type = LuaToString(l, j + 1, k + 1);
				++k;
				const int num = LuaToNumber(l, j + 1, k + 1);
				const int resId = GetResourceIdByName(l, type);
				ai->Needed[resId] = num;
			}
		} else if (!strcmp(value, "collect")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				const char *type = LuaToString(l, j + 1, k + 1);
				++k;
				const int num = LuaToNumber(l, j + 1, k + 1);
				const int resId = GetResourceIdByName(l, type);
				ai->Collect[resId] = num;
			}
		} else if (!strcmp(value, "need-mask")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				const char *type = LuaToString(l, j + 1, k + 1);
				const int resId = GetResourceIdByName(l, type);
				ai->NeededMask |= ((long long int) 1 << resId);
			}
		} else if (!strcmp(value, "need-supply")) {
			ai->NeedSupply = true;
			--j;
		} else if (!strcmp(value, "exploration")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				Vector2i pos;

				lua_rawgeti(l, j + 1, k + 1);
				if (!lua_istable(l, -1) || lua_rawlen(l, -1) != 3) {
					LuaError(l, "incorrect argument");
				}
				pos.x = LuaToNumber(l, -1, 1);
				pos.y = LuaToNumber(l, -1, 2);
				const int mask = LuaToNumber(l, -1, 3);
				lua_pop(l, 1);
				AiExplorationRequest queue(pos, mask);
				ai->FirstExplorationRequest.push_back(queue);
			}
		} else if (!strcmp(value, "last-exploration-cycle")) {
			ai->LastExplorationGameCycle = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "last-can-not-move-cycle")) {
			ai->LastCanNotMoveGameCycle = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "unit-type")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			int i = 0;
			if (subargs) {
				//Wyrmgus start
//				ai->UnitTypeRequests.resize(subargs / 2);
				ai->UnitTypeRequests.resize(subargs / 3);
				//Wyrmgus end
			}
			for (int k = 0; k < subargs; ++k) {
				const char *ident = LuaToString(l, j + 1, k + 1);
				++k;
				const int count = LuaToNumber(l, j + 1, k + 1);
				//Wyrmgus start
				++k;
				const int landmass = LuaToNumber(l, j + 1, k + 1);
				//Wyrmgus end
				ai->UnitTypeRequests[i].Type = CUnitType::Get(ident);
				ai->UnitTypeRequests[i].Count = count;
				//Wyrmgus start
				ai->UnitTypeRequests[i].Landmass = landmass;
				//Wyrmgus end
				++i;
			}
		} else if (!strcmp(value, "upgrade")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				const char *ident = LuaToString(l, j + 1, k + 1);
				ai->UpgradeToRequests.push_back(CUnitType::Get(ident));
			}
		} else if (!strcmp(value, "research")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				const char *ident = LuaToString(l, j + 1, k + 1);
				const CUpgrade *upgrade = CUpgrade::Get(ident);
				if (upgrade != nullptr) {
					ai->ResearchRequests.push_back(upgrade);
				}
			}
		} else if (!strcmp(value, "building")) {
			CclParseBuildQueue(l, ai, j + 1);
		} else if (!strcmp(value, "repair-building")) {
			ai->LastRepairBuilding = LuaToNumber(l, j + 1);
		//Wyrmgus start
		} else if (!strcmp(value, "pathway-construction-building")) {
			ai->LastPathwayConstructionBuilding = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "scouts")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				const int num = LuaToNumber(l, j + 1, k + 1);
				++k;
				ai->Scouts.push_back(&UnitManager.GetSlotUnit(num));
			}
		} else if (!strcmp(value, "transporters")) {
			if (!lua_istable(l, j + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, j + 1);
			for (int k = 0; k < subargs; ++k) {
				int landmass = LuaToNumber(l, j + 1, k + 1);
				++k;
				const int num = LuaToNumber(l, j + 1, k + 1);
				ai->Transporters[landmass].push_back(&UnitManager.GetSlotUnit(num));
			}
		//Wyrmgus end
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	return 0;
}

/**
**  Register CCL features for unit-type.
*/
void AiCclRegister()
{
	// FIXME: Need to save memory here.
	// Loading all into memory isn't necessary.

	lua_register(Lua, "DefineAiHelper", CclDefineAiHelper);
	lua_register(Lua, "DefineAi", CclDefineAi);

	lua_register(Lua, "AiGetRace", CclAiGetRace);
	lua_register(Lua, "AiGetSleepCycles", CclAiGetSleepCycles);

	lua_register(Lua, "AiDebug", CclAiDebug);
	lua_register(Lua, "AiDebugPlayer", CclAiDebugPlayer);
	lua_register(Lua, "AiNeed", CclAiNeed);
	lua_register(Lua, "AiSet", CclAiSet);
	lua_register(Lua, "AiWait", CclAiWait);

	lua_register(Lua, "AiForce", CclAiForce);

	lua_register(Lua, "AiReleaseForce", CclAiReleaseForce);
	lua_register(Lua, "AiForceRole", CclAiForceRole);
	lua_register(Lua, "AiCheckForce", CclAiCheckForce);
	lua_register(Lua, "AiWaitForce", CclAiWaitForce);

	lua_register(Lua, "AiAttackWithForce", CclAiAttackWithForce);
	lua_register(Lua, "AiSleep", CclAiSleep);
	lua_register(Lua, "AiResearch", CclAiResearch);
	lua_register(Lua, "AiUpgradeTo", CclAiUpgradeTo);

	lua_register(Lua, "AiPlayer", CclAiPlayer);
	lua_register(Lua, "AiSetReserve", CclAiSetReserve);
	lua_register(Lua, "AiSetCollect", CclAiSetCollect);

	lua_register(Lua, "AiSetBuildDepots", CclAiSetBuildDepots);
	
	lua_register(Lua, "AiDump", CclAiDump);

	lua_register(Lua, "DefineAiPlayer", CclDefineAiPlayer);
	lua_register(Lua, "AiAttackWithForces", CclAiAttackWithForces);
	lua_register(Lua, "AiWaitForces", CclAiWaitForces);
}
