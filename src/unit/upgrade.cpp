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
/**@name upgrade.cpp - The upgrade/allow functions. */
//
//      (c) Copyright 1999-2016 by Vladi Belperchinov-Shabanski, Jimmy Salmon
//		and Andrettin
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

#include <string>
#include <vector>
#include <map>

#include "stratagus.h"

#include "upgrade.h"

//Wyrmgus start
#include "action/action_build.h"
//Wyrmgus end
#include "action/action_train.h"
//Wyrmgus start
#include "action/action_upgradeto.h"
#include "../ai/ai_local.h"
//Wyrmgus end
#include "commands.h"
#include "depend.h"
//Wyrmgus start
#include "editor.h"
#include "game.h"
#include "grand_strategy.h"
//Wyrmgus end
#include "interface.h"
#include "iolib.h"
//Wyrmgus start
#include "item.h"
//Wyrmgus end
#include "map.h"
//Wyrmgus start
#include "network.h"
//Wyrmgus end
#include "player.h"
#include "script.h"
//Wyrmgus start
#include "settings.h"
#include "tileset.h"
#include "translate.h"
//Wyrmgus end
#include "unit.h"
#include "unit_find.h"
//Wyrmgus start
#include "ui.h"
#include "unit_manager.h"
//Wyrmgus end
#include "unittype.h"
#include "util.h"

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

//Wyrmgus start
//static void AllowUnitId(CPlayer &player, int id, int units);
//Wyrmgus end

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

std::vector<CUpgrade *> AllUpgrades;           /// The main user useable upgrades

/// Upgrades modifiers
CUpgradeModifier *UpgradeModifiers[UPGRADE_MODIFIERS_MAX];
/// Number of upgrades modifiers used
int NumUpgradeModifiers;

std::map<std::string, CUpgrade *> Upgrades;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/


CUnitStats::~CUnitStats()
{
	delete [] this->Variables;
}

const CUnitStats &CUnitStats::operator = (const CUnitStats &rhs)
{
	for (unsigned int i = 0; i < MaxCosts; ++i) {
		this->Costs[i] = rhs.Costs[i];
		this->Storing[i] = rhs.Storing[i];
		this->ImproveIncomes[i] = rhs.ImproveIncomes[i];
		//Wyrmgus start
		this->ResourceDemand[i] = rhs.ResourceDemand[i];
		//Wyrmgus end
	}
	//Wyrmgus start
	for (size_t i = 0; i < UnitTypes.size(); ++i) {
		this->UnitStock[i] = rhs.UnitStock[i];
	}
	//Wyrmgus end
	delete [] this->Variables;
	const unsigned int size = UnitTypeVar.GetNumberVariable();
	this->Variables = new CVariable[size];

	std::copy(rhs.Variables, rhs.Variables + size, this->Variables);
	return *this;
}

bool CUnitStats::operator == (const CUnitStats &rhs) const
{
	for (int i = 0; i != MaxCosts; ++i) {
		if (this->Costs[i] != rhs.Costs[i]) {
			return false;
		}
		if (this->Storing[i] != rhs.Storing[i]) {
			return false;
		}
		if (this->ImproveIncomes[i] != rhs.ImproveIncomes[i]) {
			return false;
		}
		//Wyrmgus start
		if (this->ResourceDemand[i] != rhs.ResourceDemand[i]) {
			return false;
		}
		//Wyrmgus end
	}
	//Wyrmgus start
	for (size_t i = 0; i < UnitTypes.size(); ++i) {
		if (this->UnitStock[i] != rhs.UnitStock[i]) {
			return false;
		}
	}
	//Wyrmgus end
	for (unsigned int i = 0; i != UnitTypeVar.GetNumberVariable(); ++i) {
		if (this->Variables[i] != rhs.Variables[i]) {
			return false;
		}
	}
	return true;
}

bool CUnitStats::operator != (const CUnitStats &rhs) const
{
	return !(*this == rhs);
}

//Wyrmgus start
int CUnitStats::GetPrice() const
{
	int cost = 0;
	
	for (int i = 1; i < MaxCosts; ++i) {
		if (this->Costs[i] > 0) {
			if (i == CopperCost) {
				cost += this->Costs[i];
			} else {
				cost += this->Costs[i] * DefaultResourcePrices[i] / 100;
			}
		}
	}
	
	return cost;
}
//Wyrmgus end

CUpgrade::CUpgrade(const std::string &ident) :
	//Wyrmgus start
//	Ident(ident), ID(0)
	Ident(ident), ID(0), Class(-1), Civilization(-1), Faction(-1), Ability(false), Weapon(false), Shield(false), Boots(false), Arrows(false), MagicPrefix(false), MagicSuffix(false), RunicAffix(false),UniqueOnly(false), Work(-1), Icon(NULL), Item(NULL), MaxLimit(1), MagicLevel(0), Year(0), Author(NULL)
	//Wyrmgus end
{
	memset(this->Costs, 0, sizeof(this->Costs));
	//Wyrmgus start
	memset(this->ScaledCosts, 0, sizeof(this->ScaledCosts));
	memset(this->GrandStrategyProductionEfficiencyModifier, 0, sizeof(this->GrandStrategyProductionEfficiencyModifier));
	memset(this->ItemPrefix, 0, sizeof(this->ItemPrefix));
	memset(this->ItemSuffix, 0, sizeof(this->ItemSuffix));
	memset(this->IncompatibleAffixes, 0, sizeof(this->IncompatibleAffixes));
	//Wyrmgus end
}

CUpgrade::~CUpgrade()
{
	//Wyrmgus start
	RequiredAbilities.clear();
	WeaponClasses.clear();
	Epithets.clear();
	//Wyrmgus end
}

/**
**  Create a new upgrade
**
**  @param ident  Upgrade identifier
*/
CUpgrade *CUpgrade::New(const std::string &ident)
{
	CUpgrade *upgrade = Upgrades[ident];
	if (upgrade) {
		return upgrade;
	} else {
		upgrade = new CUpgrade(ident);
		Upgrades[ident] = upgrade;
		upgrade->ID = AllUpgrades.size();
		AllUpgrades.push_back(upgrade);
		return upgrade;
	}
}

/**
**  Get an upgrade
**
**  @param ident  Upgrade identifier
**
**  @return       Upgrade pointer or NULL if not found.
*/
CUpgrade *CUpgrade::Get(const std::string &ident)
{
	CUpgrade *upgrade = Upgrades[ident];
	if (!upgrade) {
		DebugPrint("upgrade not found: %s\n" _C_ ident.c_str());
	}
	return upgrade;
}

/**
**  Init upgrade/allow structures
*/
void InitUpgrades()
{
}

/**
**  Cleanup the upgrade module.
*/
void CleanUpgrades()
{
	//  Free the upgrades.
	while (AllUpgrades.empty() == false) {
		CUpgrade *upgrade = AllUpgrades.back();
		AllUpgrades.pop_back();
		delete upgrade;
	}
	Upgrades.clear();

	//
	//  Free the upgrade modifiers.
	//
	for (int i = 0; i < NumUpgradeModifiers; ++i) {
		delete UpgradeModifiers[i];
	}
	NumUpgradeModifiers = 0;
}

/**
**  Save state of the dependencies to file.
**
**  @param file  Output file.
*/
void SaveUpgrades(CFile &file)
{
	file.printf("\n-- -----------------------------------------\n");
	file.printf("-- MODULE: upgrades\n\n");

	//
	//  Save the allow
	//
	for (std::vector<CUnitType *>::size_type i = 0; i < UnitTypes.size(); ++i) {
		file.printf("DefineUnitAllow(\"%s\", ", UnitTypes[i]->Ident.c_str());
		for (int p = 0; p < PlayerMax; ++p) {
			if (p) {
				file.printf(", ");
			}
			file.printf("%d", Players[p].Allow.Units[i]);
		}
		file.printf(")\n");
	}
	file.printf("\n");

	//
	//  Save the upgrades
	//
	for (std::vector<CUpgrade *>::size_type j = 0; j < AllUpgrades.size(); ++j) {
		file.printf("DefineAllow(\"%s\", \"", AllUpgrades[j]->Ident.c_str());
		for (int p = 0; p < PlayerMax; ++p) {
			file.printf("%c", Players[p].Allow.Upgrades[j]);
		}
		file.printf("\")\n");
	}
}

/*----------------------------------------------------------------------------
--  Ccl part of upgrades
----------------------------------------------------------------------------*/

