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

#include "upgrade/upgrade_modifier.h"

#include "config.h"
#include "game/game.h"
#include "map/map.h"
#include "unit/unit.h"
#include "unit/unit_class.h"
#include "unit/unit_type.h"
#include "upgrade/upgrade.h"
#include "util/string_util.h"
#include "util/vector_util.h"

namespace wyrmgus {

/// Upgrades modifiers
std::vector<upgrade_modifier *> upgrade_modifier::UpgradeModifiers;

upgrade_modifier::upgrade_modifier()
{
	memset(this->ChangeUnits, 0, sizeof(this->ChangeUnits));
	
	memset(this->ChangeUpgrades, '?', sizeof(this->ChangeUpgrades));
	this->Modifier.Variables.resize(UnitTypeVar.GetNumberVariable());
	this->ModifyPercent = std::make_unique<int[]>(UnitTypeVar.GetNumberVariable());
	memset(this->ModifyPercent.get(), 0, UnitTypeVar.GetNumberVariable() * sizeof(int));
}

std::unique_ptr<upgrade_modifier> upgrade_modifier::duplicate() const
{
	auto modifier = std::make_unique<upgrade_modifier>();

	modifier->Modifier = this->Modifier;
	memcpy(modifier->ModifyPercent.get(), this->ModifyPercent.get(), sizeof(UnitTypeVar.GetNumberVariable()));
	modifier->SpeedResearch = this->SpeedResearch;
	modifier->infantry_cost_modifier = this->infantry_cost_modifier;
	modifier->cavalry_cost_modifier = this->cavalry_cost_modifier;
	memcpy(modifier->ImproveIncomes, this->ImproveIncomes, sizeof(modifier->ImproveIncomes));
	modifier->UnitStock = this->UnitStock;
	memcpy(modifier->ChangeUnits, this->ChangeUnits, sizeof(modifier->ChangeUnits));
	memcpy(modifier->ChangeUpgrades, this->ChangeUpgrades, sizeof(modifier->ChangeUpgrades));
	modifier->unit_types = this->unit_types;
	modifier->unit_classes = this->unit_classes;
	modifier->ConvertTo = this->ConvertTo;
	modifier->change_civilization_to = this->change_civilization_to;
	modifier->change_faction_to = this->change_faction_to;
	modifier->RemoveUpgrades = this->RemoveUpgrades;

	return modifier;
}

void upgrade_modifier::process_sml_property(const sml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "infantry_cost_modifier") {
		this->infantry_cost_modifier = std::stoi(value);
	} else if (key == "cavalry_cost_modifier") {
		this->cavalry_cost_modifier = std::stoi(value);
	} else {
		const std::string variable_name = string::snake_case_to_pascal_case(key);

		const int index = UnitTypeVar.VariableNameLookup[variable_name.c_str()]; // variable index
		if (index != -1) { // valid index
			if (string::is_number(value)) {
				this->Modifier.Variables[index].Enable = 1;
				this->Modifier.Variables[index].Value = std::stoi(value);
				this->Modifier.Variables[index].Max = std::stoi(value);
			} else { // error
				throw std::runtime_error("Invalid value (\"" + value + "\") for variable \"" + key + "\" when defining modifier for upgrade \"" + CUpgrade::get_all()[this->UpgradeId]->get_identifier() + "\".");
			}
		} else {
			throw std::runtime_error("Invalid upgrade modifier property: \"" + key + "\".");
		}
	}
}

void upgrade_modifier::process_sml_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "unit_types") {
		for (const std::string &value : values) {
			this->unit_types.push_back(unit_type::get(value));
		}
	} else if (tag == "unit_classes") {
		for (const std::string &value : values) {
			this->unit_classes.push_back(unit_class::get(value));
		}
	} else if (tag == "remove_upgrades") {
		for (const std::string &value : values) {
			CUpgrade *removed_upgrade = CUpgrade::get(value);
			this->RemoveUpgrades.push_back(removed_upgrade);
		}
	} else if (tag == "costs") {
		scope.for_each_property([&](const sml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			const resource *resource = resource::get(key);
			this->Modifier.set_cost(resource, std::stoi(value));
		});
	} else if (tag == "processing_bonus") {
		scope.for_each_property([&](const sml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			const resource *resource = resource::get(key);
			this->Modifier.set_improve_income(resource, std::stoi(value));
		});
	} else {
		const std::string variable_name = string::snake_case_to_pascal_case(tag);
		const int index = UnitTypeVar.VariableNameLookup[variable_name.c_str()]; // variable index
		if (index != -1) { // valid index
			scope.for_each_property([&](const sml_property &property) {
				const std::string &key = property.get_key();
				if (key == "percent_value") {
					this->ModifyPercent[index] = std::stoi(property.get_value());
				} else {
					throw std::runtime_error("Invalid upgrade modifier variable property: \"" + key + "\".");
				}
			});
		} else {
			throw std::runtime_error("Invalid upgrade modifier scope: \"" + tag + "\".");
		}
	}
}

