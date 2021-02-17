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
//      (c) Copyright 1999-2021 by Vladi Belperchinov-Shabanski, Jimmy Salmon
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
#include "dynasty.h"
//Wyrmgus start
#include "editor.h"
//Wyrmgus end
#include "faction.h"
//Wyrmgus start
#include "game.h"
//Wyrmgus end
#include "gender.h"
//Wyrmgus start
#include "grand_strategy.h"
//Wyrmgus end
#include "iolib.h"
#include "item/item_class.h"
#include "magic_domain.h"
#include "map/map.h"
#include "map/tileset.h"
//Wyrmgus start
#include "network.h"
//Wyrmgus end
#include "player.h"
#include "religion/deity.h"
#include "script.h"
#include "script/condition/and_condition.h"
//Wyrmgus start
#include "settings.h"
#include "translate.h"
//Wyrmgus end
#include "unit/unit.h"
#include "unit/unit_find.h"
//Wyrmgus start
#include "ui/interface.h"
#include "ui/ui.h"
//Wyrmgus end
#include "unit/unit_class.h"
//Wyrmgus start
#include "unit/unit_manager.h"
//Wyrmgus end
#include "unit/unit_type.h"
#include "unit/unit_type_variation.h"
#include "upgrade/upgrade_class.h"
#include "upgrade/upgrade_modifier.h"
#include "util/util.h"
#include "util/vector_util.h"

//Wyrmgus start
//static void AllowUnitId(CPlayer &player, int id, int units);
//Wyrmgus end

CUnitStats::~CUnitStats()
{
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
	this->Variables = rhs.Variables;

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
				cost += this->Costs[i] * wyrmgus::resource::get_all()[i]->get_base_price() / 100;
			}
		}
	}
	
	return cost;
}

int CUnitStats::GetUnitStock(const wyrmgus::unit_type *unit_type) const
{
	if (!unit_type) {
		return 0;
	}

	auto find_iterator = this->UnitStock.find(unit_type->Slot);
	if (find_iterator != this->UnitStock.end()) {
		return find_iterator->second;
	} else {
		return 0;
	}
}

void CUnitStats::SetUnitStock(const wyrmgus::unit_type *unit_type, int quantity)
{
	if (!unit_type) {
		return;
	}
	
	if (quantity <= 0) {
		if (this->UnitStock.contains(unit_type->Slot)) {
			this->UnitStock.erase(unit_type->Slot);
		}
	} else {
		this->UnitStock[unit_type->Slot] = quantity;
	}
}

void CUnitStats::ChangeUnitStock(const wyrmgus::unit_type *unit_type, int quantity)
{
	this->SetUnitStock(unit_type, this->GetUnitStock(unit_type) + quantity);
}

wyrmgus::gender CUnitStats::get_gender() const
{
	return static_cast<wyrmgus::gender>(this->Variables[GENDER_INDEX].Value);
}

CUpgrade::CUpgrade(const std::string &identifier) : detailed_data_entry(identifier), Work(wyrmgus::item_class::none)
{
	memset(this->Costs, 0, sizeof(this->Costs));
	//Wyrmgus start
	memset(this->GrandStrategyProductionEfficiencyModifier, 0, sizeof(this->GrandStrategyProductionEfficiencyModifier));
	memset(this->IncompatibleAffixes, 0, sizeof(this->IncompatibleAffixes));
	//Wyrmgus end
}

CUpgrade::~CUpgrade()
{
}

void CUpgrade::process_sml_property(const wyrmgus::sml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "parent") {
		CUpgrade *parent_upgrade = CUpgrade::get(value);
		this->set_parent(parent_upgrade);
	} else if (key == "button_key") {
		this->button_key = value;
	} else {
		data_entry::process_sml_property(property);
	}
}

void CUpgrade::process_sml_scope(const wyrmgus::sml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "costs") {
		scope.for_each_property([&](const wyrmgus::sml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			const wyrmgus::resource *resource = wyrmgus::resource::get(key);
			this->Costs[resource->get_index()] = std::stoi(value);
		});
	} else if (tag == "scaled_costs") {
		scope.for_each_property([&](const wyrmgus::sml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			const wyrmgus::resource *resource = wyrmgus::resource::get(key);
			this->scaled_costs[resource] = std::stoi(value);
		});
	} else if (tag == "scaled_cost_unit_types") {
		for (const std::string &value : values) {
			this->scaled_cost_unit_types.push_back(wyrmgus::unit_type::get(value));
		}
	} else if (tag == "scaled_cost_unit_classes") {
		for (const std::string &value : values) {
			this->scaled_cost_unit_classes.push_back(wyrmgus::unit_class::get(value));
		}
	} else if (tag == "civilization_priorities") {
		scope.for_each_property([&](const wyrmgus::sml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			wyrmgus::civilization *priority_civilization = wyrmgus::civilization::get(key);
			const int priority = std::stoi(value);
			priority_civilization->UpgradePriorities[this] = priority;
		});
	} else if (tag == "modifier") {
		auto modifier = std::make_unique<wyrmgus::upgrade_modifier>();
		modifier->UpgradeId = this->ID;

		wyrmgus::database::process_sml_data(modifier, scope);

		wyrmgus::upgrade_modifier::UpgradeModifiers.push_back(modifier.get());
		this->modifiers.push_back(std::move(modifier));
	} else if (tag == "preconditions") {
		this->preconditions = std::make_unique<wyrmgus::and_condition>();
		wyrmgus::database::process_sml_data(this->preconditions, scope);
	} else if (tag == "conditions") {
		this->conditions = std::make_unique<wyrmgus::and_condition>();
		wyrmgus::database::process_sml_data(this->conditions, scope);
	} else if (tag == "affixed_item_classes") {
		for (const std::string &value : values) {
			this->affixed_item_classes.insert(wyrmgus::string_to_item_class(value));
		}
	}
}

void CUpgrade::initialize()
{
	if (this->get_icon() == nullptr) {
		if (this->get_dynasty() != nullptr) {
			this->icon = this->get_dynasty()->get_icon();
		} else if (this->get_deity() != nullptr) {
			this->icon = this->get_deity()->get_icon();
		} else if (this->magic_domain != nullptr) {
			if (this->get_name().empty()) {
				this->set_name(this->magic_domain->get_name());
			}

			if (this->get_description().empty()) {
				this->set_description(this->magic_domain->get_description());
			}

			if (this->get_background().empty()) {
				this->set_background(this->magic_domain->get_background());
			}

			if (this->get_quote().empty()) {
				this->set_quote(this->magic_domain->get_quote());
			}
		}
	}

	for (CUpgrade *other_upgrade : CUpgrade::get_all()) { //add the upgrade to the incompatible affix's counterpart list here
		if (this->IncompatibleAffixes[other_upgrade->ID]) {
			other_upgrade->IncompatibleAffixes[this->ID] = true;
		}
	}

	if (this->get_upgrade_class() != nullptr) { //if class is defined, then use this upgrade to help build the classes table, and add this upgrade to the civilization class table (if the civilization is defined)
		const wyrmgus::upgrade_class *upgrade_class = this->get_upgrade_class();
		if (this->get_faction() != nullptr) {
			this->get_faction()->set_class_upgrade(upgrade_class, this);
		} else if (this->get_civilization() != nullptr) {
			this->get_civilization()->set_class_upgrade(upgrade_class, this);
		}
	}

	CclCommand("if not (GetArrayIncludes(Units, \"" + this->get_identifier() + "\")) then table.insert(Units, \"" + this->get_identifier() + "\") end"); //FIXME: needed at present to make upgrade data files work without scripting being necessary, but it isn't optimal to interact with a scripting table like "Units" in this manner (that table should probably be replaced with getting a list of unit types from the engine)

	data_entry::initialize();
}