//Wyrmgus start
/**
**  Define an upgrade.
**
**  @param l  Lua state.
*/
static int CclDefineUpgrade(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument (expected table)");
	}

	std::string upgrade_ident = LuaToString(l, 1);
	CUpgrade *upgrade = CUpgrade::New(upgrade_ident);
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Parent")) {
			CUpgrade *parent_upgrade = CUpgrade::Get(LuaToString(l, -1));
			if (parent_upgrade != NULL) {
				upgrade->Name = parent_upgrade->Name;
				upgrade->Icon = parent_upgrade->Icon;
				upgrade->Class = parent_upgrade->Class;
				upgrade->Description = parent_upgrade->Description;
				upgrade->Quote = parent_upgrade->Quote;
				upgrade->Background = parent_upgrade->Background;
				upgrade->EffectsString = parent_upgrade->EffectsString;
				upgrade->RequirementsString = parent_upgrade->RequirementsString;
				upgrade->RequiresDeity = parent_upgrade->RequiresDeity;
				for (int i = 0; i < MaxCosts; ++i) {
					upgrade->Costs[i] = parent_upgrade->Costs[i];
					upgrade->ScaledCosts[i] = parent_upgrade->ScaledCosts[i];
					upgrade->GrandStrategyProductionEfficiencyModifier[i] = parent_upgrade->GrandStrategyProductionEfficiencyModifier[i];
				}
				for (int i = 0; i < MaxItemClasses; ++i) {
					upgrade->ItemPrefix[i] = parent_upgrade->ItemPrefix[i];
					upgrade->ItemSuffix[i] = parent_upgrade->ItemSuffix[i];
				}
				upgrade->MaxLimit = parent_upgrade->MaxLimit;
				upgrade->MagicLevel = parent_upgrade->MagicLevel;
				upgrade->Ability = parent_upgrade->Ability;
				upgrade->Weapon = parent_upgrade->Weapon;
				upgrade->Shield = parent_upgrade->Shield;
				upgrade->Boots = parent_upgrade->Boots;
				upgrade->Arrows = parent_upgrade->Arrows;
				upgrade->Item = parent_upgrade->Item;
				upgrade->MagicPrefix = parent_upgrade->MagicPrefix;
				upgrade->MagicSuffix = parent_upgrade->MagicSuffix;
				upgrade->RunicAffix = parent_upgrade->RunicAffix;
				upgrade->Work = parent_upgrade->Work;
				upgrade->UniqueOnly = parent_upgrade->UniqueOnly;
				for (size_t i = 0; i < parent_upgrade->ScaledCostUnits.size(); ++i) {
					upgrade->ScaledCostUnits.push_back(parent_upgrade->ScaledCostUnits[i]);
				}
			} else {
				LuaError(l, "Parent upgrade doesn't exist.");
			}
		} else if (!strcmp(value, "Name")) {
			upgrade->Name = LuaToString(l, -1);
		} else if (!strcmp(value, "Icon")) {
			CIcon *icon = CIcon::Get(LuaToString(l, -1));
			if (icon != NULL) {
				upgrade->Icon = icon;
			} else {
				LuaError(l, "Icon doesn't exist.");
			}
		} else if (!strcmp(value, "Class")) {
			std::string class_name = LuaToString(l, -1);
			
			int class_id = GetUpgradeClassIndexByName(class_name);
			if (class_id == -1) {
				SetUpgradeClassStringToIndex(class_name, UpgradeClasses.size());
				class_id = UpgradeClasses.size();
				UpgradeClasses.push_back(class_name);
			}
			
			upgrade->Class = class_id;
		} else if (!strcmp(value, "Civilization")) {
			std::string civilization_name = LuaToString(l, -1);
			upgrade->Civilization = PlayerRaces.GetRaceIndexByName(civilization_name.c_str());
			if (upgrade->Civilization == -1) {
				LuaError(l, "Civilization \"%s\" doesn't exist." _C_ civilization_name.c_str());
			}
		} else if (!strcmp(value, "Faction")) {
			std::string faction_name = LuaToString(l, -1);
			CFaction *faction = PlayerRaces.GetFaction(faction_name);
			if (faction) {
				upgrade->Faction = faction->ID;
			} else {
				LuaError(l, "Faction \"%s\" doesn't exist." _C_ faction_name.c_str());
			}
		} else if (!strcmp(value, "Description")) {
			upgrade->Description = LuaToString(l, -1);
		} else if (!strcmp(value, "Quote")) {
			upgrade->Quote = LuaToString(l, -1);
		} else if (!strcmp(value, "Background")) {
			upgrade->Background = LuaToString(l, -1);
		} else if (!strcmp(value, "EffectsString")) {
			upgrade->EffectsString = LuaToString(l, -1);
		} else if (!strcmp(value, "RequirementsString")) {
			upgrade->RequirementsString = LuaToString(l, -1);
		} else if (!strcmp(value, "MaxLimit")) {
			upgrade->MaxLimit = LuaToNumber(l, -1);
		} else if (!strcmp(value, "MagicLevel")) {
			upgrade->MagicLevel = LuaToNumber(l, -1);
		} else if (!strcmp(value, "Year")) {
			upgrade->Year = LuaToNumber(l, -1);
		} else if (!strcmp(value, "Ability")) {
			upgrade->Ability = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Weapon")) {
			upgrade->Weapon = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Shield")) {
			upgrade->Shield = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Boots")) {
			upgrade->Boots = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Arrows")) {
			upgrade->Arrows = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "MagicPrefix")) {
			upgrade->MagicPrefix = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "MagicSuffix")) {
			upgrade->MagicSuffix = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "RunicAffix")) {
			upgrade->RunicAffix = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "UniqueOnly")) {
			upgrade->UniqueOnly = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "RequiresDeity")) {
			upgrade->RequiresDeity = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Work")) {
			int work_type = GetItemClassIdByName(LuaToString(l, -1));
			if (work_type != -1) {
				upgrade->Work = work_type;
			} else {
				LuaError(l, "Work item class doesn't exist.");
			}
		} else if (!strcmp(value, "Item")) {
			CUnitType *item = UnitTypeByIdent(LuaToString(l, -1));
			if (item != NULL) {
				upgrade->Item = item;
			} else {
				LuaError(l, "Item type doesn't exist.");
			}
		} else if (!strcmp(value, "Costs")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				int resource = GetResourceIdByName(LuaToString(l, -1, j + 1));
				if (resource == -1) {
					LuaError(l, "Resource doesn't exist.");
				}
				++j;
				
				upgrade->Costs[resource] = LuaToNumber(l, -1, j + 1);
			}
		} else if (!strcmp(value, "ScaledCosts")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				int resource = GetResourceIdByName(LuaToString(l, -1, j + 1));
				if (resource == -1) {
					LuaError(l, "Resource doesn't exist.");
				}
				++j;
				
				upgrade->ScaledCosts[resource] = LuaToNumber(l, -1, j + 1);
			}
		} else if (!strcmp(value, "GrandStrategyProductionEfficiencyModifier")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				int resource = GetResourceIdByName(LuaToString(l, -1, j + 1));
				if (resource == -1) {
					LuaError(l, "Resource doesn't exist.");
				}
				++j;
				
				upgrade->GrandStrategyProductionEfficiencyModifier[resource] = LuaToNumber(l, -1, j + 1);
			}
		} else if (!strcmp(value, "ItemPrefix")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				int item_class = GetItemClassIdByName(LuaToString(l, -1, j + 1));
				if (item_class == -1) {
					LuaError(l, "Item class doesn't exist.");
				}
				++j;
				
				upgrade->ItemPrefix[item_class] = LuaToBoolean(l, -1, j + 1);
			}
		} else if (!strcmp(value, "ItemSuffix")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				int item_class = GetItemClassIdByName(LuaToString(l, -1, j + 1));
				if (item_class == -1) {
					LuaError(l, "Item class doesn't exist.");
				}
				++j;

				upgrade->ItemSuffix[item_class] = LuaToBoolean(l, -1, j + 1);
			}
		} else if (!strcmp(value, "IncompatibleAffixes")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				int affix_id = UpgradeIdByIdent(LuaToString(l, -1, j + 1));
				if (affix_id == -1) {
					LuaError(l, "Upgrade doesn't exist.");
				}

				upgrade->IncompatibleAffixes[affix_id] = true;
			}
		} else if (!strcmp(value, "ScaledCostUnits")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				CUnitType *scaled_cost_unit = UnitTypeByIdent(LuaToString(l, -1, j + 1));
				if (scaled_cost_unit == NULL) {
					LuaError(l, "Unit type doesn't exist.");
				}

				upgrade->ScaledCostUnits.push_back(scaled_cost_unit);
			}
		} else if (!strcmp(value, "RequiredAbilities")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				CUpgrade *required_ability = CUpgrade::Get(LuaToString(l, -1, j + 1));
				if (required_ability == NULL) {
					LuaError(l, "Upgrade doesn't exist.");
				}

				upgrade->RequiredAbilities.push_back(required_ability);
			}
		} else if (!strcmp(value, "WeaponClasses")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				int item_class = GetItemClassIdByName(LuaToString(l, -1, j + 1));
				if (item_class == -1) {
					LuaError(l, "Item class doesn't exist.");
				}

				upgrade->WeaponClasses.push_back(item_class);
			}
		} else if (!strcmp(value, "Epithets")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				std::string epithet = LuaToString(l, -1, j + 1);

				upgrade->Epithets.push_back(epithet);
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	//set the upgrade's civilization and class here
	if (upgrade->Class != -1) { //if class is defined, then use this upgrade to help build the classes table, and add this upgrade to the civilization class table (if the civilization is defined)
		int class_id = upgrade->Class;
		if (upgrade->Civilization != -1) {
			int civilization_id = upgrade->Civilization;
			
			if (upgrade->Faction != -1) {
				int faction_id = upgrade->Faction;
				if (faction_id != -1 && class_id != -1) {
					PlayerRaces.Factions[faction_id]->ClassUpgrades[class_id] = upgrade->ID;
				}
			} else {
				if (civilization_id != -1 && class_id != -1) {
					PlayerRaces.CivilizationClassUpgrades[civilization_id][class_id] = upgrade->ID;
				}
			}
		}
	}
	
	for (unsigned int i = 0; i < UpgradeMax; ++i) { //add the upgrade to the incompatible affix's counterpart list here
		if (upgrade->IncompatibleAffixes[i]) {
			AllUpgrades[i]->IncompatibleAffixes[upgrade->ID] = true;
		}
	}
	
	//load icon here
	if (upgrade->Icon != NULL && !upgrade->Icon->Loaded) {
		upgrade->Icon->Load();
	}
	
	return 0;
}
//Wyrmgus end

/**
**  Define a new upgrade modifier.
**
**  @param l  List of modifiers.
*/
static int CclDefineModifier(lua_State *l)
{
	const int args = lua_gettop(l);

	CUpgradeModifier *um = new CUpgradeModifier;

	memset(um->ChangeUpgrades, '?', sizeof(um->ChangeUpgrades));
	memset(um->ApplyTo, '?', sizeof(um->ApplyTo));
	um->Modifier.Variables = new CVariable[UnitTypeVar.GetNumberVariable()];
	um->ModifyPercent = new int[UnitTypeVar.GetNumberVariable()];
	memset(um->ModifyPercent, 0, UnitTypeVar.GetNumberVariable() * sizeof(int));

	std::string upgrade_ident = LuaToString(l, 1);
	um->UpgradeId = UpgradeIdByIdent(upgrade_ident);
	if (um->UpgradeId == -1) {
		LuaError(l, "Error when defining upgrade modifier: upgrade \"%s\" doesn't exist." _C_ upgrade_ident.c_str());
	}
	
	//Wyrmgus start
	AllUpgrades[um->UpgradeId]->UpgradeModifiers.push_back(um);
	//Wyrmgus end

	for (int j = 1; j < args; ++j) {
		if (!lua_istable(l, j + 1)) {
			LuaError(l, "incorrect argument");
		}
		const char *key = LuaToString(l, j + 1, 1);
#if 0 // To be removed. must modify lua file.
		if (!strcmp(key, "attack-range")) {
			key = "AttackRange";
		} else if (!strcmp(key, "sight-range")) {
			key = "SightRange";
		} else if (!strcmp(key, "basic-damage")) {
			key = "BasicDamage";
		} else if (!strcmp(key, "piercing-damage")) {
			key = "PiercingDamage";
		} else if (!strcmp(key, "armor")) {
			key = "Armor";
		} else if (!strcmp(key, "hit-points")) {
			key = "HitPoints";
		}
#endif
		if (!strcmp(key, "regeneration-rate")) {
			um->Modifier.Variables[HP_INDEX].Increase = LuaToNumber(l, j + 1, 2);
		} else if (!strcmp(key, "cost")) {
			//Wyrmgus start
//			if (!lua_istable(l, j + 1) || lua_rawlen(l, j + 1) != 2) {
			if (!lua_istable(l, j + 1) || lua_rawlen(l, j + 1) != 3) {
			//Wyrmgus end
				LuaError(l, "incorrect argument");
			}
			//Wyrmgus start
//			const char *value = LuaToString(l, j + 1, 1);
			const char *value = LuaToString(l, j + 1, 2);
			//Wyrmgus end
			const int resId = GetResourceIdByName(l, value);
			//Wyrmgus start
//			um->Modifier.Costs[resId] = LuaToNumber(l, j + 1, 2);
			um->Modifier.Costs[resId] = LuaToNumber(l, j + 1, 3);
			//Wyrmgus end
		} else if (!strcmp(key, "storing")) {
			if (!lua_istable(l, j + 1) || lua_rawlen(l, j + 1) != 2) {
				LuaError(l, "incorrect argument");
			}
			const char *value = LuaToString(l, j + 1, 1);
			const int resId = GetResourceIdByName(l, value);
			um->Modifier.Storing[resId] = LuaToNumber(l, j + 1, 2);
		} else if (!strcmp(key, "improve-production")) {
			const char *value = LuaToString(l, j + 1, 2);
			const int resId = GetResourceIdByName(l, value);
			um->Modifier.ImproveIncomes[resId] = LuaToNumber(l, j + 1, 3);
		//Wyrmgus start
		} else if (!strcmp(key, "resource-demand")) {
			const char *value = LuaToString(l, j + 1, 2);
			const int resId = GetResourceIdByName(l, value);
			um->Modifier.ResourceDemand[resId] = LuaToNumber(l, j + 1, 3);
		} else if (!strcmp(key, "unit-stock")) {
			std::string value = LuaToString(l, j + 1, 2);
			const int unit_type_id = UnitTypeIdByIdent(value);
			um->Modifier.UnitStock[unit_type_id] = LuaToNumber(l, j + 1, 3);
		//Wyrmgus end
		} else if (!strcmp(key, "allow-unit")) {
			const char *value = LuaToString(l, j + 1, 2);

			if (!strncmp(value, "unit-", 5)) {
				um->ChangeUnits[UnitTypeIdByIdent(value)] = LuaToNumber(l, j + 1, 3);
			} else {
				LuaError(l, "unit expected");
			}
		} else if (!strcmp(key, "allow")) {
			const char *value = LuaToString(l, j + 1, 2);
			if (!strncmp(value, "upgrade-", 8)) {
				//Wyrmgus start
//				um->ChangeUpgrades[UpgradeIdByIdent(value)] = LuaToNumber(l, j + 1, 3);
				um->ChangeUpgrades[UpgradeIdByIdent(value)] = *strdup(LuaToString(l, j + 1, 3));
				//Wyrmgus end
			} else {
				LuaError(l, "upgrade expected");
			}
		//Wyrmgus start
		} else if (!strcmp(key, "remove-upgrade")) {
			const char *value = LuaToString(l, j + 1, 2);
			if (!strncmp(value, "upgrade-", 8)) {
				CUpgrade *removed_upgrade = CUpgrade::Get(value);
				if (removed_upgrade) {
					um->RemoveUpgrades.push_back(removed_upgrade);
				} else {
					LuaError(l, "Upgrade \"%s\" doesn't exist.'" _C_ value);
				}
			} else {
				LuaError(l, "upgrade expected");
			}
		//Wyrmgus end
		} else if (!strcmp(key, "apply-to")) {
			const char *value = LuaToString(l, j + 1, 2);
			um->ApplyTo[UnitTypeIdByIdent(value)] = 'X';
		} else if (!strcmp(key, "convert-to")) {
			const char *value = LuaToString(l, j + 1, 2);
			um->ConvertTo = UnitTypeByIdent(value);
		//Wyrmgus start
		} else if (!strcmp(key, "research-speed")) {
			um->SpeedResearch = LuaToNumber(l, j + 1, 2);
		} else if (!strcmp(key, "change-civilization-to")) {
			const char *civilization_ident = LuaToString(l, j + 1, 2);
			um->ChangeCivilizationTo = PlayerRaces.GetRaceIndexByName(civilization_ident);
			
			if (um->ChangeCivilizationTo == -1) {
				LuaError(l, "invalid civilization name '%s'" _C_ civilization_ident);
			}
		} else if (!strcmp(key, "change-faction-to")) {
			std::string faction_ident = LuaToString(l, j + 1, 2);
			um->ChangeFactionTo = PlayerRaces.GetFaction(faction_ident);
			
			if (um->ChangeFactionTo == NULL) {
				LuaError(l, "Faction \"%s\" doesn't exist.'" _C_ faction_ident.c_str());
			}
		} else if (!strcmp(key, "change-dynasty-to")) {
			std::string dynasty_ident = LuaToString(l, j + 1, 2);
			um->ChangeDynastyTo = PlayerRaces.GetDynasty(dynasty_ident);
			
			if (um->ChangeDynastyTo == NULL) {
				LuaError(l, "Dynasty \"%s\" doesn't exist.'" _C_ dynasty_ident.c_str());
			}
		//Wyrmgus end
		} else {
			int index = UnitTypeVar.VariableNameLookup[key]; // variable index;
			if (index != -1) {
				if (lua_rawlen(l, j + 1) == 3) {
					const char *value = LuaToString(l, j + 1, 3);
					if (!strcmp(value, "Percent")) {
						um->ModifyPercent[index] = LuaToNumber(l, j + 1, 2);
					//Wyrmgus start
					} else if (!strcmp(value, "Increase")) {
						um->Modifier.Variables[index].Increase = LuaToNumber(l, j + 1, 2);
					//Wyrmgus end
					}
				} else {
					lua_rawgeti(l, j + 1, 2);
					if (lua_istable(l, -1)) {
						DefineVariableField(l, um->Modifier.Variables + index, -1);
					} else if (lua_isnumber(l, -1)) {
						um->Modifier.Variables[index].Enable = 1;
						um->Modifier.Variables[index].Value = LuaToNumber(l, -1);
						um->Modifier.Variables[index].Max = LuaToNumber(l, -1);
					} else {
						LuaError(l, "bad argument type for '%s'\n" _C_ key);
					}
					lua_pop(l, 1);
				}
			} else {
				LuaError(l, "wrong tag: %s" _C_ key);
			}
		}
	}

	UpgradeModifiers[NumUpgradeModifiers++] = um;
	
	return 0;
}

