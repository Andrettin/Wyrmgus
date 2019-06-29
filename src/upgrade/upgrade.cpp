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
/**@name upgrade.cpp - The upgrade/allow source file. */
//
//      (c) Copyright 1999-2019 by Vladi Belperchinov-Shabanski, Jimmy Salmon
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

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "upgrade/upgrade.h"

//Wyrmgus start
#include "action/action_build.h"
//Wyrmgus end
#include "action/action_train.h"
//Wyrmgus start
#include "action/action_upgradeto.h"
#include "ai/ai_local.h"
//Wyrmgus end
#include "civilization.h"
#include "commands.h"
#include "config.h"
#include "config_operator.h"
#include "dependency/and_dependency.h"
#include "dependency/dependency.h"
#include "dynasty.h"
//Wyrmgus start
#include "editor/editor.h"
#include "faction.h"
#include "game/game.h"
//Wyrmgus end
#include "iolib.h"
//Wyrmgus start
#include "item/item.h"
//Wyrmgus end
#include "item/item_class.h"
#include "item/item_slot.h"
#include "map/map.h"
#include "map/tileset.h"
//Wyrmgus start
#include "network/network.h"
//Wyrmgus end
#include "player.h"
#include "religion/deity.h"
#include "religion/deity_domain.h"
#include "script.h"
//Wyrmgus start
#include "settings.h"
#include "translate.h"
//Wyrmgus end
#include "ui/icon.h"
#include "unit/unit.h"
#include "unit/unit_find.h"
//Wyrmgus start
#include "ui/interface.h"
#include "ui/ui.h"
#include "unit/unit_manager.h"
//Wyrmgus end
#include "unit/unit_type.h"
#include "unit/unit_type_variation.h"
#include "upgrade/upgrade_class.h"
#include "upgrade/upgrade_modifier.h"
#include "util.h"

#include <algorithm>
#include <map>
#include <string>
#include <vector>

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

//Wyrmgus start
//static void AllowUnitId(CPlayer &player, int id, int units);
//Wyrmgus end

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
	this->UnitStock = rhs.UnitStock;
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
	if (this->UnitStock != rhs.UnitStock) {
		return false;
	}
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

int CUnitStats::GetPrice() const
{
	int cost = 0;
	
	for (int i = 1; i < MaxCosts; ++i) {
		if (this->Costs[i] > 0) {
			if (i == CopperCost) {
				cost += this->Costs[i];
			} else {
				cost += this->Costs[i] * CResource::GetAll()[i]->BasePrice / 100;
			}
		}
	}
	
	return cost;
}

int CUnitStats::GetUnitStock(const CUnitType *unit_type) const
{
	if (unit_type && this->UnitStock.find(unit_type) != this->UnitStock.end()) {
		return this->UnitStock.find(unit_type)->second;
	} else {
		return 0;
	}
}

void CUnitStats::SetUnitStock(const CUnitType *unit_type, const int quantity)
{
	if (!unit_type) {
		return;
	}
	
	if (quantity <= 0) {
		if (this->UnitStock.find(unit_type) != this->UnitStock.end()) {
			this->UnitStock.erase(unit_type);
		}
	} else {
		this->UnitStock[unit_type] = quantity;
	}
}

void CUnitStats::ChangeUnitStock(const CUnitType *unit_type, const int quantity)
{
	this->SetUnitStock(unit_type, this->GetUnitStock(unit_type) + quantity);
}

CUpgrade::CUpgrade(const std::string &ident)
{
	this->Ident = ident;

	memset(this->Costs, 0, sizeof(this->Costs));
	//Wyrmgus start
	memset(this->ScaledCosts, 0, sizeof(this->ScaledCosts));
	memset(this->GrandStrategyProductionEfficiencyModifier, 0, sizeof(this->GrandStrategyProductionEfficiencyModifier));
	memset(this->IncompatibleAffixes, 0, sizeof(this->IncompatibleAffixes));
	//Wyrmgus end
}

/**
**	@brief	Process a section in the data provided by a configuration file
**
**	@param	section		The section
**
**	@return	True if the section can be processed, or false otherwise
*/
bool CUpgrade::ProcessConfigDataSection(const CConfigData *section)
{
	if (section->Tag == "costs") {
		for (const CConfigProperty &property : section->Properties) {
			if (property.Operator != CConfigOperator::Assignment) {
				print_error("Wrong operator enumeration index for property \"" + property.Key + "\": " + String::num_int64(static_cast<int>(property.Operator)) + ".");
				continue;
			}
			
			String key = property.Key;
			String value = property.Value;
			
			key = key.replace("_", "-");
			
			const int resource = GetResourceIdByName(key.utf8().get_data());
			if (resource != -1) {
				this->Costs[resource] = value.to_int();
			} else {
				fprintf(stderr, "Invalid resource: \"%s\".\n", key.utf8().get_data());
			}
		}
	} else if (section->Tag == "predependencies") {
		this->Predependency = new CAndDependency;
		this->Predependency->ProcessConfigData(section);
	} else if (section->Tag == "dependencies") {
		this->Dependency = new CAndDependency;
		this->Dependency->ProcessConfigData(section);
	} else if (section->Tag == "modifier") {
		CUpgradeModifier *modifier = new CUpgradeModifier;
		modifier->UpgradeId = this->GetIndex();
		this->UpgradeModifiers.push_back(modifier);
		
		modifier->ProcessConfigData(section);
		
		CUpgradeModifier::UpgradeModifiers.push_back(modifier);
	} else {
		return false;
	}
	
	return true;
}

/**
**	@brief	Initialize the upgrade
*/
void CUpgrade::Initialize()
{
	//set the upgrade's civilization and class here
	if (this->Class != nullptr) { //if class is defined, then use this upgrade to help build the classes table, and add this upgrade to the civilization class table (if the civilization is defined)
		UpgradeClass *upgrade_class = this->Class;
		
		if (this->Faction != nullptr) {
			this->Faction->ClassUpgrades[upgrade_class] = this;
		} else if (this->Civilization != nullptr) {
			this->Civilization->ClassUpgrades[upgrade_class] = this;
		}
	}
	
	for (CUpgrade *upgrade : CUpgrade::GetAll()) { //add the upgrade to the incompatible affix's counterpart list here
		if (this->IncompatibleAffixes[upgrade->GetIndex()]) {
			upgrade->IncompatibleAffixes[this->GetIndex()] = true;
		}
	}
	
	this->Initialized = true;
	
	CclCommand("if not (GetArrayIncludes(Units, \"" + this->Ident + "\")) then table.insert(Units, \"" + this->Ident + "\") end"); //FIXME: needed at present to make upgrade data files work without scripting being necessary, but it isn't optimal to interact with a scripting table like "Units" in this manner (that table should probably be replaced with getting a list of unit types from the engine)
}

/**
**	@brief	Set the upgrade's parent upgrade
**
**	@param	parent_upgrade	The parent upgrade
*/
void CUpgrade::SetParentUpgrade(const CUpgrade *parent_upgrade)
{
	if (!parent_upgrade->Initialized) {
		print_error("Upgrade \"" + this->GetIdent() + "\" is inheriting features from a non-initialized parent (\"" + parent_upgrade->GetIdent() + "\").");
	}
	
	this->ParentUpgrade = parent_upgrade;
	this->Name = parent_upgrade->Name;
	this->Icon = parent_upgrade->Icon;
	this->Class = parent_upgrade->Class;
	this->Description = parent_upgrade->Description;
	this->Quote = parent_upgrade->Quote;
	this->Background = parent_upgrade->Background;
	this->EffectsString = parent_upgrade->EffectsString;
	this->RequirementsString = parent_upgrade->RequirementsString;
	for (int i = 0; i < MaxCosts; ++i) {
		this->Costs[i] = parent_upgrade->Costs[i];
		this->ScaledCosts[i] = parent_upgrade->ScaledCosts[i];
		this->GrandStrategyProductionEfficiencyModifier[i] = parent_upgrade->GrandStrategyProductionEfficiencyModifier[i];
	}
	this->ItemPrefix = parent_upgrade->ItemPrefix;
	this->ItemSuffix = parent_upgrade->ItemSuffix;
	this->MaxLimit = parent_upgrade->MaxLimit;
	this->MagicLevel = parent_upgrade->MagicLevel;
	this->Ability = parent_upgrade->Ability;
	this->ItemSlot = parent_upgrade->ItemSlot;
	this->Item = parent_upgrade->Item;
	this->MagicPrefix = parent_upgrade->MagicPrefix;
	this->MagicSuffix = parent_upgrade->MagicSuffix;
	this->RunicAffix = parent_upgrade->RunicAffix;
	this->Work = parent_upgrade->Work;
	this->UniqueOnly = parent_upgrade->UniqueOnly;
	for (size_t i = 0; i < parent_upgrade->ScaledCostUnits.size(); ++i) {
		this->ScaledCostUnits.push_back(parent_upgrade->ScaledCostUnits[i]);
	}
}