void CUpgrade::check() const
{
	if (this->get_preconditions() != nullptr) {
		this->get_preconditions()->check_validity();
	}

	if (this->get_conditions() != nullptr) {
		this->get_conditions()->check_validity();
	}
}

void CUpgrade::set_parent(const CUpgrade *parent_upgrade)
{
	if (!parent_upgrade->is_defined()) {
		throw std::runtime_error("Upgrade \"" + this->get_identifier() + "\" is inheriting features from non-defined parent \"" + parent_upgrade->get_identifier() + "\".");
	}

	if (this->get_name().empty()) {
		this->set_name(parent_upgrade->get_name());
	}
	this->icon = parent_upgrade->get_icon();
	this->upgrade_class = parent_upgrade->get_upgrade_class();
	this->set_description(parent_upgrade->get_description());
	this->set_quote(parent_upgrade->get_quote());
	this->set_background(parent_upgrade->get_background());
	this->effects_string = parent_upgrade->get_effects_string();
	this->requirements_string = parent_upgrade->get_requirements_string();
	for (int i = 0; i < MaxCosts; ++i) {
		this->Costs[i] = parent_upgrade->Costs[i];
		this->GrandStrategyProductionEfficiencyModifier[i] = parent_upgrade->GrandStrategyProductionEfficiencyModifier[i];
	}
	this->scaled_costs = parent_upgrade->scaled_costs;
	this->affixed_item_classes = parent_upgrade->affixed_item_classes;
	this->MaxLimit = parent_upgrade->MaxLimit;
	this->magic_level = parent_upgrade->magic_level;
	this->ability = parent_upgrade->is_ability();
	this->weapon = parent_upgrade->is_weapon();
	this->shield = parent_upgrade->is_shield();
	this->boots = parent_upgrade->is_boots();
	this->arrows = parent_upgrade->is_arrows();
	this->item = parent_upgrade->get_item();
	this->magic_prefix = parent_upgrade->magic_prefix;
	this->magic_suffix = parent_upgrade->magic_suffix;
	this->RunicAffix = parent_upgrade->RunicAffix;
	this->Work = parent_upgrade->Work;
	this->UniqueOnly = parent_upgrade->UniqueOnly;
	this->scaled_cost_unit_types = parent_upgrade->scaled_cost_unit_types;
	this->scaled_cost_unit_classes = parent_upgrade->scaled_cost_unit_classes;

	for (const auto &modifier : parent_upgrade->get_modifiers()) {
		std::unique_ptr<wyrmgus::upgrade_modifier> duplicated_modifier = modifier->duplicate();
		duplicated_modifier->UpgradeId = this->ID;
		wyrmgus::upgrade_modifier::UpgradeModifiers.push_back(duplicated_modifier.get());
		this->modifiers.push_back(std::move(duplicated_modifier));
	}
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
void CleanUpgradeModifiers()
{
	wyrmgus::upgrade_modifier::UpgradeModifiers.clear();
}

void CUpgrade::set_upgrade_class(wyrmgus::upgrade_class *upgrade_class)
{
	if (upgrade_class == this->get_upgrade_class()) {
		return;
	}

	if (this->get_upgrade_class() != nullptr) {
		this->get_upgrade_class()->remove_upgrade(this);
	}

	this->upgrade_class = upgrade_class;

	if (this->get_upgrade_class() != nullptr && !this->get_upgrade_class()->has_upgrade(this)) {
		this->get_upgrade_class()->add_upgrade(this);
	}
}

void CUpgrade::add_modifier(std::unique_ptr<const wyrmgus::upgrade_modifier> &&modifier)
{
	this->modifiers.push_back(std::move(modifier));
}

/**
**  Save state of the upgrade to file.
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
	for (const wyrmgus::unit_type *unit_type : wyrmgus::unit_type::get_all()) {
		file.printf("DefineUnitAllow(\"%s\", ", unit_type->Ident.c_str());
		for (int p = 0; p < PlayerMax; ++p) {
			if (p) {
				file.printf(", ");
			}
			file.printf("%d", CPlayer::Players[p]->Allow.Units[unit_type->Slot]);
		}
		file.printf(")\n");
	}
	file.printf("\n");

	//
	//  Save the upgrades
	//
	for (const CUpgrade *upgrade : CUpgrade::get_all()) {
		file.printf("DefineAllow(\"%s\", \"", upgrade->get_identifier().c_str());
		for (int p = 0; p < PlayerMax; ++p) {
			file.printf("%c", CPlayer::Players[p]->Allow.Upgrades[upgrade->ID]);
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
	CUpgrade *upgrade = CUpgrade::get_or_add(upgrade_ident, nullptr);
	
	//  Parse the list:
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);
		
		if (!strcmp(value, "Parent")) {
			CUpgrade *parent_upgrade = CUpgrade::get(LuaToString(l, -1));
			upgrade->set_parent(parent_upgrade);
		} else if (!strcmp(value, "Name")) {
			upgrade->set_name(LuaToString(l, -1));
		} else if (!strcmp(value, "Icon")) {
			wyrmgus::icon *icon = wyrmgus::icon::get(LuaToString(l, -1));
			upgrade->icon = icon;
		} else if (!strcmp(value, "Class")) {
			upgrade->set_upgrade_class(wyrmgus::upgrade_class::get(LuaToString(l, -1)));
		} else if (!strcmp(value, "Civilization")) {
			std::string civilization_name = LuaToString(l, -1);
			wyrmgus::civilization *civilization = wyrmgus::civilization::get(civilization_name);
			upgrade->civilization = civilization;
		} else if (!strcmp(value, "Faction")) {
			std::string faction_name = LuaToString(l, -1);
			wyrmgus::faction *faction = wyrmgus::faction::get(faction_name);
			upgrade->faction = faction;
		} else if (!strcmp(value, "Description")) {
			upgrade->set_description(LuaToString(l, -1));
		} else if (!strcmp(value, "Quote")) {
			upgrade->set_quote(LuaToString(l, -1));
		} else if (!strcmp(value, "Background")) {
			upgrade->set_background(LuaToString(l, -1));
		} else if (!strcmp(value, "EffectsString")) {
			upgrade->effects_string = LuaToString(l, -1);
		} else if (!strcmp(value, "RequirementsString")) {
			upgrade->requirements_string = LuaToString(l, -1);
		} else if (!strcmp(value, "MaxLimit")) {
			upgrade->MaxLimit = LuaToNumber(l, -1);
		} else if (!strcmp(value, "MagicLevel")) {
			upgrade->magic_level = LuaToNumber(l, -1);
		} else if (!strcmp(value, "Year")) {
			upgrade->Year = LuaToNumber(l, -1);
		} else if (!strcmp(value, "Ability")) {
			upgrade->ability = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Weapon")) {
			upgrade->weapon = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Shield")) {
			upgrade->shield = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Boots")) {
			upgrade->boots = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Arrows")) {
			upgrade->arrows = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "MagicPrefix")) {
			upgrade->magic_prefix = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "MagicSuffix")) {
			upgrade->magic_suffix = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "RunicAffix")) {
			upgrade->RunicAffix = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "UniqueOnly")) {
			upgrade->UniqueOnly = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Work")) {
			upgrade->Work = wyrmgus::string_to_item_class(LuaToString(l, -1));
		} else if (!strcmp(value, "Item")) {
			wyrmgus::unit_type *item = wyrmgus::unit_type::get(LuaToString(l, -1));
			upgrade->item = item;
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
				const wyrmgus::resource *resource = wyrmgus::resource::get(LuaToString(l, -1, j + 1));
				++j;
				
				upgrade->scaled_costs[resource] = LuaToNumber(l, -1, j + 1);
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
				wyrmgus::civilization *priority_civilization = wyrmgus::civilization::get(civilization_ident);
				++j;

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
				wyrmgus::faction *priority_faction = wyrmgus::faction::get(faction_ident);
				++j;
				
				int priority = LuaToNumber(l, -1, j + 1);

				priority_faction->UpgradePriorities[upgrade] = priority;
			}
		} else if (!strcmp(value, "ItemAffix")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				const wyrmgus::item_class item_class = wyrmgus::string_to_item_class(LuaToString(l, -1, j + 1));
				
				upgrade->affixed_item_classes.insert(item_class);
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
				const wyrmgus::unit_type *scaled_cost_unit = wyrmgus::unit_type::get(LuaToString(l, -1, j + 1));
				upgrade->scaled_cost_unit_types.push_back(scaled_cost_unit);
			}
		} else if (!strcmp(value, "WeaponClasses")) {
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument (expected table)");
			}
			const int subargs = lua_rawlen(l, -1);
			for (int j = 0; j < subargs; ++j) {
				upgrade->WeaponClasses.insert(wyrmgus::string_to_item_class(LuaToString(l, -1, j + 1)));
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
	
	upgrade->set_defined(true);
	
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

	auto um = std::make_unique<wyrmgus::upgrade_modifier>();

	std::string upgrade_ident = LuaToString(l, 1);
	um->UpgradeId = UpgradeIdByIdent(upgrade_ident);
	if (um->UpgradeId == -1) {
		LuaError(l, "Error when defining upgrade modifier: upgrade \"%s\" doesn't exist." _C_ upgrade_ident.c_str());
	}
	
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
			wyrmgus::unit_type *unit_type = wyrmgus::unit_type::get(value);
			um->Modifier.SetUnitStock(unit_type, LuaToNumber(l, j + 1, 3));
		//Wyrmgus end
		} else if (!strcmp(key, "allow-unit")) {
			const char *value = LuaToString(l, j + 1, 2);

			if (!strncmp(value, "unit", 4)) {
				um->ChangeUnits[UnitTypeIdByIdent(value)] = LuaToNumber(l, j + 1, 3);
			} else {
				LuaError(l, "unit expected");
			}
		} else if (!strcmp(key, "allow")) {
			const char *value = LuaToString(l, j + 1, 2);
			//Wyrmgus start
//			um->ChangeUpgrades[UpgradeIdByIdent(value)] = LuaToNumber(l, j + 1, 3);
			um->ChangeUpgrades[CUpgrade::get(value)->ID] = *strdup(LuaToString(l, j + 1, 3));
			//Wyrmgus end
		//Wyrmgus start
		} else if (!strcmp(key, "remove-upgrade")) {
			const char *value = LuaToString(l, j + 1, 2);
			CUpgrade *removed_upgrade = CUpgrade::get(value);
			um->RemoveUpgrades.push_back(removed_upgrade);
		//Wyrmgus end
		} else if (!strcmp(key, "apply-to")) {
			const char *value = LuaToString(l, j + 1, 2);
			um->unit_types.push_back(wyrmgus::unit_type::get(value));
		} else if (!strcmp(key, "convert-to")) {
			const char *value = LuaToString(l, j + 1, 2);
			um->ConvertTo = wyrmgus::unit_type::get(value);
		//Wyrmgus start
		} else if (!strcmp(key, "research-speed")) {
			um->SpeedResearch = LuaToNumber(l, j + 1, 2);
		} else if (!strcmp(key, "change-civilization-to")) {
			const char *civilization_ident = LuaToString(l, j + 1, 2);
			const wyrmgus::civilization *civilization = wyrmgus::civilization::get(civilization_ident);
			um->change_civilization_to = civilization;
		} else if (!strcmp(key, "change-faction-to")) {
			const std::string faction_ident = LuaToString(l, j + 1, 2);
			um->change_faction_to = wyrmgus::faction::get(faction_ident);
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
						DefineVariableField(l, um->Modifier.Variables[index], -1);
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

	wyrmgus::upgrade_modifier::UpgradeModifiers.push_back(um.get());
	CUpgrade::get_all()[um->UpgradeId]->add_modifier(std::move(um));

	return 0;
}

/**
**  Define which units are allowed and how much.
*/
static int CclDefineUnitAllow(lua_State *l)
{
	const int args = lua_gettop(l);

	const char *ident = LuaToString(l, 0 + 1);

	if (strncmp(ident, "unit", 4)) {
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

		if (!strncmp(ident, "unit", 4)) {
			int id = UnitTypeIdByIdent(ident);
			for (int i = 0; i < n; ++i) {
				if (ids[i] == 'A') {
					AllowUnitId(*CPlayer::Players[i], id, UnitMax);
				} else if (ids[i] == 'F') {
					AllowUnitId(*CPlayer::Players[i], id, 0);
				}
			}
		} else if (!strncmp(ident, "upgrade", 7)) {
			int id = UpgradeIdByIdent(ident);
			for (int i = 0; i < n; ++i) {
				AllowUpgradeId(*CPlayer::Players[i], id, ids[i]);
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
	CUnit *unit = &wyrmgus::unit_manager::get()->GetSlotUnit(LuaToNumber(l, -1));
	lua_pop(l, 1);
	const char *ident = LuaToString(l, 2);
	AbilityAcquire(*unit, CUpgrade::get(ident));
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
	CUnit *unit = &wyrmgus::unit_manager::get()->GetSlotUnit(LuaToNumber(l, -1));
	lua_pop(l, 1);
	const std::string ident = LuaToString(l, 2);
	if (!ident.empty()) {
		TraitAcquire(*unit, CUpgrade::get(ident));
	} else {
		if (unit->Trait != nullptr) { //remove previous trait, if any
			if (!GameSettings.NoRandomness) { // if in no randomness setting, don't apply trait modifiers
				IndividualUpgradeLost(*unit, unit->Trait);
			}
		}
		unit->Trait = nullptr;
	}
	return 0;
}

static int CclGetUpgrades(lua_State *l)
{
	lua_createtable(l, CUpgrade::get_all().size(), 0);
	for (size_t i = 1; i <= CUpgrade::get_all().size(); ++i)
	{
		lua_pushstring(l, CUpgrade::get_all()[i-1]->get_identifier().c_str());
		lua_rawseti(l, -2, i);
	}
	return 1;
}

static int CclGetItemPrefixes(lua_State *l)
{
	std::vector<const CUpgrade *> item_prefixes;
	for (const CUpgrade *upgrade : CUpgrade::get_all()) {
		if (upgrade->is_magic_prefix()) {
			item_prefixes.push_back(upgrade);
		}
	}
		
	lua_createtable(l, item_prefixes.size(), 0);
	for (size_t i = 1; i <= item_prefixes.size(); ++i)
	{
		lua_pushstring(l, item_prefixes[i-1]->get_identifier().c_str());
		lua_rawseti(l, -2, i);
	}
	return 1;
}

static int CclGetItemSuffixes(lua_State *l)
{
	std::vector<const CUpgrade *> item_suffixes;
	for (const CUpgrade *upgrade : CUpgrade::get_all()) {
		if (upgrade->is_magic_suffix() && !upgrade->RunicAffix) {
			item_suffixes.push_back(upgrade);
		}
	}
		
	lua_createtable(l, item_suffixes.size(), 0);
	for (size_t i = 1; i <= item_suffixes.size(); ++i)
	{
		lua_pushstring(l, item_suffixes[i-1]->get_identifier().c_str());
		lua_rawseti(l, -2, i);
	}
	return 1;
}

static int CclGetRunicSuffixes(lua_State *l)
{
	std::vector<const CUpgrade *> item_suffixes;
	for (const CUpgrade *upgrade : CUpgrade::get_all()) {
		if (upgrade->is_magic_suffix() && upgrade->RunicAffix) {
			item_suffixes.push_back(upgrade);
		}
	}
		
	lua_createtable(l, item_suffixes.size(), 0);
	for (size_t i = 1; i <= item_suffixes.size(); ++i)
	{
		lua_pushstring(l, item_suffixes[i-1]->get_identifier().c_str());
		lua_rawseti(l, -2, i);
	}
	return 1;
}

static int CclGetLiteraryWorks(lua_State *l)
{
	std::vector<const CUpgrade *> literary_works;
	for (const CUpgrade *upgrade : CUpgrade::get_all()) {
		if (upgrade->Work != wyrmgus::item_class::none) {
			literary_works.push_back(upgrade);
		}
	}
		
	lua_createtable(l, literary_works.size(), 0);
	for (size_t i = 1; i <= literary_works.size(); ++i)
	{
		lua_pushstring(l, literary_works[i-1]->get_identifier().c_str());
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
	const CUpgrade *upgrade = CUpgrade::get(upgrade_ident);
	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Name")) {
		lua_pushstring(l, upgrade->get_name().c_str());
		return 1;
	} else if (!strcmp(data, "Class")) {
		if (upgrade->get_upgrade_class() != nullptr) {
			lua_pushstring(l, upgrade->get_upgrade_class()->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Civilization")) {
		if (upgrade->get_civilization() != nullptr) {
			lua_pushstring(l, upgrade->get_civilization()->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Faction")) {
		if (upgrade->get_faction() != nullptr) {
			lua_pushstring(l, upgrade->get_faction()->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Icon")) {
		if (upgrade->get_icon()) {
			lua_pushstring(l, upgrade->get_icon()->get_identifier().c_str());
		} else {
			lua_pushstring(l, "");
		}
		return 1;
	} else if (!strcmp(data, "Description")) {
		lua_pushstring(l, upgrade->get_description().c_str());
		return 1;
	} else if (!strcmp(data, "Background")) {
		lua_pushstring(l, upgrade->get_background().c_str());
		return 1;
	} else if (!strcmp(data, "Quote")) {
		lua_pushstring(l, upgrade->get_quote().c_str());
		return 1;
	} else if (!strcmp(data, "EffectsString")) {
		lua_pushstring(l, upgrade->get_effects_string().c_str());
		return 1;
	} else if (!strcmp(data, "RequirementsString")) {
		lua_pushstring(l, upgrade->get_requirements_string().c_str());
		return 1;
	} else if (!strcmp(data, "Ability")) {
		lua_pushboolean(l, upgrade->is_ability());
		return 1;
	} else if (!strcmp(data, "ItemPrefix")) {
		if (nargs == 2) { //check if the upgrade is a prefix for any item type
			if (upgrade->is_magic_prefix()) {
				lua_pushboolean(l, true);
				return 1;
			} else {
				lua_pushboolean(l, false);
				return 1;
			}
		} else {
			LuaCheckArgs(l, 3);
			const std::string item_class_name = LuaToString(l, 3);
			const wyrmgus::item_class item_class = wyrmgus::string_to_item_class(item_class_name);
			lua_pushboolean(l, upgrade->is_magic_prefix() && upgrade->has_affixed_item_class(item_class));
			return 1;
		}
	} else if (!strcmp(data, "ItemSuffix")) {
		if (nargs == 2) { //check if the item is a suffix for any item type
			if (upgrade->is_magic_suffix()) {
				lua_pushboolean(l, true);
				return 1;
			} else {
				lua_pushboolean(l, false);
				return 1;
			}
		} else {
			LuaCheckArgs(l, 3);
			const std::string item_class_name = LuaToString(l, 3);
			const wyrmgus::item_class item_class = wyrmgus::string_to_item_class(item_class_name);
			lua_pushboolean(l, upgrade->is_magic_suffix() && upgrade->has_affixed_item_class(item_class));
			return 1;
		}
	} else if (!strcmp(data, "AppliesTo")) { //to which unit types or item classes this upgrade applies
		std::vector<std::string> applies_to;
		for (const wyrmgus::item_class item_class : upgrade->get_affixed_item_classes()) {
			applies_to.push_back(wyrmgus::item_class_to_string(item_class));
		}

		for (const auto &upgrade_modifier : upgrade->get_modifiers()) {
			for (const wyrmgus::unit_type *unit_type : upgrade_modifier->get_unit_types()) {
				applies_to.push_back(unit_type->get_identifier());
			}
		}

		for (const wyrmgus::unit_type *unit_type : wyrmgus::unit_type::get_all()) {
			if (unit_type->is_template()) { //if is a template, continue
				continue;
			}

			if (wyrmgus::vector::contains(unit_type->Affixes, upgrade)) {
				applies_to.push_back(unit_type->get_identifier());
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
	const wyrmgus::unit_type *type = wyrmgus::unit_type::try_get(ident);

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
	const CUpgrade *upgrade = CUpgrade::try_get(ident);

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
static void ConvertUnitTypeTo(CPlayer &player, const wyrmgus::unit_type &src, wyrmgus::unit_type &dst)
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
		if (&unit == nullptr) {
			fprintf(stderr, "Error in ConvertUnitTypeTo: unit %d, of player %d is null.\n", i, player.Index);
			continue;
		}
		//Wyrmgus end

		//  Convert already existing units to this type.
		//Wyrmgus start
//		if (unit.Type == &src) {
		if (unit.Type == &src && unit.get_character() == nullptr) { //don't do this for persistent characters
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
			if (unit.CriticalOrder != nullptr && unit.CriticalOrder->Action == UnitAction::TransformInto) {
				COrder_TransformInto *order = static_cast<COrder_TransformInto *>(unit.CriticalOrder.get());

				if (&order->GetUnitType() == &src) {
					order->ConvertUnitType(unit, dst);
				}
			}
			//Wyrmgus end
			
			for (size_t j = 0; j < unit.Orders.size(); ++j) {
				if (unit.Orders[j]->Action == UnitAction::Train) {
					COrder_Train &order = *static_cast<COrder_Train *>(unit.Orders[j].get());

					if (&order.GetUnitType() == &src) {
						order.ConvertUnitType(unit, dst);
					}
				//Wyrmgus start
				// convert building orders as well
				} else if (unit.Orders[j]->Action == UnitAction::Build) {
					COrder_Build &order = *static_cast<COrder_Build *>(unit.Orders[j].get());

					if (&order.GetUnitType() == &src) {
						order.ConvertUnitType(unit, dst);
					}
				// also convert upgrade orders
				} else if (unit.Orders[j]->Action == UnitAction::UpgradeTo) {
					COrder_UpgradeTo &order = *static_cast<COrder_UpgradeTo *>(unit.Orders[j].get());

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
static void ApplyUpgradeModifier(CPlayer &player, const wyrmgus::upgrade_modifier *um)
{
	Assert(um);

	int pn = player.Index;

	//Wyrmgus start
	if (um->SpeedResearch != 0) {
		player.SpeedResearch += um->SpeedResearch;
	}
	if (um->change_civilization_to != nullptr && GameRunning && um->change_civilization_to != player.get_civilization()) {
		player.set_civilization(um->change_civilization_to);
	}
	if (um->change_faction_to != nullptr && GameRunning && (um->change_faction_to->get_civilization() != player.get_civilization() || um->change_faction_to != player.get_faction())) {
		if (um->change_faction_to->get_civilization() != player.get_civilization()) {
			player.set_civilization(um->change_faction_to->get_civilization());
		}
		player.SetFaction(um->change_faction_to);
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

	for (wyrmgus::unit_type *unit_type : wyrmgus::unit_type::get_all()) {
		if (unit_type->is_template()) {
			continue;
		}

		CUnitStats &stat = unit_type->Stats[pn];
		// add/remove allowed units

		//Wyrmgus start
		if (stat.Variables.empty()) { // unit type's stats not initialized
			break;
		}
		//Wyrmgus end

		// FIXME: check if modify is allowed

		player.Allow.Units[unit_type->Slot] += um->ChangeUnits[unit_type->Slot];

		// this modifier should be applied to unittype id == z
		if (um->applies_to(unit_type)) {

			// if a unit type's supply is changed, we need to update the player's supply accordingly
			if (um->Modifier.Variables[SUPPLY_INDEX].Value) {
				std::vector<CUnit *> unitupgrade;

				FindUnitsByType(*unit_type, unitupgrade);
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

				FindUnitsByType(*unit_type, unitupgrade);
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
						stat.ImproveIncomes[j] += wyrmgus::resource::get_all()[j]->get_default_income() + um->Modifier.ImproveIncomes[j];
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

				stat.ResourceDemand[j] += um->Modifier.ResourceDemand[j];
			}
			
			for (const auto &kv_pair : um->Modifier.UnitStock) {
				const wyrmgus::unit_type *stock_unit_type = wyrmgus::unit_type::get_all()[kv_pair.first];
				const int unit_stock = kv_pair.second;
				if (unit_stock != 0) {
					stat.ChangeUnitStock(stock_unit_type, unit_stock);
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

//				FindUnitsByType(*UnitTypes[z], unitupgrade, true);
				//Wyrmgus end
				for (CUnit *unit : unitupgrade) {
					if (unit->Player->Index != player.Index) {
						continue;
					}
					
					//Wyrmgus start
					if (
						(CUpgrade::get_all()[um->UpgradeId]->is_weapon() && unit->EquippedItems[static_cast<int>(wyrmgus::item_slot::weapon)].size() > 0)
						|| (CUpgrade::get_all()[um->UpgradeId]->is_shield() && unit->EquippedItems[static_cast<int>(wyrmgus::item_slot::shield)].size() > 0)
						|| (CUpgrade::get_all()[um->UpgradeId]->is_boots() && unit->EquippedItems[static_cast<int>(wyrmgus::item_slot::boots)].size() > 0)
						|| (CUpgrade::get_all()[um->UpgradeId]->is_arrows() && unit->EquippedItems[static_cast<int>(wyrmgus::item_slot::arrows)].size() > 0)
					) { //if the unit already has an item equipped of the same equipment type as this upgrade, don't apply the modifier to it
						continue;
					}
					
					if (unit->get_character() != nullptr && CUpgrade::get_all()[um->UpgradeId]->get_deity() != nullptr) {
						//heroes choose their own deities
						continue;
					}
					//Wyrmgus end
					
					for (unsigned int j = 0; j < UnitTypeVar.GetNumberVariable(); j++) {
						unit->Variable[j].Enable |= um->Modifier.Variables[j].Enable;
						if (um->ModifyPercent[j]) {
							if (j != MANA_INDEX || um->ModifyPercent[j] < 0) {
								unit->Variable[j].Value += unit->Variable[j].Value * um->ModifyPercent[j] / 100;
							}
							unit->Variable[j].Max += unit->Variable[j].Max * um->ModifyPercent[j] / 100;
						} else {
							if (j != MANA_INDEX || um->Modifier.Variables[j].Value < 0) {
								unit->Variable[j].Value += um->Modifier.Variables[j].Value;
							}
							unit->Variable[j].Increase += um->Modifier.Variables[j].Increase;
						}

						unit->Variable[j].Max += um->Modifier.Variables[j].Max;
						unit->Variable[j].Max = std::max(unit->Variable[j].Max, 0);
						if (unit->Variable[j].Max > 0) {
							unit->Variable[j].Value = std::clamp(unit->Variable[j].Value, 0, unit->Variable[j].Max);
						}
						//Wyrmgus start
						if (j == ATTACKRANGE_INDEX && unit->Container) {
							unit->Container->UpdateContainerAttackRange();
						} else if (j == LEVEL_INDEX || j == POINTS_INDEX) {
							unit->UpdateXPRequired();
						} else if (IsKnowledgeVariable(j)) {
							unit->CheckKnowledgeChange(j, um->Modifier.Variables[j].Value);
						} else if ((j == SIGHTRANGE_INDEX || j == DAYSIGHTRANGEBONUS_INDEX || j == NIGHTSIGHTRANGEBONUS_INDEX) && !unit->Removed) {
							// If Sight range is upgraded, we need to change EVERY unit
							// to the new range, otherwise the counters get confused.
							MapUnmarkUnitSight(*unit);
							UpdateUnitSightRange(*unit);
							MapMarkUnitSight(*unit);
						}
						//Wyrmgus end
					}
					
					for (const auto &kv_pair : um->Modifier.UnitStock) {
						const wyrmgus::unit_type *stock_unit_type = wyrmgus::unit_type::get_all()[kv_pair.first];
						const int unit_stock = kv_pair.second;
						if (unit_stock < 0) {
							unit->ChangeUnitStock(stock_unit_type, unit_stock);
						}
					}
				}
			}
			
			//Wyrmgus start
			for (size_t j = 0; j != unitupgrade.size(); ++j) {
				CUnit &unit = *unitupgrade[j];

				if (unit.Player->Index != player.Index) {
					continue;
				}
				
				//add or remove starting abilities from the unit if the upgrade enabled/disabled them
				for (const CUpgrade *ability_upgrade : unit.Type->StartingAbilities) {
					if (!unit.GetIndividualUpgrade(ability_upgrade) && check_conditions(ability_upgrade, &unit)) {
						IndividualUpgradeAcquire(unit, ability_upgrade);
					} else if (unit.GetIndividualUpgrade(ability_upgrade) && !check_conditions(ability_upgrade, &unit)) {
						IndividualUpgradeLost(unit, ability_upgrade);
					}
				}
				
				//change variation if current one becomes forbidden
				const wyrmgus::unit_type_variation *current_variation = unit.GetVariation();
				if (current_variation != nullptr) {
					if (!unit.can_have_variation(current_variation)) {
						unit.ChooseVariation();
					}
				}
				for (int i = 0; i < MaxImageLayers; ++i) {
					const wyrmgus::unit_type_variation *current_layer_variation = unit.GetLayerVariation(i);
					if (current_layer_variation != nullptr) {
						if (!unit.can_have_variation(current_layer_variation)) {
							unit.ChooseVariation(nullptr, false, i);
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
static void RemoveUpgradeModifier(CPlayer &player, const wyrmgus::upgrade_modifier *um)
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

	for (wyrmgus::unit_type *unit_type : wyrmgus::unit_type::get_all()) {
		if (unit_type->is_template()) {
			continue;
		}

		CUnitStats &stat = unit_type->Stats[pn];
		// add/remove allowed units

		//Wyrmgus start
		if (stat.Variables.empty()) { // unit types stats not initialized
			break;
		}
		//Wyrmgus end
		
		// FIXME: check if modify is allowed

		player.Allow.Units[unit_type->Slot] -= um->ChangeUnits[unit_type->Slot];

		// this modifier should be applied to unittype id == z
		if (um->applies_to(unit_type)) {
			// if a unit type's supply is changed, we need to update the player's supply accordingly
			if (um->Modifier.Variables[SUPPLY_INDEX].Value) {
				std::vector<CUnit *> unitupgrade;

				FindUnitsByType(*unit_type, unitupgrade);
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

				FindUnitsByType(*unit_type, unitupgrade);
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
					int m = wyrmgus::resource::get_all()[j]->get_default_income();

					for (int k = 0; k < player.GetUnitCount(); ++k) {
						//Wyrmgus start
//						m = std::max(m, player.GetUnit(k).Type->Stats[player.Index].ImproveIncomes[j]);
						if (player.GetUnit(k).Type != nullptr) {
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

			for (const auto &kv_pair : um->Modifier.UnitStock) {
				const wyrmgus::unit_type *stock_unit_type = wyrmgus::unit_type::get_all()[kv_pair.first];
				const int unit_stock = kv_pair.second;
				if (unit_stock != 0) {
					stat.ChangeUnitStock(stock_unit_type, -unit_stock);
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
				int m = DefaultTradeCost;

				for (int k = 0; k < player.GetUnitCount(); ++k) {
					if (player.GetUnit(k).Type != nullptr) {
						m = std::min(m, player.GetUnit(k).Type->Stats[player.Index].Variables[TRADECOST_INDEX].Value);
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

				FindUnitsByType(*UnitTypes[z], unitupgrade, true);
				*/
				//Wyrmgus end
				for (CUnit *unit : unitupgrade) {
					if (unit->Player->Index != player.Index) {
						continue;
					}
					
					//Wyrmgus start
					if (
						(CUpgrade::get_all()[um->UpgradeId]->is_weapon() && unit->EquippedItems[static_cast<int>(wyrmgus::item_slot::weapon)].size() > 0)
						|| (CUpgrade::get_all()[um->UpgradeId]->is_shield() && unit->EquippedItems[static_cast<int>(wyrmgus::item_slot::shield)].size() > 0)
						|| (CUpgrade::get_all()[um->UpgradeId]->is_boots() && unit->EquippedItems[static_cast<int>(wyrmgus::item_slot::boots)].size() > 0)
						|| (CUpgrade::get_all()[um->UpgradeId]->is_arrows() && unit->EquippedItems[static_cast<int>(wyrmgus::item_slot::arrows)].size() > 0)
					) { //if the unit already has an item equipped of the same equipment type as this upgrade, don't remove the modifier from it (it already doesn't have it)
						continue;
					}
					//Wyrmgus end
					
					for (unsigned int j = 0; j < UnitTypeVar.GetNumberVariable(); j++) {
						unit->Variable[j].Enable |= um->Modifier.Variables[j].Enable;
						if (um->ModifyPercent[j]) {
							if (j != MANA_INDEX || um->ModifyPercent[j] >= 0) {
								unit->Variable[j].Value = unit->Variable[j].Value * 100 / (100 + um->ModifyPercent[j]);
							}
							unit->Variable[j].Max = unit->Variable[j].Max * 100 / (100 + um->ModifyPercent[j]);
						} else {
							if (j != MANA_INDEX || um->Modifier.Variables[j].Value >= 0) {
								unit->Variable[j].Value -= um->Modifier.Variables[j].Value;
							}
							unit->Variable[j].Increase -= um->Modifier.Variables[j].Increase;
						}

						unit->Variable[j].Max -= um->Modifier.Variables[j].Max;
						unit->Variable[j].Max = std::max(unit->Variable[j].Max, 0);

						if (unit->Variable[j].Max > 0) {
							unit->Variable[j].Value = std::clamp(unit->Variable[j].Value, 0, unit->Variable[j].Max);
						}

						//Wyrmgus start
						if (j == ATTACKRANGE_INDEX && unit->Container) {
							unit->Container->UpdateContainerAttackRange();
						} else if (j == LEVEL_INDEX || j == POINTS_INDEX) {
							unit->UpdateXPRequired();
						} else if (IsKnowledgeVariable(j)) {
							unit->CheckKnowledgeChange(j, - um->Modifier.Variables[j].Value);
						} else if ((j == SIGHTRANGE_INDEX || j == DAYSIGHTRANGEBONUS_INDEX || j == NIGHTSIGHTRANGEBONUS_INDEX) && !unit->Removed) {
							// If Sight range is upgraded, we need to change EVERY unit
							// to the new range, otherwise the counters get confused.
							MapUnmarkUnitSight(*unit);
							UpdateUnitSightRange(*unit);
							MapMarkUnitSight(*unit);
						}
						//Wyrmgus end
					}
					
					for (const auto &kv_pair : um->Modifier.UnitStock) {
						const wyrmgus::unit_type *stock_unit_type = wyrmgus::unit_type::get_all()[kv_pair.first];
						const int unit_stock = kv_pair.second;
						if (unit_stock > 0) {
							unit->ChangeUnitStock(stock_unit_type, -unit_stock);
						}
					}
				}
			}
			
			//Wyrmgus start
			for (size_t j = 0; j != unitupgrade.size(); ++j) {
				CUnit &unit = *unitupgrade[j];

				if (unit.Player->Index != player.Index) {
					continue;
				}
				
				//add or remove starting abilities from the unit if the upgrade enabled/disabled them
				for (const CUpgrade *ability_upgrade : unit.Type->StartingAbilities) {
					if (!unit.GetIndividualUpgrade(ability_upgrade) && check_conditions(ability_upgrade, &unit)) {
						IndividualUpgradeAcquire(unit, ability_upgrade);
					} else if (unit.GetIndividualUpgrade(ability_upgrade) && !check_conditions(ability_upgrade, &unit)) {
						IndividualUpgradeLost(unit, ability_upgrade);
					}
				}
				
				//change variation if current one becomes forbidden
				const wyrmgus::unit_type_variation *current_variation = unit.GetVariation();
				if (current_variation != nullptr) {
					if (!unit.can_have_variation(current_variation)) {
						unit.ChooseVariation();
					}
				}
				for (int i = 0; i < MaxImageLayers; ++i) {
					const wyrmgus::unit_type_variation *current_layer_variation = unit.GetLayerVariation(i);
					if (current_layer_variation != nullptr) {
						if (!unit.can_have_variation(current_layer_variation)) {
							unit.ChooseVariation(nullptr, false, i);
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
void ApplyIndividualUpgradeModifier(CUnit &unit, const wyrmgus::upgrade_modifier *um)
{
	Assert(um);

	for (size_t i = 0; i < um->RemoveUpgrades.size(); ++i) {
		if (unit.GetIndividualUpgrade(um->RemoveUpgrades[i])) {
			IndividualUpgradeLost(unit, um->RemoveUpgrades[i], true);
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
	
	for (const auto &kv_pair : um->Modifier.UnitStock) {
		const wyrmgus::unit_type *unit_type = wyrmgus::unit_type::get_all()[kv_pair.first];
		const int unit_stock = kv_pair.second;
		if (unit_stock < 0) {
			unit.ChangeUnitStock(unit_type, unit_stock);
		}
	}
	
	//Wyrmgus start
	//change variation if current one becomes forbidden
	const wyrmgus::unit_type_variation *current_variation = unit.GetVariation();
	if (current_variation != nullptr) {
		if (!unit.can_have_variation(current_variation)) {
			unit.ChooseVariation();
		}
	}
	for (int i = 0; i < MaxImageLayers; ++i) {
		const wyrmgus::unit_type_variation *current_layer_variation = unit.GetLayerVariation(i);
		if (current_layer_variation != nullptr) {
			if (!unit.can_have_variation(current_layer_variation)) {
				unit.ChooseVariation(nullptr, false, i);
			}
		}
	}
	unit.UpdateButtonIcons();
	//Wyrmgus end
	
	if (um->ConvertTo) {
		//Wyrmgus start
		//CommandTransformIntoType(unit, *um->ConvertTo);
		if (unit.get_character() == nullptr) { //don't do this for persistent characters
			CommandTransformIntoType(unit, *um->ConvertTo);
		}
		//Wyrmgus end
	}
}

void RemoveIndividualUpgradeModifier(CUnit &unit, const wyrmgus::upgrade_modifier *um)
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

	for (const auto &kv_pair : um->Modifier.UnitStock) {
		const wyrmgus::unit_type *unit_type = wyrmgus::unit_type::get_all()[kv_pair.first];
		const int unit_stock = kv_pair.second;
		if (unit_stock > 0) {
			unit.ChangeUnitStock(unit_type, -unit_stock);
		}
	}
	
	//Wyrmgus start
	//change variation if current one becomes forbidden
	const wyrmgus::unit_type_variation *current_variation = unit.GetVariation();
	if (current_variation != nullptr) {
		if (!unit.can_have_variation(current_variation)) {
			unit.ChooseVariation();
		}
	}
	for (int i = 0; i < MaxImageLayers; ++i) {
		const wyrmgus::unit_type_variation *current_layer_variation = unit.GetLayerVariation(i);
		if (current_layer_variation != nullptr) {
			if (!unit.can_have_variation(current_layer_variation)) {
				unit.ChooseVariation(nullptr, false, i);
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

	const wyrmgus::deity *upgrade_deity = upgrade->get_deity();
	if (upgrade_deity != nullptr) { // if is a deity upgrade
		for (const wyrmgus::magic_domain *domain : upgrade_deity->get_domains()) {
			const CUpgrade *domain_upgrade = domain->get_upgrade();
			if (player.Allow.Upgrades[domain_upgrade->ID] != 'R') {
				UpgradeAcquire(player, domain_upgrade);
			}
		}
		player.Deities.push_back(upgrade_deity);
	}
	//Wyrmgus end

	if (upgrade->get_dynasty() != nullptr && player.get_dynasty() != upgrade->get_dynasty()) {
		player.set_dynasty(upgrade->get_dynasty());
	}
	
	for (const auto &modifier : upgrade->get_modifiers()) {
		ApplyUpgradeModifier(player, modifier.get());
	}

	player.check_age();
	
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
void UpgradeLost(CPlayer &player, int id)
{
	//Wyrmgus start
	if (!GameRunning && !GameEstablishing) {
		return;
	}
	//Wyrmgus end
	player.UpgradeTimers.Upgrades[id] = 0;
	AllowUpgradeId(player, id, 'A'); // research is lost i.e. available
	
	const CUpgrade *upgrade = CUpgrade::get_all()[id];
	const wyrmgus::deity *upgrade_deity = upgrade->get_deity();
	if (upgrade_deity != nullptr) {
		for (const wyrmgus::magic_domain *domain : upgrade_deity->get_domains()) {
			const CUpgrade *domain_upgrade = domain->get_upgrade();
			if (player.Allow.Upgrades[domain_upgrade->ID] == 'R') {
				UpgradeLost(player, domain_upgrade->ID);
			}
		}
		wyrmgus::vector::remove(player.Deities, upgrade_deity);
	}

	for (const auto &modifier : upgrade->get_modifiers()) {
		RemoveUpgradeModifier(player, modifier.get());
	}

	//upgrades could change the buttons displayed.
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
	for (CUpgrade *upgrade : CUpgrade::get_all()) {
		for (int p = 0; p < PlayerMax; ++p) {
			if (CPlayer::Players[p]->Allow.Upgrades[upgrade->ID] == 'R') {
				int id = upgrade->ID;
				CPlayer::Players[p]->UpgradeTimers.Upgrades[id] = upgrade->Costs[TimeCost];
				AllowUpgradeId(*CPlayer::Players[p], id, 'R');  // research done

				for (const auto &modifier : upgrade->get_modifiers()) {
					ApplyUpgradeModifier(*CPlayer::Players[p], modifier.get());
				}
			}
		}
	}
	
	for (int p = 0; p < PlayerMax; ++p) {
		CPlayer::Players[p]->check_age();
	}
}

void AbilityAcquire(CUnit &unit, const CUpgrade *upgrade, bool save)
{
	unit.Variable[LEVELUP_INDEX].Value -= 1;
	unit.Variable[LEVELUP_INDEX].Max = unit.Variable[LEVELUP_INDEX].Value;
	if (!IsNetworkGame() && unit.get_character() != nullptr && save) {
		if (unit.Player == CPlayer::GetThisPlayer()) { //save ability learning, if unit has a character and it is persistent, and the character doesn't have the ability yet
			unit.get_character()->add_ability(upgrade);
			SaveHero(unit.get_character());
		}
	}
	IndividualUpgradeAcquire(unit, upgrade);
	unit.Player->UpdateLevelUpUnits();
}

void AbilityLost(CUnit &unit, CUpgrade *upgrade, bool lose_all)
{
	unit.Variable[LEVELUP_INDEX].Value += 1;
	unit.Variable[LEVELUP_INDEX].Max = unit.Variable[LEVELUP_INDEX].Value;
	unit.Variable[LEVELUP_INDEX].Enable = 1;
	if (!IsNetworkGame() && unit.get_character() != nullptr) {
		if (wyrmgus::vector::contains(unit.get_character()->get_abilities(), upgrade)) {
			if (unit.Player == CPlayer::GetThisPlayer()) { //save ability learning, if unit has a character and it is persistent, and the character doesn't have the ability yet
				unit.get_character()->remove_ability(upgrade);
				SaveHero(unit.get_character());
			}
		}
	}
	IndividualUpgradeLost(unit, upgrade);
	unit.Player->UpdateLevelUpUnits();
	
	if (lose_all && unit.GetIndividualUpgrade(upgrade) > 0) {
		AbilityLost(unit, upgrade, lose_all);
	}
}

void TraitAcquire(CUnit &unit, CUpgrade *upgrade)
{
	if (unit.Trait != nullptr) { //remove previous trait, if any
		if (!GameSettings.NoRandomness) { // if in no randomness setting, don't change trait modifiers
			IndividualUpgradeLost(unit, unit.Trait);
		}
	}

	unit.Trait = upgrade;

	if (!GameSettings.NoRandomness) { // if in no randomness setting, don't apply trait modifiers
		IndividualUpgradeAcquire(unit, upgrade);
	}
	
	unit.UpdateExtraName();

	//
	//  Upgrades could change the buttons displayed.
	//
	if (unit.Player == CPlayer::GetThisPlayer()) {
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
	
	const wyrmgus::deity *upgrade_deity = upgrade->get_deity();
	if (upgrade_deity != nullptr) {
		for (const wyrmgus::magic_domain *domain : upgrade_deity->get_domains()) {
			const CUpgrade *domain_upgrade = domain->get_upgrade();
			if (unit.GetIndividualUpgrade(domain_upgrade) == 0) {
				IndividualUpgradeAcquire(unit, domain_upgrade);
			}
		}
		if (unit.get_character() != nullptr && !wyrmgus::vector::contains(unit.get_character()->Deities, upgrade_deity) && unit.Player == CPlayer::GetThisPlayer()) {
			unit.get_character()->Deities.push_back(upgrade_deity);
			SaveHero(unit.get_character());
		}
	}

	if (!upgrade->is_ability() || upgrade->WeaponClasses.empty() || upgrade->WeaponClasses.contains(unit.GetCurrentWeaponClass())) {
		for (const auto &modifier : upgrade->get_modifiers()) {
			bool applies_to_this = false;
			bool applies_to_any_unit_types = false;
			for (const wyrmgus::unit_type *unit_type : wyrmgus::unit_type::get_all()) {
				if (unit_type->is_template()) {
					continue;
				}

				if (modifier->applies_to(unit_type)) {
					applies_to_any_unit_types = true;
					if (unit_type == unit.Type) {
						applies_to_this = true;
						break;
					}
				}
			}
			if (applies_to_this || !applies_to_any_unit_types) { //if the modifier isn't designated as being for a specific unit type, or is designated for this unit's unit type, apply it
				ApplyIndividualUpgradeModifier(unit, modifier.get());
			}
		}
	}

	//
	//  Upgrades could change the buttons displayed.
	//
	if (unit.Player == CPlayer::GetThisPlayer()) {
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

	const wyrmgus::deity *upgrade_deity = upgrade->get_deity();
	if (upgrade_deity != nullptr) {
		for (const wyrmgus::magic_domain *domain : upgrade_deity->get_domains()) {
			const CUpgrade *domain_upgrade = domain->get_upgrade();
			if (unit.GetIndividualUpgrade(domain_upgrade) > 0) {
				IndividualUpgradeLost(unit, domain_upgrade);
			}
		}
		if (unit.get_character() != nullptr && unit.Player == CPlayer::GetThisPlayer()) {
			wyrmgus::vector::remove(unit.get_character()->Deities, upgrade_deity);
			SaveHero(unit.get_character());
		}
	}

	//Wyrmgus start
	if (!upgrade->is_ability() || upgrade->WeaponClasses.empty() || upgrade->WeaponClasses.contains(unit.GetCurrentWeaponClass())) {
		for (const auto &modifier : upgrade->get_modifiers()) {
			bool applies_to_this = false;
			bool applies_to_any_unit_types = false;
			for (const wyrmgus::unit_type *unit_type : wyrmgus::unit_type::get_all()) {
				if (unit_type->is_template()) {
					continue;
				}

				if (modifier->applies_to(unit_type)) {
					applies_to_any_unit_types = true;
					if (unit_type == unit.Type) {
						applies_to_this = true;
						break;
					}
				}
			}
			if (applies_to_this || !applies_to_any_unit_types) { //if the modifier isn't designated as being for a specific unit type, or is designated for this unit's unit type, remove it
				RemoveIndividualUpgradeModifier(unit, modifier.get());
			}
		}
	}
	//Wyrmgus end

	//
	//  Upgrades could change the buttons displayed.
	//
	if (unit.Player == CPlayer::GetThisPlayer()) {
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
	int id = UpgradeIdByIdent(ident);

	if (id != -1) {
		return UpgradeIdAllowed(player, id);
	}
	DebugPrint("Fix your code, wrong identifier '%s'\n" _C_ ident.c_str());
	return '-';
}

//Wyrmgus start
std::string GetUpgradeEffectsString(const std::string &upgrade_ident, bool grand_strategy, bool multiline)
{
	const CUpgrade *upgrade = CUpgrade::try_get(upgrade_ident);
	
	std::string padding_string = ", ";
	if (multiline) {
		padding_string = "\n";
	}

	if (upgrade) {
		std::string upgrade_effects_string;
		
		bool first_element = true;
		//check if the upgrade makes modifications to any units
		for (const auto &modifier : upgrade->get_modifiers()) {
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

				if (modifier->Modifier.Variables[var].Value != 0) {
					if (!first_var) {
						upgrade_effects_string += padding_string;
					} else {
						first_var = false;
					}

					if (IsBooleanVariable(var) && modifier->Modifier.Variables[var].Value < 0) {
						upgrade_effects_string += "Lose ";
					}
										
					if (!IsBooleanVariable(var)) {
						if (modifier->Modifier.Variables[var].Value > 0) {
							upgrade_effects_string += "+";
						}
						upgrade_effects_string += std::to_string(modifier->Modifier.Variables[var].Value);
						if (IsPercentageVariable(var)) {
							upgrade_effects_string += "%";
						}
						upgrade_effects_string += " ";
					}

					upgrade_effects_string += GetVariableDisplayName(var);
						
					bool first_unit_type = true;
					for (const wyrmgus::unit_type *unit_type : wyrmgus::unit_type::get_all()) {
						if (unit_type->is_template()) {
							continue;
						}

						if (modifier->applies_to(unit_type)) {
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
					
				if (modifier->Modifier.Variables[var].Increase != 0) {
					if (!first_var) {
						upgrade_effects_string += padding_string;
					} else {
						first_var = false;
					}

					if (modifier->Modifier.Variables[var].Increase > 0) {
						upgrade_effects_string += "+";
					}
					upgrade_effects_string += std::to_string(modifier->Modifier.Variables[var].Increase);
					upgrade_effects_string += " ";
											
					upgrade_effects_string += GetVariableDisplayName(var, true);
				}
			}
				
			if (!grand_strategy) {
				bool first_res = true;
				for (int i = 0; i < MaxCosts; ++i) {
					if (modifier->Modifier.ImproveIncomes[i]) {
						if (!first_res) {
							upgrade_effects_string += padding_string;
						} else {
							first_res = false;
						}
							
						if (modifier->Modifier.ImproveIncomes[i] > 0) {
							upgrade_effects_string += "+";
						}
						upgrade_effects_string += std::to_string(modifier->Modifier.ImproveIncomes[i]);
						upgrade_effects_string += "%";
						upgrade_effects_string += " ";
						upgrade_effects_string += CapitalizeString(DefaultResourceNames[i]);
						upgrade_effects_string += " Processing";
							
						bool first_unit_type = true;
						for (const wyrmgus::unit_type *unit_type : wyrmgus::unit_type::get_all()) {
							if (unit_type->is_template()) {
								continue;
							}

							if (modifier->applies_to(unit_type)) {
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
	variable_name = FindAndReplaceString(variable_name, "DaySightRangeBonus", "DaySight");
	variable_name = FindAndReplaceString(variable_name, "NightSightRangeBonus", "NightSight");
	variable_name = FindAndReplaceString(variable_name, "SightRange", "Sight");
	variable_name = FindAndReplaceString(variable_name, "AttackRange", "Range");
	variable_name = FindAndReplaceString(variable_name, "HitPointBonus", "HitPoints");
	variable_name = FindAndReplaceString(variable_name, "Supply", "FoodSupply");
	variable_name = FindAndReplaceString(variable_name, "Demand", "FoodCost");
	variable_name = SeparateCapitalizedStringElements(variable_name);
	variable_name = FindAndReplaceString(variable_name, "Backstab", "Backstab Bonus");
	variable_name = FindAndReplaceString(variable_name, "Knowledge Magic", "Knowledge (Magic)");
	variable_name = FindAndReplaceString(variable_name, "Knowledge Warfare", "Knowledge (Warfare)");

	return _(variable_name.c_str());
}
//Wyrmgus end