/**
**  Define which units are allowed and how much.
*/
static int CclDefineUnitAllow(lua_State *l)
{
	const int args = lua_gettop(l);

	const char *ident = LuaToString(l, 0 + 1);

	if (strncmp(ident, "unit-", 5)) {
		DebugPrint(" wrong ident %s\n" _C_ ident);
		return 0;
	}
	int id = UnitTypeIdByIdent(ident);

	int i = 0;
	for (int j = 1; j < args && i < PlayerMax; ++j) {
		AllowUnitId(Players[i], id, LuaToNumber(l, j + 1));
		++i;
	}
	return 0;
}

/**
**  Define which units/upgrades are allowed.
*/
static int CclDefineAllow(lua_State *l)
{
	const int UnitMax = 65536; /// How many units supported
	const int args = lua_gettop(l);

	for (int j = 0; j < args; ++j) {
		const char *ident = LuaToString(l, j + 1);
		++j;
		const char *ids = LuaToString(l, j + 1);

		int n = strlen(ids);
		if (n > PlayerMax) {
			fprintf(stderr, "%s: Allow string too long %d\n", ident, n);
			n = PlayerMax;
		}

		if (!strncmp(ident, "unit-", 5)) {
			int id = UnitTypeIdByIdent(ident);
			for (int i = 0; i < n; ++i) {
				if (ids[i] == 'A') {
					AllowUnitId(Players[i], id, UnitMax);
				} else if (ids[i] == 'F') {
					AllowUnitId(Players[i], id, 0);
				}
			}
		} else if (!strncmp(ident, "upgrade-", 8)) {
			int id = UpgradeIdByIdent(ident);
			for (int i = 0; i < n; ++i) {
				AllowUpgradeId(Players[i], id, ids[i]);
			}
		} else {
			DebugPrint(" wrong ident %s\n" _C_ ident);
		}
	}
	return 0;
}

//Wyrmgus start
/**
** Acquire an ability
*/
static int CclAcquireAbility(lua_State *l)
{
	LuaCheckArgs(l, 2);
	
	if (lua_isnil(l, 1)) {
		return 0;
	}
	
	lua_pushvalue(l, 1);
	CUnit *unit = &UnitManager.GetSlotUnit(LuaToNumber(l, -1));
	lua_pop(l, 1);
	const char *ident = LuaToString(l, 2);
	if (!strncmp(ident, "upgrade-", 8)) {
		AbilityAcquire(*unit, CUpgrade::Get(ident));
	} else {
		DebugPrint(" wrong ident %s\n" _C_ ident);
	}
	return 0;
}

/**
** Set the unit's trait
*/
static int CclAcquireTrait(lua_State *l)
{
	LuaCheckArgs(l, 2);
	
	if (lua_isnil(l, 1)) {
		return 0;
	}
	
	lua_pushvalue(l, 1);
	CUnit *unit = &UnitManager.GetSlotUnit(LuaToNumber(l, -1));
	lua_pop(l, 1);
	const char *ident = LuaToString(l, 2);
	if (!strncmp(ident, "upgrade-", 8)) {
		TraitAcquire(*unit, CUpgrade::Get(ident));
	} else if (strlen(ident) == 0) {
		if (unit->Trait != NULL) { //remove previous trait, if any
			if (!GameSettings.NoRandomness) { // if in no randomness setting, don't apply trait modifiers
				IndividualUpgradeLost(*unit, unit->Trait);
			}
		}
		unit->Trait = NULL;
	} else {
		DebugPrint(" wrong ident %s\n" _C_ ident);
	}
	return 0;
}

static int CclGetUpgrades(lua_State *l)
{
	lua_createtable(l, AllUpgrades.size(), 0);
	for (size_t i = 1; i <= AllUpgrades.size(); ++i)
	{
		lua_pushstring(l, AllUpgrades[i-1]->Ident.c_str());
		lua_rawseti(l, -2, i);
	}
	return 1;
}

static int CclGetItemPrefixes(lua_State *l)
{
	std::vector<CUpgrade *> item_prefixes;
	for (size_t i = 0; i < AllUpgrades.size(); ++i) {
		if (AllUpgrades[i]->MagicPrefix) {
			item_prefixes.push_back(AllUpgrades[i]);
		}
	}
		
	lua_createtable(l, item_prefixes.size(), 0);
	for (size_t i = 1; i <= item_prefixes.size(); ++i)
	{
		lua_pushstring(l, item_prefixes[i-1]->Ident.c_str());
		lua_rawseti(l, -2, i);
	}
	return 1;
}

static int CclGetItemSuffixes(lua_State *l)
{
	std::vector<CUpgrade *> item_suffixes;
	for (size_t i = 0; i < AllUpgrades.size(); ++i) {
		if (AllUpgrades[i]->MagicSuffix && !AllUpgrades[i]->RunicAffix) {
			item_suffixes.push_back(AllUpgrades[i]);
		}
	}
		
	lua_createtable(l, item_suffixes.size(), 0);
	for (size_t i = 1; i <= item_suffixes.size(); ++i)
	{
		lua_pushstring(l, item_suffixes[i-1]->Ident.c_str());
		lua_rawseti(l, -2, i);
	}
	return 1;
}

static int CclGetRunicSuffixes(lua_State *l)
{
	std::vector<CUpgrade *> item_suffixes;
	for (size_t i = 0; i < AllUpgrades.size(); ++i) {
		if (AllUpgrades[i]->MagicSuffix && AllUpgrades[i]->RunicAffix) {
			item_suffixes.push_back(AllUpgrades[i]);
		}
	}
		
	lua_createtable(l, item_suffixes.size(), 0);
	for (size_t i = 1; i <= item_suffixes.size(); ++i)
	{
		lua_pushstring(l, item_suffixes[i-1]->Ident.c_str());
		lua_rawseti(l, -2, i);
	}
	return 1;
}

static int CclGetLiteraryWorks(lua_State *l)
{
	std::vector<CUpgrade *> literary_works;
	for (size_t i = 0; i < AllUpgrades.size(); ++i) {
		if (AllUpgrades[i]->Work != -1) {
			literary_works.push_back(AllUpgrades[i]);
		}
	}
		
	lua_createtable(l, literary_works.size(), 0);
	for (size_t i = 1; i <= literary_works.size(); ++i)
	{
		lua_pushstring(l, literary_works[i-1]->Ident.c_str());
		lua_rawseti(l, -2, i);
	}
	return 1;
}