void CUpgrade::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("set_parent_upgrade", "ident"), +[](CUpgrade *upgrade, const String &ident){
		const CUpgrade *parent_upgrade = CUpgrade::Get(ident);
		if (parent_upgrade != nullptr) {
			upgrade->SetParentUpgrade(parent_upgrade);
		}
	});
	ClassDB::bind_method(D_METHOD("get_parent_upgrade"), +[](const CUpgrade *upgrade){ return const_cast<CUpgrade *>(upgrade->ParentUpgrade); });
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "parent_upgrade"), "set_parent_upgrade", "get_parent_upgrade");
	
	ClassDB::bind_method(D_METHOD("set_upgrade_class", "ident"), +[](CUpgrade *upgrade, const String &ident){
		if (upgrade->Class != nullptr) {
			upgrade->Class->Upgrades.erase(upgrade);
		}
		
		upgrade->Class = UpgradeClass::Get(ident);
		
		if (upgrade->Class != nullptr) {
			upgrade->Class->Upgrades.insert(upgrade);
		}
	});
	ClassDB::bind_method(D_METHOD("get_upgrade_class"), +[](const CUpgrade *upgrade){ return const_cast<UpgradeClass *>(upgrade->GetClass()); });
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "upgrade_class"), "set_upgrade_class", "get_upgrade_class");
	
	ClassDB::bind_method(D_METHOD("set_civilization", "ident"), +[](CUpgrade *upgrade, const String &ident){ upgrade->Civilization = CCivilization::Get(ident); });
	ClassDB::bind_method(D_METHOD("get_civilization"), +[](const CUpgrade *upgrade){ return const_cast<CCivilization *>(upgrade->GetCivilization()); });
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "civilization"), "set_civilization", "get_civilization");
	
	ClassDB::bind_method(D_METHOD("set_faction", "ident"), +[](CUpgrade *upgrade, const String &ident){ upgrade->Faction = CFaction::Get(ident); });
	ClassDB::bind_method(D_METHOD("get_faction"), +[](const CUpgrade *upgrade){ return const_cast<CFaction *>(upgrade->GetFaction()); });
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "faction"), "set_faction", "get_faction");
	
	ClassDB::bind_method(D_METHOD("set_ability", "ability"), +[](CUpgrade *upgrade, const bool ability){ upgrade->Ability = ability; });
	ClassDB::bind_method(D_METHOD("is_ability"), &CUpgrade::IsAbility);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "ability"), "set_ability", "is_ability");
	
	ClassDB::bind_method(D_METHOD("set_item_slot", "ident"), +[](CUpgrade *upgrade, const String &ident){ upgrade->ItemSlot = ::ItemSlot::Get(ident); });
	ClassDB::bind_method(D_METHOD("get_item_slot"), +[](const CUpgrade *upgrade){ return const_cast<::ItemSlot *>(upgrade->GetItemSlot()); });
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "item_slot"), "set_item_slot", "get_item_slot");
	
	ClassDB::bind_method(D_METHOD("set_item", "ident"), +[](CUpgrade *upgrade, const String &ident){ upgrade->Item = CUnitType::Get(ident); });
	ClassDB::bind_method(D_METHOD("get_item"), +[](const CUpgrade *upgrade){ return const_cast<CUnitType *>(upgrade->GetItem()); });
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "item"), "set_item", "get_item");
	
	ClassDB::bind_method(D_METHOD("set_magic_level", "magic_level"), +[](CUpgrade *upgrade, const int magic_level){ upgrade->MagicLevel = magic_level; });
	ClassDB::bind_method(D_METHOD("get_magic_level"), &CUpgrade::GetMagicLevel);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "magic_level"), "set_magic_level", "get_magic_level");
	
	ClassDB::bind_method(D_METHOD("set_effects_string", "effects_string"), +[](CUpgrade *upgrade, const String &effects_string){ upgrade->EffectsString = effects_string; });
	ClassDB::bind_method(D_METHOD("get_effects_string"), &CUpgrade::GetEffectsString);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "effects_string"), "set_effects_string", "get_effects_string");
	
	ClassDB::bind_method(D_METHOD("set_requirements_string", "requirements_string"), +[](CUpgrade *upgrade, const String &requirements_string){ upgrade->RequirementsString = requirements_string; });
	ClassDB::bind_method(D_METHOD("get_requirements_string"), &CUpgrade::GetRequirementsString);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "requirements_string"), "set_requirements_string", "get_requirements_string");
	
	ClassDB::bind_method(D_METHOD("add_to_epithets", "epithet"), +[](CUpgrade *upgrade, const String &epithet){ upgrade->Epithets.push_back(epithet); });
	ClassDB::bind_method(D_METHOD("remove_from_epithets", "epithet"), +[](CUpgrade *upgrade, const String &epithet){ upgrade->Epithets.erase(std::remove(upgrade->Epithets.begin(), upgrade->Epithets.end(), epithet), upgrade->Epithets.end()); });
	ClassDB::bind_method(D_METHOD("get_epithets"), +[](const CUpgrade *upgrade){ return ContainerToGodotArray(upgrade->GetEpithets()); });
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
	//
	//  Free the upgrade modifiers.
	//
	for (size_t i = 0; i < CUpgradeModifier::UpgradeModifiers.size(); ++i) {
		delete CUpgradeModifier::UpgradeModifiers[i];
	}
	CUpgradeModifier::UpgradeModifiers.clear();
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
	for (CUnitType *unit_type : CUnitType::GetAll()) {
		file.printf("DefineUnitAllow(\"%s\", ", unit_type->Ident.c_str());
		for (int p = 0; p < PlayerMax; ++p) {
			if (p) {
				file.printf(", ");
			}
			file.printf("%d", CPlayer::Players[p]->Allow.Units[unit_type->GetIndex()]);
		}
		file.printf(")\n");
	}
	file.printf("\n");

	//
	//  Save the upgrades
	//
	for (const CUpgrade *upgrade : CUpgrade::GetAll()) {
		file.printf("DefineAllow(\"%s\", \"", upgrade->Ident.c_str());
		for (int p = 0; p < PlayerMax; ++p) {
			file.printf("%c", CPlayer::Players[p]->Allow.Upgrades[upgrade->GetIndex()]);
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
	CUpgrade *upgrade = CUpgrade::GetOrAdd(upgrade_ident);
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Parent")) {
			const CUpgrade *parent_upgrade = CUpgrade::Get(LuaToString(l, -1));
			if (parent_upgrade != nullptr) {
				upgrade->SetParentUpgrade(parent_upgrade);
			}
		} else if (!strcmp(value, "Name")) {
			upgrade->Name = LuaToString(l, -1);
		} else if (!strcmp(value, "Icon")) {
			CIcon *icon = CIcon::Get(LuaToString(l, -1));
			upgrade->Icon = icon;
			if (icon != nullptr) {
				icon->Load();
			}
		} else if (!strcmp(value, "Class")) {
			std::string class_name = LuaToString(l, -1);
			
			if (!class_name.empty()) {
				UpgradeClass *upgrade_class = UpgradeClass::Get(class_name);
				if (upgrade_class != nullptr) {
					upgrade->Class = upgrade_class;
					upgrade_class->Upgrades.insert(upgrade);
				}
			} else {
				upgrade->Class = nullptr;
			}
		} else if (!strcmp(value, "Civilization")) {
			std::string civilization_name = LuaToString(l, -1);
			CCivilization *civilization = CCivilization::Get(civilization_name);
			if (civilization) {
				upgrade->Civilization = civilization;
			}
		} else if (!strcmp(value, "Faction")) {
			std::string faction_name = LuaToString(l, -1);
			CFaction *faction = CFaction::Get(faction_name);
			if (faction) {
				upgrade->Faction = faction;
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
		} else if (!strcmp(value, "ItemSlot")) {
			upgrade->ItemSlot = ItemSlot::Get(LuaToString(l, -1));
		} else if (!strcmp(value, "MagicPrefix")) {
			upgrade->MagicPrefix = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "MagicSuffix")) {
			upgrade->MagicSuffix = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "RunicAffix")) {
			upgrade->RunicAffix = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "UniqueOnly")) {
			upgrade->UniqueOnly = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Work")) {
			const ItemClass *work_type = ItemClass::Get(LuaToString(l, -1));
			if (work_type != nullptr) {
				upgrade->Work = work_type;
			} else {
				LuaError(l, "Work item class doesn't exist.");
			}
		} else if (!strcmp(value, "Item")) {
			CUnitType *item = CUnitType::Get(LuaToString(l, -1));
			if (item != nullptr) {
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
		} else if (!strcmp(value, "CivilizationPriorities")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				std::string civilization_ident = LuaToString(l, -1, j + 1);
				CCivilization *priority_civilization = CCivilization::Get(civilization_ident);
				++j;
				if (!priority_civilization) {
					continue;
				}

				int priority = LuaToNumber(l, -1, j + 1);

				priority_civilization->UpgradePriorities[upgrade] = priority;
			}
		} else if (!strcmp(value, "FactionPriorities")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				std::string faction_ident = LuaToString(l, -1, j + 1);
				CFaction *priority_faction = CFaction::Get(faction_ident);
				if (!priority_faction) {
					LuaError(l, "Faction \"%s\" doesn't exist." _C_ faction_ident.c_str());
				}
				++j;
				
				int priority = LuaToNumber(l, -1, j + 1);

				priority_faction->UpgradePriorities[upgrade] = priority;
			}
		} else if (!strcmp(value, "ItemPrefix")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				const ItemClass *item_class = ItemClass::Get(LuaToString(l, -1, j + 1));
				if (item_class == nullptr) {
					LuaError(l, "Item class doesn't exist.");
				}
				
				upgrade->ItemPrefix.insert(item_class);
			}
		} else if (!strcmp(value, "ItemSuffix")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				const ItemClass *item_class = ItemClass::Get(LuaToString(l, -1, j + 1));
				if (item_class == nullptr) {
					LuaError(l, "Item class doesn't exist.");
				}

				upgrade->ItemSuffix.insert(item_class);
			}
		} else if (!strcmp(value, "IncompatibleAffixes")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				const CUpgrade *affix_upgrade = CUpgrade::Get(LuaToString(l, -1, j + 1));
				if (affix_upgrade == nullptr) {
					LuaError(l, "Upgrade doesn't exist.");
				}

				upgrade->IncompatibleAffixes[affix_upgrade->GetIndex()] = true;
			}
		} else if (!strcmp(value, "ScaledCostUnits")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				CUnitType *scaled_cost_unit = CUnitType::Get(LuaToString(l, -1, j + 1));
				if (scaled_cost_unit == nullptr) {
					LuaError(l, "Unit type doesn't exist.");
				}

				upgrade->ScaledCostUnits.push_back(scaled_cost_unit);
			}
		} else if (!strcmp(value, "WeaponClasses")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				const ItemClass *item_class = ItemClass::Get(LuaToString(l, -1, j + 1));
				if (item_class == nullptr) {
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
				String epithet = LuaToString(l, -1, j + 1);

				upgrade->Epithets.push_back(epithet);
			}
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	
	//set the upgrade's civilization and class here
	if (upgrade->Class != nullptr) { //if class is defined, then use this upgrade to help build the classes table, and add this upgrade to the civilization class table (if the civilization is defined)
		UpgradeClass *upgrade_class = upgrade->Class;
		
		if (upgrade->Faction != nullptr) {
			upgrade->Faction->ClassUpgrades[upgrade_class] = upgrade;
		} else if (upgrade->Civilization != nullptr) {
			upgrade->Civilization->ClassUpgrades[upgrade_class] = upgrade;
		}
	}
	
	for (CUpgrade *other_upgrade : CUpgrade::GetAll()) { //add the upgrade to the incompatible affix's counterpart list here
		if (upgrade->IncompatibleAffixes[other_upgrade->GetIndex()]) {
			other_upgrade->IncompatibleAffixes[upgrade->GetIndex()] = true;
		}
	}
	
	upgrade->Initialized = true;
	
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

	std::string upgrade_ident = LuaToString(l, 1);
	CUpgrade *upgrade = CUpgrade::Get(upgrade_ident);
	if (upgrade == nullptr) {
		LuaError(l, "Error when defining upgrade modifier: upgrade \"%s\" doesn't exist." _C_ upgrade_ident.c_str());
	}
	
	um->UpgradeId = upgrade->GetIndex();
	
	//Wyrmgus start
	upgrade->UpgradeModifiers.push_back(um);
	//Wyrmgus end

	for (int j = 1; j < args; ++j) {
		if (!lua_istable(l, j + 1)) {
			LuaError(l, "incorrect argument");
		}
		const char *key = LuaToString(l, j + 1, 1);
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
			const CUnitType *unit_type = CUnitType::Get(value);
			um->Modifier.SetUnitStock(unit_type, LuaToNumber(l, j + 1, 3));
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
//				um->ChangeUpgrades[CUpgrade::Get(value)->GetIndex()] = LuaToNumber(l, j + 1, 3);
				um->ChangeUpgrades[CUpgrade::Get(value)->GetIndex()] = *strdup(LuaToString(l, j + 1, 3));
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
			um->ApplyToUnitTypes.insert(CUnitType::Get(value));
		} else if (!strcmp(key, "convert-to")) {
			const char *value = LuaToString(l, j + 1, 2);
			um->ConvertTo = CUnitType::Get(value);
		//Wyrmgus start
		} else if (!strcmp(key, "research-speed")) {
			um->SpeedResearch = LuaToNumber(l, j + 1, 2);
		} else if (!strcmp(key, "change-civilization-to")) {
			const char *civilization_ident = LuaToString(l, j + 1, 2);
			CCivilization *civilization = CCivilization::Get(civilization_ident);
			if (civilization) {
				um->ChangeCivilizationTo = civilization->GetIndex();
			}
		} else if (!strcmp(key, "change-faction-to")) {
			std::string faction_ident = LuaToString(l, j + 1, 2);
			um->ChangeFactionTo = CFaction::Get(faction_ident);
			
			if (um->ChangeFactionTo == nullptr) {
				LuaError(l, "Faction \"%s\" doesn't exist.'" _C_ faction_ident.c_str());
			}
		} else if (!strcmp(key, "change-dynasty-to")) {
			std::string dynasty_ident = LuaToString(l, j + 1, 2);
			um->ChangeDynastyTo = CDynasty::Get(dynasty_ident);
			
			if (um->ChangeDynastyTo == nullptr) {
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

	CUpgradeModifier::UpgradeModifiers.push_back(um);
	
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
		AllowUnitId(*CPlayer::Players[i], id, LuaToNumber(l, j + 1));
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
					AllowUnitId(*CPlayer::Players[i], id, UnitMax);
				} else if (ids[i] == 'F') {
					AllowUnitId(*CPlayer::Players[i], id, 0);
				}
			}
		} else if (!strncmp(ident, "upgrade-", 8)) {
			const CUpgrade *upgrade = CUpgrade::Get(ident);
			if (upgrade != nullptr) {
				for (int i = 0; i < n; ++i) {
					AllowUpgradeId(*CPlayer::Players[i], upgrade->GetIndex(), ids[i]);
				}
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
		if (unit->Trait != nullptr) { //remove previous trait, if any
			if (!GameSettings.NoRandomness) { // if in no randomness setting, don't apply trait modifiers
				IndividualUpgradeLost(*unit, unit->Trait);
			}
		}
		unit->Trait = nullptr;
	} else {
		DebugPrint(" wrong ident %s\n" _C_ ident);
	}
	return 0;
}