bool upgrade_modifier::applies_to(const unit_type *unit_type) const
{
	if (vector::contains(this->get_unit_types(), unit_type)) {
		return true;
	}

	if (unit_type->get_unit_class() != nullptr && vector::contains(this->get_unit_classes(), unit_type->get_unit_class())) {
		return true;
	}

	return false;
}

int upgrade_modifier::GetUnitStock(unit_type *unit_type) const
{
	auto find_iterator = this->UnitStock.find(unit_type);
	if (unit_type && find_iterator != this->UnitStock.end()) {
		return find_iterator->second;
	} else {
		return 0;
	}
}

void upgrade_modifier::SetUnitStock(unit_type *unit_type, int quantity)
{
	if (!unit_type) {
		return;
	}
	
	if (quantity == 0) {
		if (this->UnitStock.contains(unit_type)) {
			this->UnitStock.erase(unit_type);
		}
	} else {
		this->UnitStock[unit_type] = quantity;
	}
}

void upgrade_modifier::ChangeUnitStock(unit_type *unit_type, int quantity)
{
	this->SetUnitStock(unit_type, this->GetUnitStock(unit_type) + quantity);
}

bool upgrade_modifier::affects_variable(const int var_index) const
{
	const unit_variable &modifier_variable = this->Modifier.Variables[var_index];

	return modifier_variable.Enable || modifier_variable.Value != 0 || modifier_variable.Max != 0 || modifier_variable.Increase != 0 || this->ModifyPercent[var_index] != 0;
}

void upgrade_modifier::apply_to_unit(CUnit *unit, const int multiplier) const
{
	for (unsigned int i = 0; i < UnitTypeVar.GetNumberVariable(); i++) {
		if (!this->affects_variable(i)) {
			continue;
		}

		switch (i) {
			case SIGHTRANGE_INDEX:
			case DAYSIGHTRANGEBONUS_INDEX:
			case NIGHTSIGHTRANGEBONUS_INDEX:
			case ETHEREALVISION_INDEX:
				if (!unit->Removed && !SaveGameLoading) {
					MapUnmarkUnitSight(*unit);
				}
				break;
			default:
				break;
		}

		unit_variable &unit_variable = unit->Variable[i];
		const wyrmgus::unit_variable &modifier_variable = this->Modifier.Variables[i];
		const int modify_percent = this->ModifyPercent[i];

		unit_variable.Enable |= modifier_variable.Enable;

		const int effective_modify_percent = modify_percent * multiplier;
		const int effective_modifier_value = modifier_variable.Value * multiplier;

		const int old_value = unit_variable.Value;

		if (effective_modify_percent) {
			if (i != MANA_INDEX || effective_modify_percent < 0) {
				if (multiplier < 0) {
					//need to calculate it a bit differently to "undo" the modification
					unit_variable.Value = unit_variable.Value * 100 / (100 - modify_percent);
				} else {
					unit_variable.Value += unit_variable.Value * modify_percent / 100;
				}
			}

			if (multiplier < 0) {
				unit_variable.Max = unit_variable.Max * 100 / (100 + modify_percent);
			} else {
				unit_variable.Max += unit_variable.Max * modify_percent / 100;
			}
		} else {
			if (i != MANA_INDEX || effective_modifier_value < 0) {
				unit_variable.Value += effective_modifier_value;
			}
			unit_variable.Increase += modifier_variable.Increase * multiplier;
		}

		unit_variable.Max += modifier_variable.Max * multiplier;
		unit_variable.Max = std::max(unit_variable.Max, 0);
		if (unit_variable.Max > 0) {
			unit_variable.Value = std::clamp(unit_variable.Value, 0, unit_variable.Max);
		}

		switch (i) {
			case SIGHTRANGE_INDEX:
			case DAYSIGHTRANGEBONUS_INDEX:
			case NIGHTSIGHTRANGEBONUS_INDEX:
			case ETHEREALVISION_INDEX:
				//if the sight range was modified, we need to change the unit to the new range, as otherwise the counters get confused
				if (!unit->Removed && !SaveGameLoading) {
					UpdateUnitSightRange(*unit);
					MapMarkUnitSight(*unit);
				}
				break;
			default:
				break;
		}

		unit->on_variable_changed(i, unit_variable.Value - old_value);
	}

	for (const auto &[stock_unit_type, unit_stock] : this->Modifier.get_unit_stocks()) {
		const int effective_unit_stock = unit_stock * multiplier;

		if (effective_unit_stock < 0) {
			unit->ChangeUnitStock(stock_unit_type, effective_unit_stock);
		}
	}
}

}
