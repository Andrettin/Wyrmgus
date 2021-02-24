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

	const std::string variable_name = string::snake_case_to_pascal_case(key);

	const int index = UnitTypeVar.VariableNameLookup[variable_name.c_str()]; // variable index
	if (index != -1) { // valid index
		if (string::is_number(value)) {
			this->Modifier.Variables[index].Enable = 1;
			this->Modifier.Variables[index].Value = std::stoi(value);
			this->Modifier.Variables[index].Max = std::stoi(value);
		} else { // error
			throw std::runtime_error("Invalid value (\"" + value +"\") for variable \"" + key + "\" when defining modifier for upgrade \"" + CUpgrade::get_all()[this->UpgradeId]->get_identifier() + "\".");
		}
	} else {
		throw std::runtime_error("Invalid upgrade modifier property: \"" + key + "\".");
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

}