static int CclGetUpgrades(lua_State *l)
{
	lua_createtable(l, CUpgrade::GetAll().size(), 0);
	for (size_t i = 1; i <= CUpgrade::GetAll().size(); ++i)
	{
		lua_pushstring(l, CUpgrade::GetAll()[i-1]->Ident.c_str());
		lua_rawseti(l, -2, i);
	}
	return 1;
}

static int CclGetItemPrefixes(lua_State *l)
{
	std::vector<const CUpgrade *> item_prefixes;
	for (const CUpgrade *upgrade : CUpgrade::GetAll()) {
		if (upgrade->MagicPrefix) {
			item_prefixes.push_back(upgrade);
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
	std::vector<const CUpgrade *> item_suffixes;
	for (const CUpgrade *upgrade : CUpgrade::GetAll()) {
		if (upgrade->MagicSuffix && !upgrade->RunicAffix) {
			item_suffixes.push_back(upgrade);
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
	std::vector<const CUpgrade *> item_suffixes;
	for (const CUpgrade *upgrade : CUpgrade::GetAll()) {
		if (upgrade->MagicSuffix && upgrade->RunicAffix) {
			item_suffixes.push_back(upgrade);
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
	std::vector<const CUpgrade *> literary_works;
	for (const CUpgrade *upgrade : CUpgrade::GetAll()) {
		if (upgrade->Work != nullptr) {
			literary_works.push_back(upgrade);
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
		lua_pushstring(l, upgrade->GetName().utf8().get_data());
		return 1;
	} else if (!strcmp(data, "Class")) {
		if (upgrade->GetClass() != nullptr) {
			lua_pushstring(l, upgrade->GetClass()->Ident.c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Civilization")) {
		if (upgrade->GetCivilization() != nullptr) {
			lua_pushstring(l, upgrade->GetCivilization()->GetIdent().utf8().get_data());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Faction")) {
		if (upgrade->GetFaction() != nullptr) {
			lua_pushstring(l, upgrade->GetFaction()->GetIdent().utf8().get_data());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Icon")) {
		if (upgrade->GetIcon()) {
			lua_pushstring(l, upgrade->GetIcon()->GetIdent().utf8().get_data());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Description")) {
		lua_pushstring(l, upgrade->GetDescription().utf8().get_data());
		return 1;
	} else if (!strcmp(data, "Background")) {
		lua_pushstring(l, upgrade->GetBackground().utf8().get_data());
		return 1;
	} else if (!strcmp(data, "Quote")) {
		lua_pushstring(l, upgrade->GetQuote().utf8().get_data());
		return 1;
	} else if (!strcmp(data, "EffectsString")) {
		lua_pushstring(l, upgrade->GetEffectsString().utf8().get_data());
		return 1;
	} else if (!strcmp(data, "RequirementsString")) {
		lua_pushstring(l, upgrade->GetRequirementsString().utf8().get_data());
		return 1;
	} else if (!strcmp(data, "Ability")) {
		lua_pushboolean(l, upgrade->IsAbility());
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
			const ItemClass *item_class = ItemClass::Get(item_class_name);
			if (item_class == nullptr) {
				LuaError(l, "Item class \"%s\" doesn't exist." _C_ item_class_name.c_str());
			}
			lua_pushboolean(l, upgrade->ItemPrefix.find(item_class) != upgrade->ItemPrefix.end());
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
			const ItemClass *item_class = ItemClass::Get(item_class_name);
			if (item_class == nullptr) {
				LuaError(l, "Item class \"%s\" doesn't exist." _C_ item_class_name.c_str());
			}
			lua_pushboolean(l, upgrade->ItemSuffix.find(item_class) != upgrade->ItemSuffix.end());
			return 1;
		}
	} else if (!strcmp(data, "AppliesTo")) { //to which unit types or item classes this upgrade applies
		std::vector<std::string> applies_to;
		for (const ItemClass *item_class : ItemClass::GetAll()) {
			if (upgrade->ItemPrefix.find(item_class) != upgrade->ItemPrefix.end() || upgrade->ItemSuffix.find(item_class) != upgrade->ItemSuffix.end()) {
				applies_to.push_back(item_class->Ident);
			}
		}

		for (size_t z = 0; z < upgrade->UpgradeModifiers.size(); ++z) {
			for (CUnitType *unit_type : CUnitType::GetAll()) {
				if (unit_type->Ident.find("template") != std::string::npos) { //if is a template, continue
					continue;
				}
				
				if (upgrade->UpgradeModifiers[z]->AppliesToUnitType(unit_type) || std::find(unit_type->Affixes.begin(), unit_type->Affixes.end(), upgrade) != unit_type->Affixes.end()) {
					applies_to.push_back(unit_type->Ident);
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
	const CUnitType *type = CUnitType::Get(ident);

	if (type) {
		return type->GetIndex();
	}
	DebugPrint(" fix this %s\n" _C_ ident.c_str());
	Assert(0);
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
			if (player.Ai->UnitTypeRequests[i].Type->GetIndex() == src.GetIndex()) {
				player.Ai->UnitTypeRequests[i].Type = &dst;
			}
		}

		for (unsigned int i = 0; i < player.Ai->UpgradeToRequests.size(); ++i) {
			if (player.Ai->UpgradeToRequests[i]->GetIndex() == src.GetIndex()) {
				player.Ai->UpgradeToRequests[i] = &dst;
			}
		}
		
		for (unsigned int i = 0; i < player.Ai->Force.Size(); ++i) {
			AiForce &force = player.Ai->Force[i];

			for (unsigned int j = 0; j < force.UnitTypes.size(); ++j) {
				if (force.UnitTypes[j].Type->GetIndex() == src.GetIndex()) {
					force.UnitTypes[j].Type = &dst;
				}
			}
		}

		for (unsigned int i = 0; i < player.Ai->UnitTypeBuilt.size(); ++i) {
			if (player.Ai->UnitTypeBuilt[i].Type->GetIndex() == src.GetIndex()) {
				player.Ai->UnitTypeBuilt[i].Type = &dst;
			}
		}
	}
	//Wyrmgus end
	for (int i = 0; i < player.GetUnitCount(); ++i) {
		CUnit &unit = player.GetUnit(i);

		//Wyrmgus start
		if (&unit == nullptr) {
			fprintf(stderr, "Error in ConvertUnitTypeTo: unit %d, of player %d is null.\n", i, player.GetIndex());
			continue;
		}
		//Wyrmgus end

		//  Convert already existing units to this type.
		//Wyrmgus start
//		if (unit.GetType() == &src) {
		if (unit.GetType() == &src && !unit.Character) { //don't do this for persistent characters
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

	int pn = player.GetIndex();

	//Wyrmgus start
	if (um->SpeedResearch != 0) {
		player.SpeedResearch += um->SpeedResearch;
	}
	if (um->ChangeCivilizationTo != -1 && GameRunning && um->ChangeCivilizationTo != player.Race) {
		player.SetCivilization(um->ChangeCivilizationTo);
	}
	if (um->ChangeFactionTo != nullptr && GameRunning && (um->ChangeFactionTo->GetCivilization()->GetIndex() != player.Race || um->ChangeFactionTo != player.GetFaction())) {
		if (um->ChangeFactionTo->GetCivilization()->GetIndex() != player.Race) {
			player.SetCivilization(um->ChangeFactionTo->GetCivilization()->GetIndex());
		}
		player.SetFaction(um->ChangeFactionTo);
	}
	if (um->ChangeDynastyTo != nullptr && um->ChangeDynastyTo != player.Dynasty) {
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
		if (player.Allow.Upgrades[um->RemoveUpgrades[i]->GetIndex()] == 'R') {
			UpgradeLost(player, um->RemoveUpgrades[i]);
		}
	}
	//Wyrmgus end

	for (CUnitType *unit_type : CUnitType::GetAll()) {
		CUnitStats &stat = unit_type->Stats[pn];
		// add/remove allowed units

		//Wyrmgus start
		if (stat.Variables == nullptr) { // unit type's stats not initialized
			break;
		}
		//Wyrmgus end

		// FIXME: check if modify is allowed

		player.Allow.Units[unit_type->GetIndex()] += um->ChangeUnits[unit_type->GetIndex()];

		// this modifier should be applied to unittype id == unit_type->GetIndex()
		if (um->AppliesToUnitType(unit_type)) {

			// if a unit type's supply is changed, we need to update the player's supply accordingly
			if (um->Modifier.Variables[SUPPLY_INDEX].Value) {
				std::vector<CUnit *> unitupgrade;

				FindUnitsByType(*unit_type, unitupgrade);
				for (size_t j = 0; j != unitupgrade.size(); ++j) {
					CUnit &unit = *unitupgrade[j];
					if (unit.GetPlayer()->GetIndex() == pn && unit.IsAlive()) {
						unit.GetPlayer()->Supply += um->Modifier.Variables[SUPPLY_INDEX].Value;
					}
				}
			}
			
			// if a unit type's demand is changed, we need to update the player's demand accordingly
			if (um->Modifier.Variables[DEMAND_INDEX].Value) {
				std::vector<CUnit *> unitupgrade;

				FindUnitsByType(*unit_type, unitupgrade);
				for (size_t j = 0; j != unitupgrade.size(); ++j) {
					CUnit &unit = *unitupgrade[j];
					if (unit.GetPlayer()->GetIndex() == pn && unit.IsAlive()) {
						unit.GetPlayer()->Demand += um->Modifier.Variables[DEMAND_INDEX].Value;
					}
				}
			}
			
			// upgrade costs :)
			for (unsigned int j = 0; j < MaxCosts; ++j) {
				stat.Costs[j] += um->Modifier.Costs[j];
				stat.Storing[j] += um->Modifier.Storing[j];
				if (um->Modifier.ImproveIncomes[j]) {
					if (!stat.ImproveIncomes[j]) {
						stat.ImproveIncomes[j] += CResource::GetAll()[j]->DefaultIncome + um->Modifier.ImproveIncomes[j];
					} else {
						stat.ImproveIncomes[j] += um->Modifier.ImproveIncomes[j];
					}
					//update player's income
					std::vector<CUnit *> unitupgrade;
					FindUnitsByType(*unit_type, unitupgrade);
					if (unitupgrade.size() > 0) {
						player.Incomes[j] = std::max(player.Incomes[j], stat.ImproveIncomes[j]);
					}
				}
				//Wyrmgus start
				stat.ResourceDemand[j] += um->Modifier.ResourceDemand[j];
				//Wyrmgus end
			}
			
			for (std::map<const CUnitType *, int>::const_iterator iterator = um->Modifier.UnitStock.begin(); iterator != um->Modifier.UnitStock.end(); ++iterator) {
				const CUnitType *unit_type = iterator->first;
				const int unit_stock = iterator->second;
				if (unit_stock != 0) {
					stat.ChangeUnitStock(unit_type, unit_stock);
				}
			}

			int varModified = 0;
			for (unsigned int j = 0; j < UnitTypeVar.GetNumberVariable(); j++) {
				varModified |= um->Modifier.Variables[j].Value
							   | um->Modifier.Variables[j].Max
							   | um->Modifier.Variables[j].Increase
							   | um->Modifier.Variables[j].Enable
							   | um->ModifyPercent[j];
				stat.Variables[j].Enable |= um->Modifier.Variables[j].Enable;
				if (um->ModifyPercent[j]) {
					if (j != MANA_INDEX || um->ModifyPercent[j] < 0) {
						stat.Variables[j].Value += stat.Variables[j].Value * um->ModifyPercent[j] / 100;
					}
					stat.Variables[j].Max += stat.Variables[j].Max * um->ModifyPercent[j] / 100;
				} else {
					if (j != MANA_INDEX || um->Modifier.Variables[j].Value < 0) {
						stat.Variables[j].Value += um->Modifier.Variables[j].Value;
					}
					stat.Variables[j].Max += um->Modifier.Variables[j].Max;
					stat.Variables[j].Increase += um->Modifier.Variables[j].Increase;
				}

				stat.Variables[j].Max = std::max(stat.Variables[j].Max, 0);
				//Wyrmgus start
//				stat.Variables[j].Value = std::clamp(stat.Variables[j].Value, 0, stat.Variables[j].Max);
				if (stat.Variables[j].Max > 0) {
					stat.Variables[j].Value = std::clamp(stat.Variables[j].Value, 0, stat.Variables[j].Max);
				}
				//Wyrmgus end
			}
			
			if (um->Modifier.Variables[TRADECOST_INDEX].Value) {
				std::vector<CUnit *> unitupgrade;

				FindUnitsByType(*unit_type, unitupgrade);
				if (unitupgrade.size() > 0) {
					player.TradeCost = std::min(player.TradeCost, stat.Variables[TRADECOST_INDEX].Value);
				}
			}

			// And now modify ingame units
			//Wyrmgus start
			std::vector<CUnit *> unitupgrade;

			FindUnitsByType(*unit_type, unitupgrade, true);
			//Wyrmgus end
			
			if (varModified) {
				//Wyrmgus start
//				std::vector<CUnit *> unitupgrade;

//				FindUnitsByType(*unit_type, unitupgrade, true);
				//Wyrmgus end
				for (size_t j = 0; j != unitupgrade.size(); ++j) {
					CUnit &unit = *unitupgrade[j];

					if (unit.GetPlayer()->GetIndex() != player.GetIndex()) {
						continue;
					}
					
					//Wyrmgus start
					if (
						CUpgrade::Get(um->UpgradeId)->GetItemSlot() != nullptr
						&& unit.EquippedItems.find(CUpgrade::Get(um->UpgradeId)->GetItemSlot()) != unit.EquippedItems.end()
						&& unit.EquippedItems.find(CUpgrade::Get(um->UpgradeId)->GetItemSlot())->second.size() > 0
					) { //if the unit already has an item equipped of the same equipment type as this upgrade, don't apply the modifier to it
						continue;
					}
					
					if (unit.Character && !strncmp(CUpgrade::Get(um->UpgradeId)->Ident.c_str(), "upgrade-deity-", 14)) { //heroes choose their own deities
						continue;
					}
					//Wyrmgus end
					
					for (unsigned int j = 0; j < UnitTypeVar.GetNumberVariable(); j++) {
						unit.Variable[j].Enable |= um->Modifier.Variables[j].Enable;
						if (um->ModifyPercent[j]) {
							if (j != MANA_INDEX || um->ModifyPercent[j] < 0) {
								unit.Variable[j].Value += unit.Variable[j].Value * um->ModifyPercent[j] / 100;
							}
							unit.Variable[j].Max += unit.Variable[j].Max * um->ModifyPercent[j] / 100;
						} else {
							if (j != MANA_INDEX || um->Modifier.Variables[j].Value < 0) {
								unit.Variable[j].Value += um->Modifier.Variables[j].Value;
							}
							unit.Variable[j].Increase += um->Modifier.Variables[j].Increase;
						}

						unit.Variable[j].Max += um->Modifier.Variables[j].Max;
						unit.Variable[j].Max = std::max(unit.Variable[j].Max, 0);
						if (unit.Variable[j].Max > 0) {
							unit.Variable[j].Value = std::clamp(unit.Variable[j].Value, 0, unit.Variable[j].Max);
						}
						//Wyrmgus start
						if (j == ATTACKRANGE_INDEX && unit.Container) {
							unit.Container->UpdateContainerAttackRange();
						} else if (j == LEVEL_INDEX || j == POINTS_INDEX) {
							unit.UpdateXPRequired();
						} else if (IsKnowledgeVariable(j)) {
							unit.CheckKnowledgeChange(j, um->Modifier.Variables[j].Value);
						} else if ((j == SIGHTRANGE_INDEX || j == DAYSIGHTRANGEBONUS_INDEX || j == NIGHTSIGHTRANGEBONUS_INDEX) && !unit.Removed) {
							// If Sight range is upgraded, we need to change EVERY unit
							// to the new range, otherwise the counters get confused.
							MapUnmarkUnitSight(unit);
							UpdateUnitSightRange(unit);
							MapMarkUnitSight(unit);
						}
						//Wyrmgus end
					}
					
					for (std::map<const CUnitType *, int>::const_iterator iterator = um->Modifier.UnitStock.begin(); iterator != um->Modifier.UnitStock.end(); ++iterator) {
						const CUnitType *unit_type = iterator->first;
						const int unit_stock = iterator->second;
						if (unit_stock < 0) {
							unit.ChangeUnitStock(unit_type, unit_stock);
						}
					}
				}
			}
			
			//Wyrmgus start
			for (size_t j = 0; j != unitupgrade.size(); ++j) {
				CUnit &unit = *unitupgrade[j];

				if (unit.GetPlayer()->GetIndex() != player.GetIndex()) {
					continue;
				}
				
				//add or remove starting abilities from the unit if the upgrade enabled/disabled them
				for (const CUpgrade *ability_upgrade : unit.GetType()->StartingAbilities) {
					if (!unit.GetIndividualUpgrade(ability_upgrade) && CheckDependencies(ability_upgrade, &unit)) {
						IndividualUpgradeAcquire(unit, ability_upgrade);
					} else if (unit.GetIndividualUpgrade(ability_upgrade) && !CheckDependencies(ability_upgrade, &unit)) {
						IndividualUpgradeLost(unit, ability_upgrade);
					}
				}
				
				//change variation if current one becomes forbidden
				const UnitTypeVariation *current_variation = unit.GetVariation();
				if (current_variation) {
					bool upgrade_forbidden = false;
					for (const CUpgrade *forbidden_upgrade : current_variation->UpgradesForbidden) {
						if (um->UpgradeId == forbidden_upgrade->GetIndex()) {
							upgrade_forbidden = true;
							break;
						}
					}
					if (upgrade_forbidden == true) {
						unit.ChooseVariation();
					}
				}
				for (int i = 0; i < MaxImageLayers; ++i) {
					const UnitTypeVariation *current_layer_variation = unit.GetLayerVariation(i);
					if (current_layer_variation) {
						bool upgrade_forbidden = false;
						for (const CUpgrade *forbidden_upgrade : current_layer_variation->UpgradesForbidden) {
							if (um->UpgradeId == forbidden_upgrade->GetIndex()) {
								upgrade_forbidden = true;
								break;
							}
						}

						if (upgrade_forbidden == true) {
							unit.ChooseVariation(nullptr, i);
						}
					}
				}
				unit.UpdateButtonIcons();
			}
			//Wyrmgus end
			
			if (um->ConvertTo) {
				ConvertUnitTypeTo(player, *unit_type, *um->ConvertTo);
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

	int pn = player.GetIndex();

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

	for (CUnitType *unit_type : CUnitType::GetAll()) {
		CUnitStats &stat = unit_type->Stats[pn];
		// add/remove allowed units

		//Wyrmgus start
		if (stat.Variables == nullptr) { // unit types stats not initialized
			break;
		}
		//Wyrmgus end
		
		// FIXME: check if modify is allowed

		player.Allow.Units[unit_type->GetIndex()] -= um->ChangeUnits[unit_type->GetIndex()];

		// this modifier should be applied to unittype id == unit_type->GetIndex()
		if (um->AppliesToUnitType(unit_type)) {
			// if a unit type's supply is changed, we need to update the player's supply accordingly
			if (um->Modifier.Variables[SUPPLY_INDEX].Value) {
				std::vector<CUnit *> unitupgrade;

				FindUnitsByType(*unit_type, unitupgrade);
				for (size_t j = 0; j != unitupgrade.size(); ++j) {
					CUnit &unit = *unitupgrade[j];
					if (unit.GetPlayer()->GetIndex() == pn && unit.IsAlive()) {
						unit.GetPlayer()->Supply -= um->Modifier.Variables[SUPPLY_INDEX].Value;
					}
				}
			}
			
			// if a unit type's demand is changed, we need to update the player's demand accordingly
			if (um->Modifier.Variables[DEMAND_INDEX].Value) {
				std::vector<CUnit *> unitupgrade;

				FindUnitsByType(*unit_type, unitupgrade);
				for (size_t j = 0; j != unitupgrade.size(); ++j) {
					CUnit &unit = *unitupgrade[j];
					if (unit.GetPlayer()->GetIndex() == pn && unit.IsAlive()) {
						unit.GetPlayer()->Demand -= um->Modifier.Variables[DEMAND_INDEX].Value;
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
					int m = CResource::GetAll()[j]->DefaultIncome;

					for (int k = 0; k < player.GetUnitCount(); ++k) {
						//Wyrmgus start
//						m = std::max(m, player.GetUnit(k).GetType()->Stats[player.GetIndex()].ImproveIncomes[j]);
						if (player.GetUnit(k).GetType() != nullptr) {
							m = std::max(m, player.GetUnit(k).GetType()->Stats[player.GetIndex()].ImproveIncomes[j]);
						}
						//Wyrmgus end
					}
					player.Incomes[j] = m;
				}
				//Wyrmgus start
				stat.ResourceDemand[j] -= um->Modifier.ResourceDemand[j];
				//Wyrmgus end
			}
			
			for (std::map<const CUnitType *, int>::const_iterator iterator = um->Modifier.UnitStock.begin(); iterator != um->Modifier.UnitStock.end(); ++iterator) {
				const CUnitType *unit_type = iterator->first;
				const int unit_stock = iterator->second;
				if (unit_stock != 0) {
					stat.ChangeUnitStock(unit_type, - unit_stock);
				}
			}
			
			int varModified = 0;
			for (unsigned int j = 0; j < UnitTypeVar.GetNumberVariable(); j++) {
				varModified |= um->Modifier.Variables[j].Value
							   | um->Modifier.Variables[j].Max
							   | um->Modifier.Variables[j].Increase
							   | um->Modifier.Variables[j].Enable
							   | um->ModifyPercent[j];
				stat.Variables[j].Enable |= um->Modifier.Variables[j].Enable;
				if (um->ModifyPercent[j]) {
					if (j != MANA_INDEX || um->Modifier.Variables[j].Value >= 0) {
						stat.Variables[j].Value = stat.Variables[j].Value * 100 / (100 + um->ModifyPercent[j]);
					}
					stat.Variables[j].Max = stat.Variables[j].Max * 100 / (100 + um->ModifyPercent[j]);
				} else {
					if (j != MANA_INDEX || um->Modifier.Variables[j].Value >= 0) {
						stat.Variables[j].Value -= um->Modifier.Variables[j].Value;
					}
					stat.Variables[j].Max -= um->Modifier.Variables[j].Max;
					stat.Variables[j].Increase -= um->Modifier.Variables[j].Increase;
				}

				stat.Variables[j].Max = std::max(stat.Variables[j].Max, 0);
				//Wyrmgus start
//				stat.Variables[j].Value = std::clamp(stat.Variables[j].Value, 0, stat.Variables[j].Max);
				if (stat.Variables[j].Max > 0) {
					stat.Variables[j].Value = std::clamp(stat.Variables[j].Value, 0, stat.Variables[j].Max);
				}
				//Wyrmgus end
			}
			
			if (um->Modifier.Variables[TRADECOST_INDEX].Value && (stat.Variables[TRADECOST_INDEX].Value + um->Modifier.Variables[TRADECOST_INDEX].Value) == player.TradeCost) {
				int m = DEFAULT_TRADE_COST;

				for (int k = 0; k < player.GetUnitCount(); ++k) {
					if (player.GetUnit(k).GetType() != nullptr) {
						m = std::min(m, player.GetUnit(k).GetType()->Stats[player.GetIndex()].Variables[TRADECOST_INDEX].Value);
					}
				}
				player.TradeCost = m;
			}

			//Wyrmgus start
			std::vector<CUnit *> unitupgrade;

			FindUnitsByType(*unit_type, unitupgrade, true);
			//Wyrmgus end
			
			// And now modify ingame units
			if (varModified) {
				//Wyrmgus start
				/*
				std::vector<CUnit *> unitupgrade;

				FindUnitsByType(*unit_type, unitupgrade, true);
				*/
				//Wyrmgus end
				for (size_t j = 0; j != unitupgrade.size(); ++j) {
					CUnit &unit = *unitupgrade[j];

					if (unit.GetPlayer()->GetIndex() != player.GetIndex()) {
						continue;
					}
					
					//Wyrmgus start
					if (
						CUpgrade::Get(um->UpgradeId)->GetItemSlot() != nullptr
						&& unit.EquippedItems.find(CUpgrade::Get(um->UpgradeId)->GetItemSlot()) != unit.EquippedItems.end()
						&& unit.EquippedItems.find(CUpgrade::Get(um->UpgradeId)->GetItemSlot())->second.size() > 0
					) { //if the unit already has an item equipped of the same equipment type as this upgrade, don't remove the modifier from it (it already doesn't have it)
						continue;
					}
					//Wyrmgus end
					
					for (unsigned int j = 0; j < UnitTypeVar.GetNumberVariable(); j++) {
						unit.Variable[j].Enable |= um->Modifier.Variables[j].Enable;
						if (um->ModifyPercent[j]) {
							if (j != MANA_INDEX || um->ModifyPercent[j] >= 0) {
								unit.Variable[j].Value = unit.Variable[j].Value * 100 / (100 + um->ModifyPercent[j]);
							}
							unit.Variable[j].Max = unit.Variable[j].Max * 100 / (100 + um->ModifyPercent[j]);
						} else {
							if (j != MANA_INDEX || um->Modifier.Variables[j].Value >= 0) {
								unit.Variable[j].Value -= um->Modifier.Variables[j].Value;
							}
							unit.Variable[j].Increase -= um->Modifier.Variables[j].Increase;
						}

						unit.Variable[j].Max -= um->Modifier.Variables[j].Max;
						unit.Variable[j].Max = std::max(unit.Variable[j].Max, 0);

						//Wyrmgus start
//						unit.Variable[j].Value = std::clamp(unit.Variable[j].Value, 0, unit.Variable[j].Max);
						if (unit.Variable[j].Max > 0) {
							unit.Variable[j].Value = std::clamp(unit.Variable[j].Value, 0, unit.Variable[j].Max);
						}
						//Wyrmgus end
						//Wyrmgus start
						if (j == ATTACKRANGE_INDEX && unit.Container) {
							unit.Container->UpdateContainerAttackRange();
						} else if (j == LEVEL_INDEX || j == POINTS_INDEX) {
							unit.UpdateXPRequired();
						} else if (IsKnowledgeVariable(j)) {
							unit.CheckKnowledgeChange(j, - um->Modifier.Variables[j].Value);
						} else if ((j == SIGHTRANGE_INDEX || j == DAYSIGHTRANGEBONUS_INDEX || j == NIGHTSIGHTRANGEBONUS_INDEX) && !unit.Removed) {
							// If Sight range is upgraded, we need to change EVERY unit
							// to the new range, otherwise the counters get confused.
							MapUnmarkUnitSight(unit);
							UpdateUnitSightRange(unit);
							MapMarkUnitSight(unit);
						}
						//Wyrmgus end
					}
					
					for (std::map<const CUnitType *, int>::const_iterator iterator = um->Modifier.UnitStock.begin(); iterator != um->Modifier.UnitStock.end(); ++iterator) {
						const CUnitType *unit_type = iterator->first;
						const int unit_stock = iterator->second;
						if (unit_stock > 0) {
							unit.ChangeUnitStock(unit_type, - unit_stock);
						}
					}
				}
			}
			
			//Wyrmgus start
			for (size_t j = 0; j != unitupgrade.size(); ++j) {
				CUnit &unit = *unitupgrade[j];

				if (unit.GetPlayer()->GetIndex() != player.GetIndex()) {
					continue;
				}
				
				//add or remove starting abilities from the unit if the upgrade enabled/disabled them
				for (const CUpgrade *ability_upgrade : unit.GetType()->StartingAbilities) {
					if (!unit.GetIndividualUpgrade(ability_upgrade) && CheckDependencies(ability_upgrade, &unit)) {
						IndividualUpgradeAcquire(unit, ability_upgrade);
					} else if (unit.GetIndividualUpgrade(ability_upgrade) && !CheckDependencies(ability_upgrade, &unit)) {
						IndividualUpgradeLost(unit, ability_upgrade);
					}
				}
				
				//change variation if current one becomes forbidden
				const UnitTypeVariation *current_variation = unit.GetVariation();
				if (current_variation) {
					bool upgrade_required = false;
					for (const CUpgrade *required_upgrade : current_variation->UpgradesRequired) {
						if (um->UpgradeId == required_upgrade->GetIndex()) {
							upgrade_required = true;
							break;
						}
					}
					if (upgrade_required == true) {
						unit.ChooseVariation();
					}
				}
				for (int i = 0; i < MaxImageLayers; ++i) {
					const UnitTypeVariation *current_layer_variation = unit.GetLayerVariation(i);
					if (current_layer_variation) {
						bool upgrade_required = false;
						for (const CUpgrade *required_upgrade : current_layer_variation->UpgradesRequired) {
							if (um->UpgradeId == required_upgrade->GetIndex()) {
								upgrade_required = true;
								break;
							}
						}
						if (upgrade_required == true) {
							unit.ChooseVariation(nullptr, i);
						}
					}
				}
				unit.UpdateButtonIcons();
			}
			//Wyrmgus end
			
			if (um->ConvertTo) {
				ConvertUnitTypeTo(player, *um->ConvertTo, *unit_type);
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
		if (unit.GetIndividualUpgrade(um->RemoveUpgrades[i])) {
			IndividualUpgradeLost(unit, um->RemoveUpgrades[i], true);
		}
	}

	if (um->Modifier.Variables[SUPPLY_INDEX].Value) {
		if (unit.IsAlive()) {
			unit.GetPlayer()->Supply += um->Modifier.Variables[SUPPLY_INDEX].Value;
		}
	}

	for (unsigned int j = 0; j < UnitTypeVar.GetNumberVariable(); j++) {
		unit.Variable[j].Enable |= um->Modifier.Variables[j].Enable;
		if (um->ModifyPercent[j]) {
			if (j != MANA_INDEX || um->ModifyPercent[j] < 0) {
				unit.Variable[j].Value += unit.Variable[j].Value * um->ModifyPercent[j] / 100;
			}
			unit.Variable[j].Max += unit.Variable[j].Max * um->ModifyPercent[j] / 100;
		} else {
			if (j != MANA_INDEX || um->Modifier.Variables[j].Value < 0) {
				unit.Variable[j].Value += um->Modifier.Variables[j].Value;
			}
			unit.Variable[j].Increase += um->Modifier.Variables[j].Increase;
		}
		unit.Variable[j].Max += um->Modifier.Variables[j].Max;
		unit.Variable[j].Max = std::max(unit.Variable[j].Max, 0);
		if (unit.Variable[j].Max > 0) {
			unit.Variable[j].Value = std::clamp(unit.Variable[j].Value, 0, unit.Variable[j].Max);
		}
		//Wyrmgus start
		if (j == ATTACKRANGE_INDEX && unit.Container && !SaveGameLoading) {
			unit.Container->UpdateContainerAttackRange();
		} else if (j == LEVEL_INDEX || j == POINTS_INDEX) {
			unit.UpdateXPRequired();
		} else if (IsKnowledgeVariable(j)) {
			unit.CheckKnowledgeChange(j, um->Modifier.Variables[j].Value);
		} else if (j == SIGHTRANGE_INDEX || j == DAYSIGHTRANGEBONUS_INDEX || j == NIGHTSIGHTRANGEBONUS_INDEX) {
			if (!unit.Removed && !SaveGameLoading) {
				MapUnmarkUnitSight(unit);
				UpdateUnitSightRange(unit);
				MapMarkUnitSight(unit);
			}
		}
		//Wyrmgus end
	}
	
	for (std::map<const CUnitType *, int>::const_iterator iterator = um->Modifier.UnitStock.begin(); iterator != um->Modifier.UnitStock.end(); ++iterator) {
		const CUnitType *unit_type = iterator->first;
		const int unit_stock = iterator->second;
		if (unit_stock < 0) {
			unit.ChangeUnitStock(unit_type, unit_stock);
		}
	}
	
	//Wyrmgus start
	//change variation if current one becomes forbidden
	const UnitTypeVariation *current_variation = unit.GetVariation();
	if (current_variation) {
		bool upgrade_forbidden = false;
		for (const CUpgrade *forbidden_upgrade : current_variation->UpgradesForbidden) {
			if (um->UpgradeId == forbidden_upgrade->GetIndex()) {
				upgrade_forbidden = true;
				break;
			}
		}
		if (upgrade_forbidden == true) {
			unit.ChooseVariation();
		}
	}
	for (int i = 0; i < MaxImageLayers; ++i) {
		const UnitTypeVariation *current_layer_variation = unit.GetLayerVariation(i);
		if (current_layer_variation) {
			bool upgrade_forbidden = false;
			for (const CUpgrade *forbidden_upgrade : current_layer_variation->UpgradesForbidden) {
				if (um->UpgradeId == forbidden_upgrade->GetIndex()) {
					upgrade_forbidden = true;
					break;
				}
			}
			if (upgrade_forbidden == true) {
				unit.ChooseVariation(nullptr, i);
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
			unit.GetPlayer()->Supply -= um->Modifier.Variables[SUPPLY_INDEX].Value;
		}
	}
	
	for (unsigned int j = 0; j < UnitTypeVar.GetNumberVariable(); j++) {
		unit.Variable[j].Enable |= um->Modifier.Variables[j].Enable;
		if (um->ModifyPercent[j]) {
			if (j != MANA_INDEX || um->ModifyPercent[j] >= 0) {
				unit.Variable[j].Value = unit.Variable[j].Value * 100 / (100 + um->ModifyPercent[j]);
			}
			unit.Variable[j].Max = unit.Variable[j].Max * 100 / (100 + um->ModifyPercent[j]);
		} else {
			if (j != MANA_INDEX || um->Modifier.Variables[j].Value >= 0) {
				unit.Variable[j].Value -= um->Modifier.Variables[j].Value;
			}
			unit.Variable[j].Increase -= um->Modifier.Variables[j].Increase;
		}
		unit.Variable[j].Max -= um->Modifier.Variables[j].Max;
		unit.Variable[j].Max = std::max(unit.Variable[j].Max, 0);
		if (unit.Variable[j].Max > 0) {
			unit.Variable[j].Value = std::clamp(unit.Variable[j].Value, 0, unit.Variable[j].Max);
		}
		//Wyrmgus start
		if (j == ATTACKRANGE_INDEX && unit.Container) {
			unit.Container->UpdateContainerAttackRange();
		} else if (j == LEVEL_INDEX || j == POINTS_INDEX) {
			unit.UpdateXPRequired();
		} else if (IsKnowledgeVariable(j)) {
			unit.CheckKnowledgeChange(j, - um->Modifier.Variables[j].Value);
		} else if (j == SIGHTRANGE_INDEX || j == DAYSIGHTRANGEBONUS_INDEX || j == NIGHTSIGHTRANGEBONUS_INDEX) {
			if (!unit.Removed && !SaveGameLoading) {
				MapUnmarkUnitSight(unit);
				UpdateUnitSightRange(unit);
				MapMarkUnitSight(unit);
			}
		}
		//Wyrmgus end
	}
	
	for (std::map<const CUnitType *, int>::const_iterator iterator = um->Modifier.UnitStock.begin(); iterator != um->Modifier.UnitStock.end(); ++iterator) {
		const CUnitType *unit_type = iterator->first;
		const int unit_stock = iterator->second;
		if (unit_stock > 0) {
			unit.ChangeUnitStock(unit_type, - unit_stock);
		}
	}
	
	//Wyrmgus start
	//change variation if current one becomes forbidden
	const UnitTypeVariation *current_variation = unit.GetVariation();
	if (current_variation) {
		bool upgrade_required = false;
		for (const CUpgrade *required_upgrade : current_variation->UpgradesRequired) {
			if (um->UpgradeId == required_upgrade->GetIndex()) {
				upgrade_required = true;
				break;
			}
		}
		if (upgrade_required == true) {
			unit.ChooseVariation();
		}
	}
	for (int i = 0; i < MaxImageLayers; ++i) {
		const UnitTypeVariation *current_layer_variation = unit.GetLayerVariation(i);
		if (current_layer_variation) {
			bool upgrade_required = false;
			for (const CUpgrade *required_upgrade : current_layer_variation->UpgradesRequired) {
				if (um->UpgradeId == required_upgrade->GetIndex()) {
					upgrade_required = true;
					break;
				}
			}
			if (upgrade_required == true) {
				unit.ChooseVariation(nullptr, i);
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
	int id = upgrade->GetIndex();
	player.UpgradeTimers.Upgrades[id] = upgrade->Costs[TimeCost];
	AllowUpgradeId(player, id, 'R');  // research done

	//Wyrmgus start
	if (!strncmp(upgrade->Ident.c_str(), "upgrade-deity-", 14) && strncmp(upgrade->Ident.c_str(), "upgrade-deity-domain-", 21)) { // if is a deity upgrade, but isn't a deity domain upgrade
		CDeity *upgrade_deity = CDeity::GetByUpgrade(upgrade);
		if (upgrade_deity) {
			for (CDeityDomain *deity_domain : upgrade_deity->GetDomains()) {
				CUpgrade *domain_upgrade = deity_domain->Upgrade;
				if (player.Allow.Upgrades[domain_upgrade->GetIndex()] != 'R') {
					UpgradeAcquire(player, domain_upgrade);
				}
			}
			player.Deities.push_back(upgrade_deity);
		}
	}
	//Wyrmgus end
	
	for (size_t z = 0; z < upgrade->UpgradeModifiers.size(); ++z) {
		ApplyUpgradeModifier(player, upgrade->UpgradeModifiers[z]);
	}

	player.CheckAge();
	
	//
	//  Upgrades could change the buttons displayed.
	//
	if (&player == CPlayer::GetThisPlayer()) {
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
void UpgradeLost(CPlayer &player, const CUpgrade *upgrade)
{
	//Wyrmgus start
	if (!GameRunning && !GameEstablishing) {
		return;
	}
	//Wyrmgus end
	player.UpgradeTimers.Upgrades[upgrade->GetIndex()] = 0;
	//Wyrmgus start
	AllowUpgradeId(player, upgrade->GetIndex(), 'A'); // research is lost i.e. available
	//Wyrmgus end
	
	//Wyrmgus start
	if (!strncmp(upgrade->Ident.c_str(), "upgrade-deity-", 14) && strncmp(upgrade->Ident.c_str(), "upgrade-deity-domain-", 21)) { // if is a deity upgrade, but isn't a deity domain upgrade
		CDeity *upgrade_deity = CDeity::GetByUpgrade(upgrade);
		if (upgrade_deity) {
			for (CDeityDomain *deity_domain : upgrade_deity->GetDomains()) {
				CUpgrade *domain_upgrade = deity_domain->Upgrade;
				if (player.Allow.Upgrades[domain_upgrade->GetIndex()] == 'R') {
					UpgradeLost(player, domain_upgrade);
				}
			}
			player.Deities.erase(std::remove(player.Deities.begin(), player.Deities.end(), upgrade_deity), player.Deities.end());
		}
	}

	for (const CUpgradeModifier *modifier : upgrade->UpgradeModifiers) {
		RemoveUpgradeModifier(player, modifier);
	}
	//Wyrmgus end

	//
	//  Upgrades could change the buttons displayed.
	//
	if (&player == CPlayer::GetThisPlayer()) {
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
	for (const CUpgrade *upgrade : CUpgrade::GetAll()) {
		if (upgrade) {
			for (int p = 0; p < PlayerMax; ++p) {
				int id = upgrade->GetIndex();
				if (CPlayer::Players[p]->Allow.Upgrades[id] == 'R') {
					CPlayer::Players[p]->UpgradeTimers.Upgrades[id] = upgrade->Costs[TimeCost];
					AllowUpgradeId(*CPlayer::Players[p], id, 'R');  // research done

					for (size_t z = 0; z < upgrade->UpgradeModifiers.size(); ++z) {
						ApplyUpgradeModifier(*CPlayer::Players[p], upgrade->UpgradeModifiers[z]);
					}
				}
			}
		}
	}
	
	for (int p = 0; p < PlayerMax; ++p) {
		CPlayer::Players[p]->CheckAge();
	}
}

//Wyrmgus start
/**
**	@brief	Handle that an ability was acquired.
**
**	@param	unit	The unit that is learning the ability upgrade.
**	@param	upgrade	The ability upgrade being learned.
**	@param	save	Whether to save that the ability was learned, if the unit is a character.
*/
void AbilityAcquire(CUnit &unit, const CUpgrade *upgrade, const bool save)
{
	unit.Variable[LEVELUP_INDEX].Value -= 1;
	unit.Variable[LEVELUP_INDEX].Max = unit.Variable[LEVELUP_INDEX].Value;
	if (!IsNetworkGame() && unit.Character != nullptr && save) {
		if (unit.GetPlayer()->AiEnabled == false) { //save ability learning, if unit has a character and it is persistent, and the character doesn't have the ability yet
			unit.Character->Abilities.push_back(upgrade);
			SaveHero(unit.Character);
		}
	}
	IndividualUpgradeAcquire(unit, upgrade);
	unit.GetPlayer()->UpdateLevelUpUnits();
}

/**
**	@brief	Handle that an ability was lost.
**
**	@param	unit		The unit that is losing the ability upgrade.
**	@param	upgrade		The ability upgrade being lost.
**	@param	lose_all	Whether all of instances of the ability should be lost for the unit, rather than only one of them
*/
void AbilityLost(CUnit &unit, const CUpgrade *upgrade, const bool lose_all)
{
	unit.Variable[LEVELUP_INDEX].Value += 1;
	unit.Variable[LEVELUP_INDEX].Max = unit.Variable[LEVELUP_INDEX].Value;
	unit.Variable[LEVELUP_INDEX].Enable = 1;
	if (!IsNetworkGame() && unit.Character != nullptr) {
		if (std::find(unit.Character->Abilities.begin(), unit.Character->Abilities.end(), upgrade) != unit.Character->Abilities.end()) {
			if (unit.GetPlayer()->AiEnabled == false) { //save ability learning, if unit has a character and it is persistent, and the character doesn't have the ability yet
				unit.Character->Abilities.erase(std::remove(unit.Character->Abilities.begin(), unit.Character->Abilities.end(), upgrade), unit.Character->Abilities.end());
				SaveHero(unit.Character);
			}
		}
	}
	IndividualUpgradeLost(unit, upgrade);
	unit.GetPlayer()->UpdateLevelUpUnits();
	
	if (lose_all && unit.GetIndividualUpgrade(upgrade) > 0) {
		AbilityLost(unit, upgrade, lose_all);
	}
}

void TraitAcquire(CUnit &unit, const CUpgrade *upgrade)
{
	if (unit.Trait != nullptr) { //remove previous trait, if any
		if (!GameSettings.NoRandomness) { // if in no randomness setting, don't change trait modifiers
			IndividualUpgradeLost(unit, unit.Trait);
		}
	}

	unit.Trait = CUpgrade::Get(upgrade->Ident);

	if (!GameSettings.NoRandomness) { // if in no randomness setting, don't apply trait modifiers
		IndividualUpgradeAcquire(unit, upgrade);
	}
	
	unit.UpdateExtraName();

	//
	//  Upgrades could change the buttons displayed.
	//
	if (unit.GetPlayer() == CPlayer::GetThisPlayer()) {
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
	unit.SetIndividualUpgrade(upgrade, unit.GetIndividualUpgrade(upgrade) + 1);
	
	if (!strncmp(upgrade->Ident.c_str(), "upgrade-deity-", 14) && strncmp(upgrade->Ident.c_str(), "upgrade-deity-domain-", 21)) { // if is a deity upgrade, but isn't a deity domain upgrade
		CDeity *upgrade_deity = CDeity::GetByUpgrade(upgrade);
		if (upgrade_deity) {
			for (CDeityDomain *deity_domain : upgrade_deity->GetDomains()) {
				CUpgrade *domain_upgrade = deity_domain->Upgrade;
				if (!unit.GetIndividualUpgrade(domain_upgrade)) {
					IndividualUpgradeAcquire(unit, domain_upgrade);
				}
			}
			if (unit.Character && std::find(unit.Character->Deities.begin(), unit.Character->Deities.end(), upgrade_deity) == unit.Character->Deities.end()) {
				unit.Character->Deities.push_back(upgrade_deity);
				SaveHero(unit.Character);
			}
		}
	}

	//Wyrmgus start
	/*
	for (const CUpgradeModifier *modifier : CUpgradeModifier::UpgradeModifiers) {
		if (modifier->UpgradeId == id) {
			ApplyIndividualUpgradeModifier(unit, modifier);
		}
	}
	*/
	if (!(upgrade->IsAbility() && upgrade->WeaponClasses.size() > 0 && std::find(upgrade->WeaponClasses.begin(), upgrade->WeaponClasses.end(), unit.GetCurrentWeaponClass()) == upgrade->WeaponClasses.end())) {
		for (size_t z = 0; z < upgrade->UpgradeModifiers.size(); ++z) {
			bool applies_to_this = false;
			bool applies_to_any_unit_types = false;
			for (CUnitType *unit_type : CUnitType::GetAll()) {
				if (upgrade->UpgradeModifiers[z]->AppliesToUnitType(unit_type)) {
					applies_to_any_unit_types = true;
					if (unit_type->GetIndex() == unit.GetType()->GetIndex()) {
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
	if (unit.GetPlayer() == CPlayer::GetThisPlayer()) {
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
	unit.SetIndividualUpgrade(upgrade, unit.GetIndividualUpgrade(upgrade) - 1);

	if (!strncmp(upgrade->Ident.c_str(), "upgrade-deity-", 14) && strncmp(upgrade->Ident.c_str(), "upgrade-deity-domain-", 21)) { // if is a deity upgrade, but isn't a deity domain upgrade
		CDeity *upgrade_deity = CDeity::GetByUpgrade(upgrade);
		if (upgrade_deity) {
			for (CDeityDomain *deity_domain : upgrade_deity->GetDomains()) {
				CUpgrade *domain_upgrade = deity_domain->Upgrade;
				if (unit.GetIndividualUpgrade(domain_upgrade)) {
					IndividualUpgradeLost(unit, domain_upgrade);
				}
			}
			if (unit.Character) {
				unit.Character->Deities.erase(std::remove(unit.Character->Deities.begin(), unit.Character->Deities.end(), upgrade_deity), unit.Character->Deities.end());
				SaveHero(unit.Character);
			}
		}
	}

	//Wyrmgus start
	if (!(upgrade->IsAbility() && upgrade->WeaponClasses.size() > 0 && std::find(upgrade->WeaponClasses.begin(), upgrade->WeaponClasses.end(), unit.GetCurrentWeaponClass()) == upgrade->WeaponClasses.end())) {
		for (size_t z = 0; z < upgrade->UpgradeModifiers.size(); ++z) {
			bool applies_to_this = false;
			bool applies_to_any_unit_types = false;
			for (CUnitType *unit_type : CUnitType::GetAll()) {
				if (upgrade->UpgradeModifiers[z]->AppliesToUnitType(unit_type)) {
					applies_to_any_unit_types = true;
					if (unit_type->GetIndex() == unit.GetType()->GetIndex()) {
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
	if (unit.GetPlayer() == CPlayer::GetThisPlayer()) {
		SelectedUnitChanged();
	}
	
	if (lose_all && unit.GetIndividualUpgrade(upgrade) > 0) {
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
	const CUpgrade *upgrade = CUpgrade::Get(ident);

	if (upgrade != nullptr) {
		return UpgradeIdAllowed(player, upgrade->GetIndex());
	}
	
	fprintf(stderr, "Error in UpgradeIdentAllowed(), wrong identifier '%s'\n" _C_ ident.c_str());
	
	return '-';
}

//Wyrmgus start
std::string GetUpgradeEffectsString(const std::string &upgrade_ident)
{
	const CUpgrade *upgrade = CUpgrade::Get(upgrade_ident);
	
	std::string padding_string = ", ";

	if (upgrade) {
		std::string upgrade_effects_string;
		
		bool first_element = true;
		//check if the upgrade makes modifications to any units
		for (size_t z = 0; z < upgrade->UpgradeModifiers.size(); ++z) {
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

					upgrade_effects_string += GetVariableDisplayName(var).utf8().get_data();
						
					bool first_unit_type = true;
					for (CUnitType *unit_type : CUnitType::GetAll()) {
						if (upgrade->UpgradeModifiers[z]->AppliesToUnitType(unit_type)) {
							if (!first_unit_type) {
								upgrade_effects_string += ", ";
							} else {
								upgrade_effects_string += " for ";
								first_unit_type = false;
							}
									
							upgrade_effects_string += unit_type->GetNamePlural();
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
											
					upgrade_effects_string += GetVariableDisplayName(var, true).utf8().get_data();
				}
			}
				
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
					for (CUnitType *unit_type : CUnitType::GetAll()) {
						if (upgrade->UpgradeModifiers[z]->AppliesToUnitType(unit_type)) {
							if (!first_unit_type) {
								upgrade_effects_string += ", ";
							} else {
								upgrade_effects_string += " for ";
								first_unit_type = false;
							}
									
							upgrade_effects_string += unit_type->GetNamePlural();
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
	return (
		var == CHARGEBONUS_INDEX || var == BACKSTAB_INDEX
		|| var == BONUSAGAINSTMOUNTED_INDEX || var == BONUSAGAINSTBUILDINGS_INDEX || var == BONUSAGAINSTAIR_INDEX || var == BONUSAGAINSTGIANTS_INDEX || var == BONUSAGAINSTDRAGONS_INDEX
		|| var == FIRERESISTANCE_INDEX || var == COLDRESISTANCE_INDEX || var == ARCANERESISTANCE_INDEX || var == LIGHTNINGRESISTANCE_INDEX || var == AIRRESISTANCE_INDEX || var == EARTHRESISTANCE_INDEX || var == WATERRESISTANCE_INDEX || var == ACIDRESISTANCE_INDEX
		|| var == HACKRESISTANCE_INDEX || var == PIERCERESISTANCE_INDEX || var == BLUNTRESISTANCE_INDEX
		|| var == TIMEEFFICIENCYBONUS_INDEX || var == RESEARCHSPEEDBONUS_INDEX || var == TRADECOST_INDEX || var == SALVAGEFACTOR_INDEX || var == MUGGING_INDEX || var == RAIDING_INDEX
	);
}

bool IsBonusVariable(int var)
{
	return var == GATHERINGBONUS_INDEX || var == COPPERGATHERINGBONUS_INDEX || var == SILVERGATHERINGBONUS_INDEX || var == GOLDGATHERINGBONUS_INDEX || var == IRONGATHERINGBONUS_INDEX || var == MITHRILGATHERINGBONUS_INDEX || var == LUMBERGATHERINGBONUS_INDEX || var == STONEGATHERINGBONUS_INDEX || var == COALGATHERINGBONUS_INDEX || var == JEWELRYGATHERINGBONUS_INDEX || var == FURNITUREGATHERINGBONUS_INDEX || var == LEATHERGATHERINGBONUS_INDEX || var == GEMSGATHERINGBONUS_INDEX || var == SPEEDBONUS_INDEX || var == CHARGEBONUS_INDEX || var == BACKSTAB_INDEX || var == DAYSIGHTRANGEBONUS_INDEX || var == NIGHTSIGHTRANGEBONUS_INDEX || var == TIMEEFFICIENCYBONUS_INDEX || var == RESEARCHSPEEDBONUS_INDEX || var == GARRISONEDRANGEBONUS_INDEX;
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

String GetVariableDisplayName(const int var_index, const bool increase)
{
	String variable_name = UnitTypeVar.VariableNameLookup[var_index];

	variable_name = variable_name.replace("BasicDamage", "Damage");
	variable_name = variable_name.replace("DaySightRangeBonus", "DaySightBonus");
	variable_name = variable_name.replace("NightSightRangeBonus", "NightSightBonus");
	variable_name = variable_name.replace("SightRange", "Sight");
	variable_name = variable_name.replace("AttackRange", "Range");
	variable_name = variable_name.replace("HitPointBonus", "HitPoints");
	variable_name = variable_name.replace("Supply", "FoodSupply");
	variable_name = variable_name.replace("Demand", "FoodCost");
	variable_name = SeparateCapitalizedStringElements(variable_name);
	variable_name = variable_name.replace("Backstab", "Backstab Bonus");
	variable_name = variable_name.replace("Knowledge Magic", "Knowledge (Magic)");
	variable_name = variable_name.replace("Knowledge Warfare", "Knowledge (Warfare)");
	
	if (increase) {
		if (var_index == HP_INDEX || var_index == HITPOINTBONUS_INDEX) {
			variable_name = "Regeneration";
		} else if (var_index == GIVERESOURCE_INDEX) {
			variable_name = "Resource Replenishment";
		} else {
			variable_name += " per Cycle";
		}
	}

	return RTR(variable_name);
}
//Wyrmgus end