/**
**  Get upgrade data.
**
**  @param l  Lua state.
*/
static int CclGetUpgradeData(lua_State *l)
{
	const int nargs = lua_gettop(l);
	if (nargs < 2) {
		LuaError(l, "incorrect argument");
	}
	std::string upgrade_ident = LuaToString(l, 1);
	const CUpgrade *upgrade = CUpgrade::Get(upgrade_ident);
	if (!upgrade) {
		LuaError(l, "Upgrade \"%s\" doesn't exist." _C_ upgrade_ident.c_str());
	}
	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Name")) {
		lua_pushstring(l, upgrade->Name.c_str());
		return 1;
	} else if (!strcmp(data, "Class")) {
		if (upgrade->Class != -1) {
			lua_pushstring(l, UpgradeClasses[upgrade->Class].c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Civilization")) {
		if (upgrade->Civilization != -1) {
			lua_pushstring(l, PlayerRaces.Name[upgrade->Civilization].c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Faction")) {
		if (upgrade->Faction != -1) {
			lua_pushstring(l, PlayerRaces.Factions[upgrade->Faction]->Ident.c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Description")) {
		lua_pushstring(l, upgrade->Description.c_str());
		return 1;
	} else if (!strcmp(data, "Background")) {
		lua_pushstring(l, upgrade->Background.c_str());
		return 1;
	} else if (!strcmp(data, "Quote")) {
		lua_pushstring(l, upgrade->Quote.c_str());
		return 1;
	} else if (!strcmp(data, "EffectsString")) {
		lua_pushstring(l, upgrade->EffectsString.c_str());
		return 1;
	} else if (!strcmp(data, "RequirementsString")) {
		lua_pushstring(l, upgrade->RequirementsString.c_str());
		return 1;
	} else if (!strcmp(data, "ItemPrefix")) {
		if (nargs == 2) { //check if the upgrade is a prefix for any item type
			if (upgrade->MagicPrefix) {
				lua_pushboolean(l, true);
				return 1;
			} else {
				lua_pushboolean(l, false);
				return 1;
			}
		} else {
			LuaCheckArgs(l, 3);
			std::string item_class_name = LuaToString(l, 3);
			int item_class = GetItemClassIdByName(item_class_name);
			if (item_class == -1) {
				LuaError(l, "Item class \"%s\" doesn't exist." _C_ item_class_name.c_str());
			}
			lua_pushboolean(l, upgrade->ItemPrefix[item_class]);
			return 1;
		}
	} else if (!strcmp(data, "ItemSuffix")) {
		if (nargs == 2) { //check if the item is a suffix for any item type
			if (upgrade->MagicSuffix) {
				lua_pushboolean(l, true);
				return 1;
			} else {
				lua_pushboolean(l, false);
				return 1;
			}
		} else {
			LuaCheckArgs(l, 3);
			std::string item_class_name = LuaToString(l, 3);
			int item_class = GetItemClassIdByName(item_class_name);
			if (item_class == -1) {
				LuaError(l, "Item class \"%s\" doesn't exist." _C_ item_class_name.c_str());
			}
			lua_pushboolean(l, upgrade->ItemSuffix[item_class]);
			return 1;
		}
	} else if (!strcmp(data, "AppliesTo")) { //to which unit types or item classes this upgrade applies
		std::vector<std::string> applies_to;
		for (int i = 0; i < MaxItemClasses; ++i) {
			if (upgrade->ItemPrefix[i] || upgrade->ItemSuffix[i]) {
				applies_to.push_back(GetItemClassNameById(i));
			}
		}

		for (size_t z = 0; z < upgrade->UpgradeModifiers.size(); ++z) {
			for (size_t i = 0; i < UnitTypes.size(); ++i) {
				if (UnitTypes[i]->Ident.find("template") != std::string::npos) { //if is a template, continue
					continue;
				}
				
				if (upgrade->UpgradeModifiers[z]->ApplyTo[i] == 'X' || std::find(UnitTypes[i]->Affixes.begin(), UnitTypes[i]->Affixes.end(), upgrade) != UnitTypes[i]->Affixes.end()) {
					applies_to.push_back(UnitTypes[i]->Ident);
				}
			}
		}
		
		lua_createtable(l, applies_to.size(), 0);
		for (size_t i = 1; i <= applies_to.size(); ++i)
		{
			lua_pushstring(l, applies_to[i-1].c_str());
			lua_rawseti(l, -2, i);
		}
		return 1;
	} else {
		LuaError(l, "Invalid field: %s" _C_ data);
	}

	return 0;
}
//Wyrmgus end

/**
**  Register CCL features for upgrades.
*/
void UpgradesCclRegister()
{
	//Wyrmgus start
	lua_register(Lua, "DefineUpgrade", CclDefineUpgrade);
	//Wyrmgus end
	lua_register(Lua, "DefineModifier", CclDefineModifier);
	lua_register(Lua, "DefineAllow", CclDefineAllow);
	lua_register(Lua, "DefineUnitAllow", CclDefineUnitAllow);
	//Wyrmgus start
	lua_register(Lua, "AcquireAbility", CclAcquireAbility);
	lua_register(Lua, "AcquireTrait", CclAcquireTrait);
	lua_register(Lua, "GetUpgrades", CclGetUpgrades);
	lua_register(Lua, "GetItemPrefixes", CclGetItemPrefixes);
	lua_register(Lua, "GetItemSuffixes", CclGetItemSuffixes);
	lua_register(Lua, "GetRunicSuffixes", CclGetRunicSuffixes);
	lua_register(Lua, "GetLiteraryWorks", CclGetLiteraryWorks);
	lua_register(Lua, "GetUpgradeData", CclGetUpgradeData);
	//Wyrmgus end
}

/*----------------------------------------------------------------------------
-- General/Map functions
----------------------------------------------------------------------------*/

// AllowStruct and UpgradeTimers will be static in the player so will be
// load/saved with the player struct

/**
**  UnitType ID by identifier.
**
**  @param ident  The unit-type identifier.
**  @return       Unit-type ID (int) or -1 if not found.
*/
int UnitTypeIdByIdent(const std::string &ident)
{
	const CUnitType *type = UnitTypeByIdent(ident);

	if (type) {
		return type->Slot;
	}
	DebugPrint(" fix this %s\n" _C_ ident.c_str());
	Assert(0);
	return -1;
}

/**
**  Upgrade ID by identifier.
**
**  @param ident  The upgrade identifier.
**  @return       Upgrade ID (int) or -1 if not found.
*/
int UpgradeIdByIdent(const std::string &ident)
{
	const CUpgrade *upgrade = CUpgrade::Get(ident);

	if (upgrade) {
		return upgrade->ID;
	}
	DebugPrint(" fix this %s\n" _C_ ident.c_str());
	return -1;
}

/*----------------------------------------------------------------------------
-- Upgrades
----------------------------------------------------------------------------*/

/**
**  Convert unit-type to.
**
**  @param player  For this player.
**  @param src     From this unit-type.
**  @param dst     To this unit-type.
*/
static void ConvertUnitTypeTo(CPlayer &player, const CUnitType &src, CUnitType &dst)
{
	//Wyrmgus start
	if (player.AiEnabled && GameCycle > 0) {
		//if is AI player, convert all requests from the old unit type to the new one; FIXME: if already has requests of the new unit type, then the count of the old one should be added to the new one, instead of merely changing the type of the old one to the new one
		for (unsigned int i = 0; i < player.Ai->UnitTypeRequests.size(); ++i) {
			if (player.Ai->UnitTypeRequests[i].Type->Slot == src.Slot) {
				player.Ai->UnitTypeRequests[i].Type = &dst;
			}
		}

		for (unsigned int i = 0; i < player.Ai->UpgradeToRequests.size(); ++i) {
			if (player.Ai->UpgradeToRequests[i]->Slot == src.Slot) {
				player.Ai->UpgradeToRequests[i] = &dst;
			}
		}
		
		for (unsigned int i = 0; i < player.Ai->Force.Size(); ++i) {
			AiForce &force = player.Ai->Force[i];

			for (unsigned int j = 0; j < force.UnitTypes.size(); ++j) {
				if (force.UnitTypes[j].Type->Slot == src.Slot) {
					force.UnitTypes[j].Type = &dst;
				}
			}
		}

		for (unsigned int i = 0; i < player.Ai->UnitTypeBuilt.size(); ++i) {
			if (player.Ai->UnitTypeBuilt[i].Type->Slot == src.Slot) {
				player.Ai->UnitTypeBuilt[i].Type = &dst;
			}
		}
	}
	//Wyrmgus end
	for (int i = 0; i < player.GetUnitCount(); ++i) {
		CUnit &unit = player.GetUnit(i);

		//Wyrmgus start
		if (&unit == NULL) {
			fprintf(stderr, "Error in ConvertUnitTypeTo: unit %d, of player %d is NULL.\n", i, player.Index);
			continue;
		}
		//Wyrmgus end

		//  Convert already existing units to this type.
		//Wyrmgus start
//		if (unit.Type == &src) {
		if (unit.Type == &src && !unit.Character) { //don't do this for persistent characters
		//Wyrmgus end
			CommandTransformIntoType(unit, dst);
			//  Convert trained units to this type.
			//  FIXME: what about buildings?
		//Wyrmgus start
//		} else {
		} else if (GameCycle > 0) {
		//Wyrmgus end
			//Wyrmgus start
			// convert transformation order
			if (unit.CriticalOrder && unit.CriticalOrder->Action == UnitActionTransformInto) {
				COrder_TransformInto &order = *static_cast<COrder_TransformInto *>(unit.CriticalOrder);

				if (&order.GetUnitType() == &src) {
					order.ConvertUnitType(unit, dst);
				}
			}
			//Wyrmgus end
			
			for (size_t j = 0; j < unit.Orders.size(); ++j) {
				if (unit.Orders[j]->Action == UnitActionTrain) {
					COrder_Train &order = *static_cast<COrder_Train *>(unit.Orders[j]);

					if (&order.GetUnitType() == &src) {
						order.ConvertUnitType(unit, dst);
					}
				//Wyrmgus start
				// convert building orders as well
				} else if (unit.Orders[j]->Action == UnitActionBuild) {
					COrder_Build &order = *static_cast<COrder_Build *>(unit.Orders[j]);

					if (&order.GetUnitType() == &src) {
						order.ConvertUnitType(unit, dst);
					}
				// also convert upgrade orders
				} else if (unit.Orders[j]->Action == UnitActionUpgradeTo) {
					COrder_UpgradeTo &order = *static_cast<COrder_UpgradeTo *>(unit.Orders[j]);

					if (&order.GetUnitType() == &src) {
						order.ConvertUnitType(unit, dst);
					}
				//Wyrmgus end
				}
			}
		}
	}
}

/**
**  Apply the modifiers of an upgrade.
**
**  This function will mark upgrade done and do all required modifications
**  to unit types and will modify allow/forbid maps
**
**  @param player  Player that get all the upgrades.
**  @param um      Upgrade modifier that do the effects
*/
static void ApplyUpgradeModifier(CPlayer &player, const CUpgradeModifier *um)
{
	Assert(um);

	int pn = player.Index;

	//Wyrmgus start
	if (um->SpeedResearch != 0) {
		player.SpeedResearch += um->SpeedResearch;
	}
	if (um->ChangeCivilizationTo != -1 && GameRunning && um->ChangeCivilizationTo != player.Race) {
		player.SetCivilization(um->ChangeCivilizationTo);
	}
	if (um->ChangeFactionTo != NULL && GameRunning && (um->ChangeFactionTo->Civilization != player.Race || um->ChangeFactionTo->ID != player.Faction)) {
		if (um->ChangeFactionTo->Civilization != player.Race) {
			player.SetCivilization(um->ChangeFactionTo->Civilization);
		}
		player.SetFaction(um->ChangeFactionTo);
	}
	if (um->ChangeDynastyTo != NULL && um->ChangeDynastyTo != player.Dynasty) {
		player.SetDynasty(um->ChangeDynastyTo);
	}
	//Wyrmgus end

	for (int z = 0; z < UpgradeMax; ++z) {
		// allow/forbid upgrades for player.  only if upgrade is not acquired

		// FIXME: check if modify is allowed

		if (player.Allow.Upgrades[z] != 'R') {
			if (um->ChangeUpgrades[z] == 'A') {
				player.Allow.Upgrades[z] = 'A';
			}
			if (um->ChangeUpgrades[z] == 'F') {
				player.Allow.Upgrades[z] = 'F';
			}
			// we can even have upgrade acquired w/o costs
			if (um->ChangeUpgrades[z] == 'R') {
				player.Allow.Upgrades[z] = 'R';
			}
		}
	}
	
	//Wyrmgus start
	for (size_t i = 0; i < um->RemoveUpgrades.size(); ++i) {
		if (player.Allow.Upgrades[um->RemoveUpgrades[i]->ID] == 'R') {
			UpgradeLost(player, um->RemoveUpgrades[i]->ID);
		}
	}
	//Wyrmgus end

	for (size_t z = 0; z < UnitTypes.size(); ++z) {
		CUnitStats &stat = UnitTypes[z]->Stats[pn];
		// add/remove allowed units

		//Wyrmgus start
		if (stat.Variables == NULL) { // unit type's stats not initialized
			break;
		}
		//Wyrmgus end

		// FIXME: check if modify is allowed

		player.Allow.Units[z] += um->ChangeUnits[z];

		Assert(um->ApplyTo[z] == '?' || um->ApplyTo[z] == 'X');

		// this modifier should be applied to unittype id == z
		if (um->ApplyTo[z] == 'X') {

			// if a unit type's supply is changed, we need to update the player's supply accordingly
			if (um->Modifier.Variables[SUPPLY_INDEX].Value) {
				std::vector<CUnit *> unitupgrade;

				FindUnitsByType(*UnitTypes[z], unitupgrade);
				for (size_t j = 0; j != unitupgrade.size(); ++j) {
					CUnit &unit = *unitupgrade[j];
					if (unit.Player->Index == pn && unit.IsAlive()) {
						unit.Player->Supply += um->Modifier.Variables[SUPPLY_INDEX].Value;
					}
				}
			}
			
			// if a unit type's demand is changed, we need to update the player's demand accordingly
			if (um->Modifier.Variables[DEMAND_INDEX].Value) {
				std::vector<CUnit *> unitupgrade;

				FindUnitsByType(*UnitTypes[z], unitupgrade);
				for (size_t j = 0; j != unitupgrade.size(); ++j) {
					CUnit &unit = *unitupgrade[j];
					if (unit.Player->Index == pn && unit.IsAlive()) {
						unit.Player->Demand += um->Modifier.Variables[DEMAND_INDEX].Value;
					}
				}
			}
			
			// upgrade costs :)
			for (unsigned int j = 0; j < MaxCosts; ++j) {
				stat.Costs[j] += um->Modifier.Costs[j];
				stat.Storing[j] += um->Modifier.Storing[j];
				if (um->Modifier.ImproveIncomes[j]) {
					if (!stat.ImproveIncomes[j]) {
						stat.ImproveIncomes[j] += DefaultIncomes[j] + um->Modifier.ImproveIncomes[j];
					} else {
						stat.ImproveIncomes[j] += um->Modifier.ImproveIncomes[j];
					}
					//update player's income
					std::vector<CUnit *> unitupgrade;
					FindUnitsByType(*UnitTypes[z], unitupgrade);
					if (unitupgrade.size() > 0) {
						player.Incomes[j] = std::max(player.Incomes[j], stat.ImproveIncomes[j]);
					}
				}
				//Wyrmgus start
				stat.ResourceDemand[j] += um->Modifier.ResourceDemand[j];
				//Wyrmgus end
			}
			
			//Wyrmgus start
			for (size_t j = 0; j < UnitTypes.size(); ++j) {
				if (um->Modifier.UnitStock[j]) {
					stat.UnitStock[j] += um->Modifier.UnitStock[j];
				}
			}
			//Wyrmgus end

			int varModified = 0;
			for (unsigned int j = 0; j < UnitTypeVar.GetNumberVariable(); j++) {
				varModified |= um->Modifier.Variables[j].Value
							   | um->Modifier.Variables[j].Max
							   | um->Modifier.Variables[j].Increase
							   | um->Modifier.Variables[j].Enable
							   | um->ModifyPercent[j];
				stat.Variables[j].Enable |= um->Modifier.Variables[j].Enable;
				if (um->ModifyPercent[j]) {
					stat.Variables[j].Value += stat.Variables[j].Value * um->ModifyPercent[j] / 100;
					stat.Variables[j].Max += stat.Variables[j].Max * um->ModifyPercent[j] / 100;
				} else {
					stat.Variables[j].Value += um->Modifier.Variables[j].Value;
					stat.Variables[j].Max += um->Modifier.Variables[j].Max;
					stat.Variables[j].Increase += um->Modifier.Variables[j].Increase;
				}

				stat.Variables[j].Max = std::max(stat.Variables[j].Max, 0);
				//Wyrmgus start
//				clamp(&stat.Variables[j].Value, 0, stat.Variables[j].Max);
				if (stat.Variables[j].Max > 0) {
					clamp(&stat.Variables[j].Value, 0, stat.Variables[j].Max);
				}
				//Wyrmgus end
			}

			// And now modify ingame units
			//Wyrmgus start
			std::vector<CUnit *> unitupgrade;

			FindUnitsByType(*UnitTypes[z], unitupgrade, true);
			//Wyrmgus end
			
			if (varModified) {
				//Wyrmgus start
//				std::vector<CUnit *> unitupgrade;

//				FindUnitsByType(*UnitTypes[z], unitupgrade, true);
				//Wyrmgus end
				for (size_t j = 0; j != unitupgrade.size(); ++j) {
					CUnit &unit = *unitupgrade[j];

					if (unit.Player->Index != player.Index) {
						continue;
					}
					
					//Wyrmgus start
					if (
						(AllUpgrades[um->UpgradeId]->Weapon && unit.EquippedItems[WeaponItemSlot].size() > 0)
						|| (AllUpgrades[um->UpgradeId]->Shield && unit.EquippedItems[ShieldItemSlot].size() > 0)
						|| (AllUpgrades[um->UpgradeId]->Boots && unit.EquippedItems[BootsItemSlot].size() > 0)
						|| (AllUpgrades[um->UpgradeId]->Arrows && unit.EquippedItems[ArrowsItemSlot].size() > 0)
					) { //if the unit already has an item equipped of the same equipment type as this upgrade, don't apply the modifier to it
						continue;
					}
					
					if (unit.Character && !strncmp(AllUpgrades[um->UpgradeId]->Ident.c_str(), "upgrade-deity-", 14)) { //heroes choose their own deities
						continue;
					}
					//Wyrmgus end
					
					for (unsigned int j = 0; j < UnitTypeVar.GetNumberVariable(); j++) {
						unit.Variable[j].Enable |= um->Modifier.Variables[j].Enable;
						if (um->ModifyPercent[j]) {
							unit.Variable[j].Value += unit.Variable[j].Value * um->ModifyPercent[j] / 100;
							unit.Variable[j].Max += unit.Variable[j].Max * um->ModifyPercent[j] / 100;
						} else {
							unit.Variable[j].Value += um->Modifier.Variables[j].Value;
							unit.Variable[j].Increase += um->Modifier.Variables[j].Increase;
						}

						unit.Variable[j].Max += um->Modifier.Variables[j].Max;
						unit.Variable[j].Max = std::max(unit.Variable[j].Max, 0);
						if (unit.Variable[j].Max > 0) {
							//Wyrmgus start
//							clamp(&unit.Variable[j].Value, 0, unit.Variable[j].Max);
							clamp(&unit.Variable[j].Value, 0, unit.GetModifiedVariable(j, VariableMax));
							//Wyrmgus end
						}
						//Wyrmgus start
						if (j == ATTACKRANGE_INDEX && unit.Container) {
							unit.Container->UpdateContainerAttackRange();
						} else if (j == LEVEL_INDEX || j == POINTS_INDEX) {
							unit.UpdateXPRequired();
						} else if (IsKnowledgeVariable(j)) {
							unit.CheckKnowledgeChange(j, um->Modifier.Variables[j].Value);
						} else if (j == SIGHTRANGE_INDEX && !unit.Removed) {
							// If Sight range is upgraded, we need to change EVERY unit
							// to the new range, otherwise the counters get confused.
							MapUnmarkUnitSight(unit);
							UpdateUnitSightRange(unit);
							MapMarkUnitSight(unit);
						}
						//Wyrmgus end
					}
					
					//Wyrmgus start
					for (size_t j = 0; j < UnitTypes.size(); ++j) {
						if (um->Modifier.UnitStock[j] < 0) {
							if (unit.UnitStock.find(j) == unit.UnitStock.end()) {
								unit.UnitStock[j] = 0;
							}
							unit.UnitStock[j] += um->Modifier.UnitStock[j];
							unit.UnitStock[j] = std::max(unit.UnitStock[j], 0);
						}
					}
					//Wyrmgus end
				}
			}
			
			//Wyrmgus start
			for (size_t j = 0; j != unitupgrade.size(); ++j) {
				CUnit &unit = *unitupgrade[j];

				if (unit.Player->Index != player.Index) {
					continue;
				}
				
				//add or remove starting abilities from the unit if the upgrade enabled/disabled them
				for (size_t i = 0; i < unit.Type->StartingAbilities.size(); ++i) {
					if (!unit.IndividualUpgrades[unit.Type->StartingAbilities[i]->ID] && CheckDependByIdent(unit, unit.Type->StartingAbilities[i]->Ident)) {
						IndividualUpgradeAcquire(unit, unit.Type->StartingAbilities[i]);
					} else if (unit.IndividualUpgrades[unit.Type->StartingAbilities[i]->ID] && !CheckDependByIdent(unit, unit.Type->StartingAbilities[i]->Ident)) {
						IndividualUpgradeLost(unit, unit.Type->StartingAbilities[i]);
					}
				}
				
				//change variation if current one becomes forbidden
				VariationInfo *current_varinfo = UnitTypes[z]->VarInfo[unit.Variation];
				if (current_varinfo) {
					bool forbidden_upgrade = false;
					for (int u = 0; u < VariationMax; ++u) {
						if (!current_varinfo->UpgradesForbidden[u].empty() && um->UpgradeId == CUpgrade::Get(current_varinfo->UpgradesForbidden[u])->ID) {
							forbidden_upgrade = true;
							break;
						}
					}
					if (forbidden_upgrade == true) {
						unit.ChooseVariation();
					}
				}
				for (int i = 0; i < MaxImageLayers; ++i) {
					if (unit.LayerVariation[i] != -1 && unit.LayerVariation[i] < ((int) unit.Type->LayerVarInfo[i].size())) {
						VariationInfo *current_layer_varinfo = UnitTypes[z]->LayerVarInfo[i][unit.LayerVariation[i]];
						bool forbidden_upgrade = false;
						for (int u = 0; u < VariationMax; ++u) {
							if (!current_layer_varinfo->UpgradesForbidden[u].empty() && um->UpgradeId == CUpgrade::Get(current_layer_varinfo->UpgradesForbidden[u])->ID) {
								forbidden_upgrade = true;
								break;
							}
						}
						if (forbidden_upgrade == true) {
							unit.ChooseVariation(NULL, false, i);
						}
					}
				}
				unit.UpdateButtonIcons();
			}
			//Wyrmgus end
			
			if (um->ConvertTo) {
				ConvertUnitTypeTo(player, *UnitTypes[z], *um->ConvertTo);
			}
		}
	}
}

/**
**  Remove the modifiers of an upgrade.
**
**  This function will unmark upgrade as done and undo all required modifications
**  to unit types and will modify allow/forbid maps back
**
**  @param player  Player that get all the upgrades.
**  @param um      Upgrade modifier that do the effects
*/
static void RemoveUpgradeModifier(CPlayer &player, const CUpgradeModifier *um)
{
	Assert(um);

	int pn = player.Index;

	if (um->SpeedResearch != 0) {
		player.SpeedResearch -= um->SpeedResearch;
	}

	for (int z = 0; z < UpgradeMax; ++z) {
		// allow/forbid upgrades for player.  only if upgrade is not acquired

		// FIXME: check if modify is allowed

		if (player.Allow.Upgrades[z] != 'R') {
			if (um->ChangeUpgrades[z] == 'A') {
				player.Allow.Upgrades[z] = 'F';
			}
			if (um->ChangeUpgrades[z] == 'F') {
				player.Allow.Upgrades[z] = 'A';
			}
			// we can even have upgrade acquired w/o costs
			if (um->ChangeUpgrades[z] == 'R') {
				player.Allow.Upgrades[z] = 'A';
			}
		}
	}

	for (size_t z = 0; z < UnitTypes.size(); ++z) {
		CUnitStats &stat = UnitTypes[z]->Stats[pn];
		// add/remove allowed units

		//Wyrmgus start
		if (stat.Variables == NULL) { // unit types stats not initialized
			break;
		}
		//Wyrmgus end
		
		// FIXME: check if modify is allowed

		player.Allow.Units[z] -= um->ChangeUnits[z];

		Assert(um->ApplyTo[z] == '?' || um->ApplyTo[z] == 'X');

		// this modifier should be applied to unittype id == z
		if (um->ApplyTo[z] == 'X') {
			// if a unit type's supply is changed, we need to update the player's supply accordingly
			if (um->Modifier.Variables[SUPPLY_INDEX].Value) {
				std::vector<CUnit *> unitupgrade;

				FindUnitsByType(*UnitTypes[z], unitupgrade);
				for (size_t j = 0; j != unitupgrade.size(); ++j) {
					CUnit &unit = *unitupgrade[j];
					if (unit.Player->Index == pn && unit.IsAlive()) {
						unit.Player->Supply -= um->Modifier.Variables[SUPPLY_INDEX].Value;
					}
				}
			}
			
			// if a unit type's demand is changed, we need to update the player's demand accordingly
			if (um->Modifier.Variables[DEMAND_INDEX].Value) {
				std::vector<CUnit *> unitupgrade;

				FindUnitsByType(*UnitTypes[z], unitupgrade);
				for (size_t j = 0; j != unitupgrade.size(); ++j) {
					CUnit &unit = *unitupgrade[j];
					if (unit.Player->Index == pn && unit.IsAlive()) {
						unit.Player->Demand -= um->Modifier.Variables[DEMAND_INDEX].Value;
					}
				}
			}
			
			// upgrade costs :)
			for (unsigned int j = 0; j < MaxCosts; ++j) {
				stat.Costs[j] -= um->Modifier.Costs[j];
				stat.Storing[j] -= um->Modifier.Storing[j];
				stat.ImproveIncomes[j] -= um->Modifier.ImproveIncomes[j];
				//if this was the highest improve income, search for another
				if (player.Incomes[j] && (stat.ImproveIncomes[j] + um->Modifier.ImproveIncomes[j]) == player.Incomes[j]) {
					int m = DefaultIncomes[j];

					for (int k = 0; k < player.GetUnitCount(); ++k) {
						//Wyrmgus start
//						m = std::max(m, player.GetUnit(k).Type->Stats[player.Index].ImproveIncomes[j]);
						if (player.GetUnit(k).Type != NULL) {
							m = std::max(m, player.GetUnit(k).Type->Stats[player.Index].ImproveIncomes[j]);
						}
						//Wyrmgus end
					}
					player.Incomes[j] = m;
				}
				//Wyrmgus start
				stat.ResourceDemand[j] -= um->Modifier.ResourceDemand[j];
				//Wyrmgus end
			}

			//Wyrmgus start
			for (size_t j = 0; j < UnitTypes.size(); ++j) {
				if (um->Modifier.UnitStock[j]) {
					stat.UnitStock[j] -= um->Modifier.UnitStock[j];
				}
			}
			//Wyrmgus end

			int varModified = 0;
			for (unsigned int j = 0; j < UnitTypeVar.GetNumberVariable(); j++) {
				varModified |= um->Modifier.Variables[j].Value
							   | um->Modifier.Variables[j].Max
							   | um->Modifier.Variables[j].Increase
							   | um->Modifier.Variables[j].Enable
							   | um->ModifyPercent[j];
				stat.Variables[j].Enable |= um->Modifier.Variables[j].Enable;
				if (um->ModifyPercent[j]) {
					stat.Variables[j].Value = stat.Variables[j].Value * 100 / (100 + um->ModifyPercent[j]);
					stat.Variables[j].Max = stat.Variables[j].Max * 100 / (100 + um->ModifyPercent[j]);
				} else {
					stat.Variables[j].Value -= um->Modifier.Variables[j].Value;
					stat.Variables[j].Max -= um->Modifier.Variables[j].Max;
					stat.Variables[j].Increase -= um->Modifier.Variables[j].Increase;
				}

				stat.Variables[j].Max = std::max(stat.Variables[j].Max, 0);
				//Wyrmgus start
//				clamp(&stat.Variables[j].Value, 0, stat.Variables[j].Max);
				if (stat.Variables[j].Max > 0) {
					clamp(&stat.Variables[j].Value, 0, stat.Variables[j].Max);
				}
				//Wyrmgus end
			}

			//Wyrmgus start
			std::vector<CUnit *> unitupgrade;

			FindUnitsByType(*UnitTypes[z], unitupgrade, true);
			//Wyrmgus end
			
			// And now modify ingame units
			if (varModified) {
				//Wyrmgus start
				/*
				std::vector<CUnit *> unitupgrade;

				FindUnitsByType(*UnitTypes[z], unitupgrade, true);
				*/
				//Wyrmgus end
				for (size_t j = 0; j != unitupgrade.size(); ++j) {
					CUnit &unit = *unitupgrade[j];

					if (unit.Player->Index != player.Index) {
						continue;
					}
					
					//Wyrmgus start
					if (
						(AllUpgrades[um->UpgradeId]->Weapon && unit.EquippedItems[WeaponItemSlot].size() > 0)
						|| (AllUpgrades[um->UpgradeId]->Shield && unit.EquippedItems[ShieldItemSlot].size() > 0)
						|| (AllUpgrades[um->UpgradeId]->Boots && unit.EquippedItems[BootsItemSlot].size() > 0)
						|| (AllUpgrades[um->UpgradeId]->Arrows && unit.EquippedItems[ArrowsItemSlot].size() > 0)
					) { //if the unit already has an item equipped of the same equipment type as this upgrade, don't remove the modifier from it (it already doesn't have it)
						continue;
					}
					//Wyrmgus end
					
					for (unsigned int j = 0; j < UnitTypeVar.GetNumberVariable(); j++) {
						unit.Variable[j].Enable |= um->Modifier.Variables[j].Enable;
						if (um->ModifyPercent[j]) {
							unit.Variable[j].Value = unit.Variable[j].Value * 100 / (100 + um->ModifyPercent[j]);
							unit.Variable[j].Max = unit.Variable[j].Max * 100 / (100 + um->ModifyPercent[j]);
						} else {
							unit.Variable[j].Value -= um->Modifier.Variables[j].Value;
							unit.Variable[j].Increase -= um->Modifier.Variables[j].Increase;
						}

						unit.Variable[j].Max -= um->Modifier.Variables[j].Max;
						unit.Variable[j].Max = std::max(unit.Variable[j].Max, 0);

						//Wyrmgus start
//						clamp(&unit.Variable[j].Value, 0, unit.Variable[j].Max);
						if (unit.Variable[j].Max > 0) {
							clamp(&unit.Variable[j].Value, 0, unit.GetModifiedVariable(j, VariableMax));
						}
						//Wyrmgus end
						//Wyrmgus start
						if (j == ATTACKRANGE_INDEX && unit.Container) {
							unit.Container->UpdateContainerAttackRange();
						} else if (j == LEVEL_INDEX || j == POINTS_INDEX) {
							unit.UpdateXPRequired();
						} else if (IsKnowledgeVariable(j)) {
							unit.CheckKnowledgeChange(j, - um->Modifier.Variables[j].Value);
						} else if (j == SIGHTRANGE_INDEX && !unit.Removed) {
							// If Sight range is upgraded, we need to change EVERY unit
							// to the new range, otherwise the counters get confused.
							MapUnmarkUnitSight(unit);
							UpdateUnitSightRange(unit);
							MapMarkUnitSight(unit);
						}
						//Wyrmgus end
					}
					
					//Wyrmgus start
					for (size_t j = 0; j < UnitTypes.size(); ++j) {
						if (um->Modifier.UnitStock[j] > 0) {
							if (unit.UnitStock.find(j) == unit.UnitStock.end()) {
								unit.UnitStock[j] = 0;
							}
							unit.UnitStock[j] -= um->Modifier.UnitStock[j];
							unit.UnitStock[j] = std::max(unit.UnitStock[j], 0);
						}
					}
					//Wyrmgus end
				}
			}
			
			//Wyrmgus start
			for (size_t j = 0; j != unitupgrade.size(); ++j) {
				CUnit &unit = *unitupgrade[j];

				if (unit.Player->Index != player.Index) {
					continue;
				}
				
				//add or remove starting abilities from the unit if the upgrade enabled/disabled them
				for (size_t i = 0; i < unit.Type->StartingAbilities.size(); ++i) {
					if (!unit.IndividualUpgrades[unit.Type->StartingAbilities[i]->ID] && CheckDependByIdent(unit, unit.Type->StartingAbilities[i]->Ident)) {
						IndividualUpgradeAcquire(unit, unit.Type->StartingAbilities[i]);
					} else if (unit.IndividualUpgrades[unit.Type->StartingAbilities[i]->ID] && !CheckDependByIdent(unit, unit.Type->StartingAbilities[i]->Ident)) {
						IndividualUpgradeLost(unit, unit.Type->StartingAbilities[i]);
					}
				}
				
				//change variation if current one becomes forbidden
				VariationInfo *current_varinfo = UnitTypes[z]->VarInfo[unit.Variation];
				if (current_varinfo) {
					bool required_upgrade = false;
					for (int u = 0; u < VariationMax; ++u) {
						if (!current_varinfo->UpgradesRequired[u].empty() && um->UpgradeId == CUpgrade::Get(current_varinfo->UpgradesRequired[u])->ID) {
							required_upgrade = true;
							break;
						}
					}
					if (required_upgrade == true) {
						unit.ChooseVariation();
					}
				}
				for (int i = 0; i < MaxImageLayers; ++i) {
					if (unit.LayerVariation[i] != -1 && unit.LayerVariation[i] < ((int) unit.Type->LayerVarInfo[i].size())) {
						VariationInfo *current_layer_varinfo = UnitTypes[z]->LayerVarInfo[i][unit.LayerVariation[i]];
						bool required_upgrade = false;
						for (int u = 0; u < VariationMax; ++u) {
							if (!current_layer_varinfo->UpgradesRequired[u].empty() && um->UpgradeId == CUpgrade::Get(current_layer_varinfo->UpgradesRequired[u])->ID) {
								required_upgrade = true;
								break;
							}
						}
						if (required_upgrade == true) {
							unit.ChooseVariation(NULL, false, i);
						}
					}
				}
				unit.UpdateButtonIcons();
			}
			//Wyrmgus end
			
			if (um->ConvertTo) {
				ConvertUnitTypeTo(player, *um->ConvertTo, *UnitTypes[z]);
			}
		}
	}
}

/**
**  Apply the modifiers of an individual upgrade.
**
**  @param unit    Unit that will get the modifier applied
**  @param um      Upgrade modifier that does the effects
*/
void ApplyIndividualUpgradeModifier(CUnit &unit, const CUpgradeModifier *um)
{
	Assert(um);

	for (size_t i = 0; i < um->RemoveUpgrades.size(); ++i) {
		if (unit.IndividualUpgrades[um->RemoveUpgrades[i]->ID]) {
			IndividualUpgradeLost(unit, um->RemoveUpgrades[i]);
		}
	}

	if (um->Modifier.Variables[SUPPLY_INDEX].Value) {
		if (unit.IsAlive()) {
			unit.Player->Supply += um->Modifier.Variables[SUPPLY_INDEX].Value;
		}
	}

	for (unsigned int j = 0; j < UnitTypeVar.GetNumberVariable(); j++) {
		unit.Variable[j].Enable |= um->Modifier.Variables[j].Enable;
		if (um->ModifyPercent[j]) {
			unit.Variable[j].Value += unit.Variable[j].Value * um->ModifyPercent[j] / 100;
			unit.Variable[j].Max += unit.Variable[j].Max * um->ModifyPercent[j] / 100;
		} else {
			unit.Variable[j].Value += um->Modifier.Variables[j].Value;
			unit.Variable[j].Increase += um->Modifier.Variables[j].Increase;
		}
		unit.Variable[j].Max += um->Modifier.Variables[j].Max;
		unit.Variable[j].Max = std::max(unit.Variable[j].Max, 0);
		if (unit.Variable[j].Max > 0) {
			//Wyrmgus start
//			clamp(&unit.Variable[j].Value, 0, unit.Variable[j].Max);
			clamp(&unit.Variable[j].Value, 0, unit.GetModifiedVariable(j, VariableMax));
			//Wyrmgus end
		}
		//Wyrmgus start
		if (j == ATTACKRANGE_INDEX && unit.Container && !SaveGameLoading) {
			unit.Container->UpdateContainerAttackRange();
		} else if (j == LEVEL_INDEX || j == POINTS_INDEX) {
			unit.UpdateXPRequired();
		} else if (IsKnowledgeVariable(j)) {
			unit.CheckKnowledgeChange(j, um->Modifier.Variables[j].Value);
		} else if (j == SIGHTRANGE_INDEX) {
			if (!unit.Removed && !SaveGameLoading) {
				MapUnmarkUnitSight(unit);
				UpdateUnitSightRange(unit);
				MapMarkUnitSight(unit);
			}
		}
		//Wyrmgus end
	}
	
	//Wyrmgus start
	for (size_t j = 0; j < UnitTypes.size(); ++j) {
		if (um->Modifier.UnitStock[j] < 0) {
			if (unit.UnitStock.find(j) == unit.UnitStock.end()) {
				unit.UnitStock[j] = 0;
			}
			unit.UnitStock[j] += um->Modifier.UnitStock[j];
			unit.UnitStock[j] = std::max(unit.UnitStock[j], 0);
		}
	}
	//Wyrmgus end					
	
	//Wyrmgus start
	//change variation if current one becomes forbidden
	VariationInfo *current_varinfo = unit.Type->VarInfo[unit.Variation];
	if (current_varinfo) {
		bool forbidden_upgrade = false;
		for (int u = 0; u < VariationMax; ++u) {
			if (!current_varinfo->UpgradesForbidden[u].empty() && um->UpgradeId == CUpgrade::Get(current_varinfo->UpgradesForbidden[u])->ID) {
				forbidden_upgrade = true;
				break;
			}
		}
		if (forbidden_upgrade == true) {
			unit.ChooseVariation();
		}
	}
	for (int i = 0; i < MaxImageLayers; ++i) {
		if (unit.LayerVariation[i] != -1 && unit.LayerVariation[i] < ((int) unit.Type->LayerVarInfo[i].size())) {
			VariationInfo *current_layer_varinfo = unit.Type->LayerVarInfo[i][unit.LayerVariation[i]];
			bool forbidden_upgrade = false;
			for (int u = 0; u < VariationMax; ++u) {
				if (!current_layer_varinfo->UpgradesForbidden[u].empty() && um->UpgradeId == CUpgrade::Get(current_layer_varinfo->UpgradesForbidden[u])->ID) {
					forbidden_upgrade = true;
					break;
				}
			}
			if (forbidden_upgrade == true) {
				unit.ChooseVariation(NULL, false, i);
			}
		}
	}
	unit.UpdateButtonIcons();
	//Wyrmgus end
	
	if (um->ConvertTo) {
		//Wyrmgus start
		//CommandTransformIntoType(unit, *um->ConvertTo);
		if (!unit.Character) { //don't do this for persistent characters
			CommandTransformIntoType(unit, *um->ConvertTo);
		}
		//Wyrmgus end
	}
}

//Wyrmgus start
//static void RemoveIndividualUpgradeModifier(CUnit &unit, const CUpgradeModifier *um)
void RemoveIndividualUpgradeModifier(CUnit &unit, const CUpgradeModifier *um)
//Wyrmgus end
{
	Assert(um);

	if (um->Modifier.Variables[SUPPLY_INDEX].Value) {
		if (unit.IsAlive()) {
			unit.Player->Supply -= um->Modifier.Variables[SUPPLY_INDEX].Value;
		}
	}
	
	for (unsigned int j = 0; j < UnitTypeVar.GetNumberVariable(); j++) {
		unit.Variable[j].Enable |= um->Modifier.Variables[j].Enable;
		if (um->ModifyPercent[j]) {
			unit.Variable[j].Value = unit.Variable[j].Value * 100 / (100 + um->ModifyPercent[j]);
			unit.Variable[j].Max = unit.Variable[j].Max * 100 / (100 + um->ModifyPercent[j]);
		} else {
			unit.Variable[j].Value -= um->Modifier.Variables[j].Value;
			unit.Variable[j].Increase -= um->Modifier.Variables[j].Increase;
		}
		unit.Variable[j].Max -= um->Modifier.Variables[j].Max;
		unit.Variable[j].Max = std::max(unit.Variable[j].Max, 0);
		if (unit.Variable[j].Max > 0) {
			//Wyrmgus start
//			clamp(&unit.Variable[j].Value, 0, unit.Variable[j].Max);
			clamp(&unit.Variable[j].Value, 0, unit.GetModifiedVariable(j, VariableMax));
			//Wyrmgus end
		}
		//Wyrmgus start
		if (j == ATTACKRANGE_INDEX && unit.Container) {
			unit.Container->UpdateContainerAttackRange();
		} else if (j == LEVEL_INDEX || j == POINTS_INDEX) {
			unit.UpdateXPRequired();
		} else if (IsKnowledgeVariable(j)) {
			unit.CheckKnowledgeChange(j, - um->Modifier.Variables[j].Value);
		} else if (j == SIGHTRANGE_INDEX) {
			if (!unit.Removed && !SaveGameLoading) {
				MapUnmarkUnitSight(unit);
				UpdateUnitSightRange(unit);
				MapMarkUnitSight(unit);
			}
		}
		//Wyrmgus end
	}
	
	//Wyrmgus start
	for (size_t j = 0; j < UnitTypes.size(); ++j) {
		if (um->Modifier.UnitStock[j] > 0) {
			if (unit.UnitStock.find(j) == unit.UnitStock.end()) {
				unit.UnitStock[j] = 0;
			}
			unit.UnitStock[j] -= um->Modifier.UnitStock[j];
			unit.UnitStock[j] = std::max(unit.UnitStock[j], 0);
		}
	}
	//Wyrmgus end					
	
	//Wyrmgus start
	//change variation if current one becomes forbidden
	VariationInfo *current_varinfo = unit.Type->VarInfo[unit.Variation];
	if (current_varinfo) {
		bool required_upgrade = false;
		for (int u = 0; u < VariationMax; ++u) {
			if (!current_varinfo->UpgradesRequired[u].empty() && um->UpgradeId == CUpgrade::Get(current_varinfo->UpgradesRequired[u])->ID) {
				required_upgrade = true;
				break;
			}
		}
		if (required_upgrade == true) {
			unit.ChooseVariation();
		}
	}
	for (int i = 0; i < MaxImageLayers; ++i) {
		if (unit.LayerVariation[i] != -1 && unit.LayerVariation[i] < ((int) unit.Type->LayerVarInfo[i].size())) {
			VariationInfo *current_layer_varinfo = unit.Type->LayerVarInfo[i][unit.LayerVariation[i]];
			bool required_upgrade = false;
			for (int u = 0; u < VariationMax; ++u) {
				if (!current_layer_varinfo->UpgradesRequired[u].empty() && um->UpgradeId == CUpgrade::Get(current_layer_varinfo->UpgradesRequired[u])->ID) {
					required_upgrade = true;
					break;
				}
			}
			if (required_upgrade == true) {
				unit.ChooseVariation(NULL, false, i);
			}
		}
	}
	unit.UpdateButtonIcons();
	//Wyrmgus end
}

/**
**  Handle that an upgrade was acquired.
**
**  @param player   Player researching the upgrade.
**  @param upgrade  Upgrade ready researched.
*/
void UpgradeAcquire(CPlayer &player, const CUpgrade *upgrade)
{
	//Wyrmgus start
	if (!GameRunning && !GameEstablishing) {
		return;
	}
	//Wyrmgus end
	int id = upgrade->ID;
	player.UpgradeTimers.Upgrades[id] = upgrade->Costs[TimeCost];
	AllowUpgradeId(player, id, 'R');  // research done

	//Wyrmgus start
	if (!strncmp(upgrade->Ident.c_str(), "upgrade-deity-", 14) && strncmp(upgrade->Ident.c_str(), "upgrade-deity-domain-", 21)) { // if is a deity upgrade, but isn't a deity domain upgrade
		CDeity *upgrade_deity = PlayerRaces.GetDeity(FindAndReplaceString(upgrade->Ident, "upgrade-deity-", ""));
		if (upgrade_deity) {
			for (size_t i = 0; i < upgrade_deity->Domains.size(); ++i) {
				CUpgrade *domain_upgrade = CUpgrade::Get("upgrade-deity-domain-" + upgrade_deity->Domains[i]->Ident);
				if (player.Allow.Upgrades[domain_upgrade->ID] != 'R') {
					UpgradeAcquire(player, domain_upgrade);
				}
			}
			player.Deities.push_back(upgrade_deity);
		} else {
			fprintf(stderr, "Deity \"%s\" has an upgrade, but doesn't exist.\n", upgrade->Ident.c_str());
		}
	}
	
	/*
	for (int z = 0; z < NumUpgradeModifiers; ++z) {
		if (UpgradeModifiers[z]->UpgradeId == id) {
			ApplyUpgradeModifier(player, UpgradeModifiers[z]);
		}
	}
	*/
	for (size_t z = 0; z < upgrade->UpgradeModifiers.size(); ++z) {
		ApplyUpgradeModifier(player, upgrade->UpgradeModifiers[z]);
	}
	//Wyrmgus end

	//
	//  Upgrades could change the buttons displayed.
	//
	if (&player == ThisPlayer) {
		SelectedUnitChanged();
	}
}

/**
**  Upgrade will be lost
**
**  @param player   Player researching the upgrade.
**  @param id       Upgrade to be lost.
**  
*/
void UpgradeLost(CPlayer &player, int id)
{
	//Wyrmgus start
	if (!GameRunning && !GameEstablishing) {
		return;
	}
	//Wyrmgus end
	player.UpgradeTimers.Upgrades[id] = 0;
	//Wyrmgus start
	AllowUpgradeId(player, id, 'A'); // research is lost i.e. available
	//Wyrmgus end
	
	//Wyrmgus start
	CUpgrade *upgrade = AllUpgrades[id];
	if (!strncmp(upgrade->Ident.c_str(), "upgrade-deity-", 14) && strncmp(upgrade->Ident.c_str(), "upgrade-deity-domain-", 21)) { // if is a deity upgrade, but isn't a deity domain upgrade
		CDeity *upgrade_deity = PlayerRaces.GetDeity(FindAndReplaceString(upgrade->Ident, "upgrade-deity-", ""));
		if (upgrade_deity) {
			for (size_t i = 0; i < upgrade_deity->Domains.size(); ++i) {
				CUpgrade *domain_upgrade = CUpgrade::Get("upgrade-deity-domain-" + upgrade_deity->Domains[i]->Ident);
				if (player.Allow.Upgrades[domain_upgrade->ID] == 'R') {
					UpgradeLost(player, domain_upgrade->ID);
				}
			}
			player.Deities.erase(std::remove(player.Deities.begin(), player.Deities.end(), upgrade_deity), player.Deities.end());
		} else {
			fprintf(stderr, "Deity \"%s\" has an upgrade, but doesn't exist.\n", upgrade->Ident.c_str());
		}
	}

	/*
	for (int z = 0; z < NumUpgradeModifiers; ++z) {
		if (UpgradeModifiers[z]->UpgradeId == id) {
			RemoveUpgradeModifier(player, UpgradeModifiers[z]);
		}
	}
	*/
	for (size_t z = 0; z < AllUpgrades[id]->UpgradeModifiers.size(); ++z) {
		RemoveUpgradeModifier(player, AllUpgrades[id]->UpgradeModifiers[z]);
	}
	//Wyrmgus end

	//
	//  Upgrades could change the buttons displayed.
	//
	if (&player == ThisPlayer) {
		SelectedUnitChanged();
	}
}

/**
**  Apply researched upgrades when map is loading
**
**  @return:   void
*/
void ApplyUpgrades()
{
	for (std::vector<CUpgrade *>::size_type j = 0; j < AllUpgrades.size(); ++j) {
		CUpgrade *upgrade = AllUpgrades[j];
		if (upgrade) {
			for (int p = 0; p < PlayerMax; ++p) {
				if (Players[p].Allow.Upgrades[j] == 'R') {
					int id = upgrade->ID;
					Players[p].UpgradeTimers.Upgrades[id] = upgrade->Costs[TimeCost];
					AllowUpgradeId(Players[p], id, 'R');  // research done

					//Wyrmgus start
					/*
					for (int z = 0; z < NumUpgradeModifiers; ++z) {
						if (UpgradeModifiers[z]->UpgradeId == id) {
							ApplyUpgradeModifier(Players[p], UpgradeModifiers[z]);
						}
					}
					*/
					for (size_t z = 0; z < upgrade->UpgradeModifiers.size(); ++z) {
						ApplyUpgradeModifier(Players[p], upgrade->UpgradeModifiers[z]);
					}
					//Wyrmgus end
				}
			}
		}
	}
}

//Wyrmgus start
/**
**  Handle that an ability was acquired.
**
**  @param unit     Unit learning the upgrade.
**  @param upgrade  Upgrade learned.
*/
void AbilityAcquire(CUnit &unit, CUpgrade *upgrade)
{
	unit.Variable[LEVELUP_INDEX].Value -= 1;
	unit.Variable[LEVELUP_INDEX].Max = unit.Variable[LEVELUP_INDEX].Value;
	if (!IsNetworkGame() && unit.Character != NULL) {
		if (std::find(unit.Character->Abilities.begin(), unit.Character->Abilities.end(), upgrade) == unit.Character->Abilities.end()) {
			if (unit.Player->AiEnabled == false) { //save ability learning, if unit has a character and it is persistent, and the character doesn't have the ability yet
				unit.Character->Abilities.push_back(upgrade);
				SaveHero(unit.Character);
			}
		}
	}
	IndividualUpgradeAcquire(unit, upgrade);
	unit.Player->UpdateLevelUpUnits();
}

/**
**  Handle that an ability was lost.
**
**  @param unit     Unit losing the upgrade.
**  @param upgrade  Upgrade lost.
*/
void AbilityLost(CUnit &unit, CUpgrade *upgrade, bool lose_all)
{
	unit.Variable[LEVELUP_INDEX].Value += 1;
	unit.Variable[LEVELUP_INDEX].Max = unit.Variable[LEVELUP_INDEX].Value;
	if (!IsNetworkGame() && unit.Character != NULL) {
		if (std::find(unit.Character->Abilities.begin(), unit.Character->Abilities.end(), upgrade) != unit.Character->Abilities.end()) {
			if (unit.Player->AiEnabled == false) { //save ability learning, if unit has a character and it is persistent, and the character doesn't have the ability yet
				unit.Character->Abilities.erase(std::remove(unit.Character->Abilities.begin(), unit.Character->Abilities.end(), upgrade), unit.Character->Abilities.end());
				SaveHero(unit.Character);
			}
		}
	}
	IndividualUpgradeLost(unit, upgrade);
	unit.Player->UpdateLevelUpUnits();
	
	if (lose_all && unit.IndividualUpgrades[upgrade->ID] > 0) {
		AbilityLost(unit, upgrade, lose_all);
	}
}

void TraitAcquire(CUnit &unit, const CUpgrade *upgrade)
{
	if (unit.Trait != NULL) { //remove previous trait, if any
		if (!GameSettings.NoRandomness) { // if in no randomness setting, don't change trait modifiers
			IndividualUpgradeLost(unit, unit.Trait);
		}
	}

	unit.Trait = CUpgrade::Get(upgrade->Ident);

	if (!GameSettings.NoRandomness) { // if in no randomness setting, don't apply trait modifiers
		IndividualUpgradeAcquire(unit, upgrade);
	}
	
	unit.UpdatePersonalName();

	//
	//  Upgrades could change the buttons displayed.
	//
	if (unit.Player == ThisPlayer) {
		SelectedUnitChanged();
	}
}
//Wyrmgus end

void IndividualUpgradeAcquire(CUnit &unit, const CUpgrade *upgrade)
{
	//Wyrmgus start
	if (!GameRunning && !GameEstablishing && !SaveGameLoading) {
		return;
	}
	//Wyrmgus end
	int id = upgrade->ID;
	unit.IndividualUpgrades[id] += 1;
	
	if (!strncmp(upgrade->Ident.c_str(), "upgrade-deity-", 14) && strncmp(upgrade->Ident.c_str(), "upgrade-deity-domain-", 21)) { // if is a deity upgrade, but isn't a deity domain upgrade
		CDeity *upgrade_deity = PlayerRaces.GetDeity(FindAndReplaceString(upgrade->Ident, "upgrade-deity-", ""));
		if (upgrade_deity) {
			for (size_t i = 0; i < upgrade_deity->Domains.size(); ++i) {
				CUpgrade *domain_upgrade = CUpgrade::Get("upgrade-deity-domain-" + upgrade_deity->Domains[i]->Ident);
				if (!unit.IndividualUpgrades[domain_upgrade->ID]) {
					IndividualUpgradeAcquire(unit, domain_upgrade);
				}
			}
			if (unit.Character && std::find(unit.Character->Deities.begin(), unit.Character->Deities.end(), upgrade_deity) == unit.Character->Deities.end()) {
				unit.Character->Deities.push_back(upgrade_deity);
			}
		} else {
			fprintf(stderr, "Deity \"%s\" has an upgrade, but doesn't exist.\n", upgrade->Ident.c_str());
		}
	}

	//Wyrmgus start
	/*
	for (int z = 0; z < NumUpgradeModifiers; ++z) {
		if (UpgradeModifiers[z]->UpgradeId == id) {
			ApplyIndividualUpgradeModifier(unit, UpgradeModifiers[z]);
		}
	}
	*/
	if (!(upgrade->Ability && upgrade->WeaponClasses.size() > 0 && std::find(upgrade->WeaponClasses.begin(), upgrade->WeaponClasses.end(), unit.GetCurrentWeaponClass()) == upgrade->WeaponClasses.end())) {
		for (size_t z = 0; z < upgrade->UpgradeModifiers.size(); ++z) {
			bool applies_to_this = false;
			bool applies_to_any_unit_types = false;
			for (size_t i = 0; i < UnitTypes.size(); ++i) {
				if (upgrade->UpgradeModifiers[z]->ApplyTo[i] == 'X') {
					applies_to_any_unit_types = true;
					if (i == unit.Type->Slot) {
						applies_to_this = true;
						break;
					}
				}
			}
			if (applies_to_this || !applies_to_any_unit_types) { //if the modifier isn't designated as being for a specific unit type, or is designated for this unit's unit type, apply it
				ApplyIndividualUpgradeModifier(unit, upgrade->UpgradeModifiers[z]);
			}
		}
	}
	//Wyrmgus end

	//
	//  Upgrades could change the buttons displayed.
	//
	if (unit.Player == ThisPlayer) {
		SelectedUnitChanged();
	}
}

void IndividualUpgradeLost(CUnit &unit, const CUpgrade *upgrade, bool lose_all)
{
	//Wyrmgus start
	if (!GameRunning && !GameEstablishing && !SaveGameLoading) {
		return;
	}
	//Wyrmgus end
	int id = upgrade->ID;
	unit.IndividualUpgrades[id] -= 1;

	if (!strncmp(upgrade->Ident.c_str(), "upgrade-deity-", 14) && strncmp(upgrade->Ident.c_str(), "upgrade-deity-domain-", 21)) { // if is a deity upgrade, but isn't a deity domain upgrade
		CDeity *upgrade_deity = PlayerRaces.GetDeity(FindAndReplaceString(upgrade->Ident, "upgrade-deity-", ""));
		if (upgrade_deity) {
			for (size_t i = 0; i < upgrade_deity->Domains.size(); ++i) {
				CUpgrade *domain_upgrade = CUpgrade::Get("upgrade-deity-domain-" + upgrade_deity->Domains[i]->Ident);
				if (unit.IndividualUpgrades[domain_upgrade->ID]) {
					IndividualUpgradeLost(unit, domain_upgrade);
				}
			}
			if (unit.Character) {
				unit.Character->Deities.erase(std::remove(unit.Character->Deities.begin(), unit.Character->Deities.end(), upgrade_deity), unit.Character->Deities.end());
			}
		} else {
			fprintf(stderr, "Deity \"%s\" has an upgrade, but doesn't exist.\n", upgrade->Ident.c_str());
		}
	}

	//Wyrmgus start
	if (!(upgrade->Ability && upgrade->WeaponClasses.size() > 0 && std::find(upgrade->WeaponClasses.begin(), upgrade->WeaponClasses.end(), unit.GetCurrentWeaponClass()) == upgrade->WeaponClasses.end())) {
		for (size_t z = 0; z < upgrade->UpgradeModifiers.size(); ++z) {
			bool applies_to_this = false;
			bool applies_to_any_unit_types = false;
			for (size_t i = 0; i < UnitTypes.size(); ++i) {
				if (upgrade->UpgradeModifiers[z]->ApplyTo[i] == 'X') {
					applies_to_any_unit_types = true;
					if (i == unit.Type->Slot) {
						applies_to_this = true;
						break;
					}
				}
			}
			if (applies_to_this || !applies_to_any_unit_types) { //if the modifier isn't designated as being for a specific unit type, or is designated for this unit's unit type, remove it
				RemoveIndividualUpgradeModifier(unit, upgrade->UpgradeModifiers[z]);
			}
		}
	}
	//Wyrmgus end

	//
	//  Upgrades could change the buttons displayed.
	//
	if (unit.Player == ThisPlayer) {
		SelectedUnitChanged();
	}
	
	if (lose_all && unit.IndividualUpgrades[id] > 0) {
		IndividualUpgradeLost(unit, upgrade, lose_all);
	}
}

/*----------------------------------------------------------------------------
--  Allow(s)
----------------------------------------------------------------------------*/

// all the following functions are just map handlers, no specific notes

/**
**  Change allow for an unit-type.
**
**  @param player  Player to change
**  @param id      unit type id
**  @param units   maximum amount of units allowed
*/
//Wyrmgus start
//static void AllowUnitId(CPlayer &player, int id, int units)
void AllowUnitId(CPlayer &player, int id, int units)
//Wyrmgus end
{
	player.Allow.Units[id] = units;
}

/**
**  Change allow for an upgrade.
**
**  @param player  Player to change
**  @param id      upgrade id
**  @param af      'A'llow/'F'orbid/'R'esearched
*/
void AllowUpgradeId(CPlayer &player, int id, char af)
{
	Assert(af == 'A' || af == 'F' || af == 'R');
	player.Allow.Upgrades[id] = af;
}

/**
**  Return the allow state of the unit.
**
**  @param player   Check state of this player.
**  @param id       Unit identifier.
**
**  @return the allow state of the unit.
*/
int UnitIdAllowed(const CPlayer &player, int id)
{
	Assert(id >= 0 && id < UnitTypeMax);
	return player.Allow.Units[id];
}

/**
**  Return the allow state of an upgrade.
**
**  @param player  Check state for this player.
**  @param id      Upgrade identifier.
**
**  @return the allow state of the upgrade.
*/
char UpgradeIdAllowed(const CPlayer &player, int id)
{
	Assert(id >= 0 && id < UpgradeMax);
	return player.Allow.Upgrades[id];
}

// ***************by string identifiers's

/**
**  Return the allow state of an upgrade.
**
**  @param player  Check state for this player.
**  @param ident   Upgrade identifier.
**
**  @note This function shouldn't be used during runtime, it is only for setup.
*/
char UpgradeIdentAllowed(const CPlayer &player, const std::string &ident)
{
	int id = UpgradeIdByIdent(ident);

	if (id != -1) {
		return UpgradeIdAllowed(player, id);
	}
	DebugPrint("Fix your code, wrong identifier '%s'\n" _C_ ident.c_str());
	return '-';
}

//Wyrmgus start
std::string GetUpgradeEffectsString(std::string upgrade_ident, bool grand_strategy, bool multiline)
{
	const CUpgrade *upgrade = CUpgrade::Get(upgrade_ident);
	
	std::string padding_string = ", ";
	if (multiline) {
		padding_string = "\n";
	}

	if (upgrade) {
		std::string upgrade_effects_string;
		
		bool first_element = true;
		//check if the upgrade makes modifications to any units
		for (size_t z = 0; z < upgrade->UpgradeModifiers.size(); ++z) {
			if (grand_strategy) { // don't show modifiers in the grand strategy mode for now
				continue;
			}

			if (!first_element) {
				upgrade_effects_string += padding_string;
			} else {
				first_element = false;
			}
				
			bool first_var = true;
			for (size_t var = 0; var < UnitTypeVar.GetNumberVariable(); ++var) {
				if (var == PRIORITY_INDEX || var == POINTS_INDEX) {
					continue;
				}
						
				if (var == STRENGTH_INDEX || var == DEXTERITY_INDEX || var == INTELLIGENCE_INDEX || var == CHARISMA_INDEX) { // don't show attributes for now
					continue;
				}
					
				if (grand_strategy) {
					if (
						var == SUPPLY_INDEX // don't show supply effects in the grand strategy mode
					) {
						continue;
					}
				}

				if (upgrade->UpgradeModifiers[z]->Modifier.Variables[var].Value != 0) {
					if (!first_var) {
						upgrade_effects_string += padding_string;
					} else {
						first_var = false;
					}

					if (IsBooleanVariable(var) && upgrade->UpgradeModifiers[z]->Modifier.Variables[var].Value < 0) {
						upgrade_effects_string += "Lose ";
					}
										
					if (!IsBooleanVariable(var)) {
						if (upgrade->UpgradeModifiers[z]->Modifier.Variables[var].Value > 0) {
							upgrade_effects_string += "+";
						}
						upgrade_effects_string += std::to_string((long long) upgrade->UpgradeModifiers[z]->Modifier.Variables[var].Value);
						if (IsPercentageVariable(var)) {
							upgrade_effects_string += "%";
						}
						upgrade_effects_string += " ";
					}

					upgrade_effects_string += GetVariableDisplayName(var);
						
					bool first_unit_type = true;
					for (size_t i = 0; i < UnitTypes.size(); ++i) {
						if (upgrade->UpgradeModifiers[z]->ApplyTo[i] == 'X') {
							if (!first_unit_type) {
								upgrade_effects_string += ", ";
							} else {
								upgrade_effects_string += " for ";
								first_unit_type = false;
							}
									
							upgrade_effects_string += UnitTypes[i]->GetNamePlural();
						}
					}
				}
					
				if (upgrade->UpgradeModifiers[z]->Modifier.Variables[var].Increase != 0) {
					if (!first_var) {
						upgrade_effects_string += padding_string;
					} else {
						first_var = false;
					}

					if (upgrade->UpgradeModifiers[z]->Modifier.Variables[var].Increase > 0) {
						upgrade_effects_string += "+";
					}
					upgrade_effects_string += std::to_string((long long) upgrade->UpgradeModifiers[z]->Modifier.Variables[var].Increase);
					upgrade_effects_string += " ";
											
					upgrade_effects_string += GetVariableDisplayName(var, true);
				}
			}
				
			if (!grand_strategy) {
				bool first_res = true;
				for (int i = 0; i < MaxCosts; ++i) {
					if (upgrade->UpgradeModifiers[z]->Modifier.ImproveIncomes[i]) {
						if (!first_res) {
							upgrade_effects_string += padding_string;
						} else {
							first_res = false;
						}
							
						if (upgrade->UpgradeModifiers[z]->Modifier.ImproveIncomes[i] > 0) {
							upgrade_effects_string += "+";
						}
						upgrade_effects_string += std::to_string((long long) upgrade->UpgradeModifiers[z]->Modifier.ImproveIncomes[i]);
						upgrade_effects_string += "%";
						upgrade_effects_string += " ";
						upgrade_effects_string += CapitalizeString(DefaultResourceNames[i]);
						upgrade_effects_string += " Processing";
							
						bool first_unit_type = true;
						for (size_t i = 0; i < UnitTypes.size(); ++i) {
							if (upgrade->UpgradeModifiers[z]->ApplyTo[i] == 'X') {
								if (!first_unit_type) {
									upgrade_effects_string += ", ";
								} else {
									upgrade_effects_string += " for ";
									first_unit_type = false;
								}
										
								upgrade_effects_string += UnitTypes[i]->GetNamePlural();
							}
						}
					}
				}
			}
		}
		
		return upgrade_effects_string;
	}
	
	return "";
}

bool IsPercentageVariable(int var)
{
	return var == BACKSTAB_INDEX || var == BONUSAGAINSTMOUNTED_INDEX || var == BONUSAGAINSTBUILDINGS_INDEX || var == BONUSAGAINSTAIR_INDEX || var == BONUSAGAINSTGIANTS_INDEX || var == BONUSAGAINSTDRAGONS_INDEX || var == FIRERESISTANCE_INDEX || var == COLDRESISTANCE_INDEX || var == ARCANERESISTANCE_INDEX || var == LIGHTNINGRESISTANCE_INDEX || var == AIRRESISTANCE_INDEX || var == EARTHRESISTANCE_INDEX || var == WATERRESISTANCE_INDEX || var == HACKRESISTANCE_INDEX || var == PIERCERESISTANCE_INDEX || var == BLUNTRESISTANCE_INDEX || var == TIMEEFFICIENCYBONUS_INDEX || var == TRADECOST_INDEX || var == SALVAGEFACTOR_INDEX;
}

bool IsBonusVariable(int var)
{
	return var == GATHERINGBONUS_INDEX || var == COPPERGATHERINGBONUS_INDEX || var == SILVERGATHERINGBONUS_INDEX || var == GOLDGATHERINGBONUS_INDEX || var == LUMBERGATHERINGBONUS_INDEX || var == STONEGATHERINGBONUS_INDEX || var == COALGATHERINGBONUS_INDEX || var == FURNITUREGATHERINGBONUS_INDEX || var == LEATHERGATHERINGBONUS_INDEX || var == GEMSGATHERINGBONUS_INDEX || var == SPEEDBONUS_INDEX || var == BACKSTAB_INDEX;
}

bool IsBooleanVariable(int var)
{
	return var == DISEMBARKMENTBONUS_INDEX || var == DESERTSTALK_INDEX || var == FORESTSTALK_INDEX || var == SWAMPSTALK_INDEX || var == DEHYDRATIONIMMUNITY_INDEX || var == LEADERSHIPAURA_INDEX || var == REGENERATIONAURA_INDEX || var == HYDRATINGAURA_INDEX || var == ETHEREALVISION_INDEX;
}

bool IsKnowledgeVariable(int var)
{
	return var == KNOWLEDGEMAGIC_INDEX || var == KNOWLEDGEWARFARE_INDEX || var == KNOWLEDGEMINING_INDEX;
}

bool IsPotentiallyNegativeVariable(int var)
{
	return var == DAYSIGHTRANGEBONUS_INDEX || var == NIGHTSIGHTRANGEBONUS_INDEX;
}

std::string GetVariableDisplayName(int var, bool increase)
{
	std::string variable_name = UnitTypeVar.VariableNameLookup[var];

	if (increase) {
		variable_name += "Increase";
		variable_name = FindAndReplaceString(variable_name, "HitPointsIncrease", "Regeneration");
		variable_name = FindAndReplaceString(variable_name, "HitPointBonusIncrease", "Regeneration");
		variable_name = FindAndReplaceString(variable_name, "GiveResourceIncrease", "ResourceReplenishment");
	}
	
	variable_name = FindAndReplaceString(variable_name, "BasicDamage", "Damage");
	variable_name = FindAndReplaceString(variable_name, "SightRange", "Sight");
	variable_name = FindAndReplaceString(variable_name, "AttackRange", "Range");
	variable_name = FindAndReplaceString(variable_name, "HitPointBonus", "HitPoints");
	variable_name = FindAndReplaceString(variable_name, "DaySightBonus", "Day Sight");
	variable_name = FindAndReplaceString(variable_name, "NightSightBonus", "Night Sight");
	variable_name = FindAndReplaceString(variable_name, "Supply", "FoodSupply");
	variable_name = FindAndReplaceString(variable_name, "Demand", "FoodCost");
	variable_name = SeparateCapitalizedStringElements(variable_name);
	variable_name = FindAndReplaceString(variable_name, "Backstab", "Backstab Bonus");
	variable_name = FindAndReplaceString(variable_name, "Knowledge Magic", "Knowledge (Magic)");
	variable_name = FindAndReplaceString(variable_name, "Knowledge Warfare", "Knowledge (Warfare)");

	return _(variable_name.c_str());
}
//Wyrmgus end

//@}
