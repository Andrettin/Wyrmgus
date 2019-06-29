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
/**@name upgrade_modifier.cpp - The upgrade modifier source file. */
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

#include "upgrade/upgrade_modifier.h"

#include "config.h"
#include "config_operator.h"
#include "unit/unit_class.h"
#include "unit/unit_type.h"
#include "upgrade/upgrade.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

/// Upgrades modifiers
std::vector<CUpgradeModifier *> CUpgradeModifier::UpgradeModifiers;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Constructor
*/
CUpgradeModifier::CUpgradeModifier()
{
	memset(this->ChangeUnits, 0, sizeof(this->ChangeUnits));
	
	memset(this->ChangeUpgrades, '?', sizeof(this->ChangeUpgrades));
	this->Modifier.Variables = new CVariable[UnitTypeVar.GetNumberVariable()];
	this->ModifyPercent = new int[UnitTypeVar.GetNumberVariable()];
	memset(this->ModifyPercent, 0, UnitTypeVar.GetNumberVariable() * sizeof(int));
}

/**
**	@brief	Process data provided by a configuration file
**
**	@param	config_data	The configuration data
*/
void CUpgradeModifier::ProcessConfigData(const CConfigData *config_data)
{
	for (const CConfigProperty &property : config_data->Properties) {
		if (property.ProcessForObject(*this)) {
			continue;
		}
		
		if (property.Operator != CConfigOperator::Assignment) {
			print_error("Wrong operator enumeration index for property \"" + property.Key + "\": " + String::num_int64(static_cast<int>(property.Operator)) + ".");
			continue;
		}
		
		String key = property.Key;
		String value = property.Value;
		
		if (key == "remove_upgrade") {
			value = value.replace("_", "-");
			CUpgrade *removed_upgrade = CUpgrade::Get(value.utf8().get_data());
			if (removed_upgrade) {
				this->RemoveUpgrades.push_back(removed_upgrade);
			} else {
				fprintf(stderr, "Invalid upgrade: \"%s\".\n", value.utf8().get_data());
			}
		} else {
			key = SnakeCaseToPascalCase(key);
			
			int index = UnitTypeVar.VariableNameLookup[key.utf8().get_data()]; // variable index
			if (index != -1) { // valid index
				if (value.is_valid_integer()) {
					this->Modifier.Variables[index].Enable = 1;
					this->Modifier.Variables[index].Value = value.to_int();
					this->Modifier.Variables[index].Max = value.to_int();
				} else { // error
					fprintf(stderr, "Invalid value (\"%s\") for variable \"%s\" when defining modifier for upgrade \"%s\".\n", value.utf8().get_data(), key.utf8().get_data(), CUpgrade::Get(this->UpgradeId)->Ident.c_str());
				}
			} else {
				fprintf(stderr, "Invalid upgrade modifier property: \"%s\".\n", key.utf8().get_data());
			}
		}
	}
	
	for (const CConfigData *section : config_data->Sections) {
		String tag = SnakeCaseToPascalCase(section->Tag);
		
		const int index = UnitTypeVar.VariableNameLookup[tag.utf8().get_data()]; // variable index
		
		if (index != -1) { // valid index
			int var_value = 0;
			bool percent = false;
			bool increase = false;
			
			for (const CConfigProperty &property : section->Properties) {
				if (property.Operator != CConfigOperator::Assignment) {
					print_error("Wrong operator enumeration index for property \"" + property.Key + "\": " + String::num_int64(static_cast<int>(property.Operator)) + ".");
					continue;
				}
				
				String key = property.Key;
				String value = property.Value;
				
				if (key == "value") {
					var_value = value.to_int();
				} else if (key == "percent") {
					percent = StringToBool(value);
				} else if (key == "increase") {
					increase = StringToBool(value);
				} else {
					fprintf(stderr, "Invalid upgrade modifier variable property: \"%s\".\n", key.utf8().get_data());
				}
			}
			
			if (percent) {
				this->ModifyPercent[index] = var_value;
			} else if (increase) {
				this->Modifier.Variables[index].Increase = var_value;
			} else {
				this->Modifier.Variables[index].Enable = 1;
				this->Modifier.Variables[index].Value = var_value;
				this->Modifier.Variables[index].Max = var_value;
			}
		} else {
			fprintf(stderr, "Invalid upgrade modifier section: \"%s\".\n", section->Tag.utf8().get_data());
		}
	}
}

bool CUpgradeModifier::AppliesToUnitType(const CUnitType *unit_type) const
{
	if (this->ApplyToUnitTypes.find(unit_type) != this->ApplyToUnitTypes.end()) {
		return true;
	}
	
	if (unit_type->GetClass() != nullptr && this->AppliesToUnitClass(unit_type->GetClass())) {
		return true;
	}
	
	return false;
}
int CUpgradeModifier::GetUnitStock(const CUnitType *unit_type) const
{
	if (unit_type && this->UnitStock.find(unit_type) != this->UnitStock.end()) {
		return this->UnitStock.find(unit_type)->second;
	} else {
		return 0;
	}
}

void CUpgradeModifier::SetUnitStock(const CUnitType *unit_type, const int quantity)
{
	if (!unit_type) {
		return;
	}
	
	if (quantity == 0) {
		if (this->UnitStock.find(unit_type) != this->UnitStock.end()) {
			this->UnitStock.erase(unit_type);
		}
	} else {
		this->UnitStock[unit_type] = quantity;
	}
}

void CUpgradeModifier::ChangeUnitStock(const CUnitType *unit_type, const int quantity)
{
	this->SetUnitStock(unit_type, this->GetUnitStock(unit_type) + quantity);
}

void CUpgradeModifier::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("add_to_apply_to_unit_types", "ident"), +[](CUpgradeModifier *modifier, const String &ident){ modifier->ApplyToUnitTypes.insert(CUnitType::Get(ident)); });
	ClassDB::bind_method(D_METHOD("remove_from_apply_to_unit_types", "ident"), +[](CUpgradeModifier *modifier, const String &ident){ modifier->ApplyToUnitTypes.erase(CUnitType::Get(ident)); });
	ClassDB::bind_method(D_METHOD("get_apply_to_unit_types"), +[](const CUpgradeModifier *modifier){ return ContainerToGodotArray(modifier->ApplyToUnitTypes); });
	
	ClassDB::bind_method(D_METHOD("add_to_apply_to_unit_classes", "ident"), +[](CUpgradeModifier *modifier, const String &ident){ modifier->ApplyToUnitClasses.insert(UnitClass::Get(ident)); });
	ClassDB::bind_method(D_METHOD("remove_from_apply_to_unit_classes", "ident"), +[](CUpgradeModifier *modifier, const String &ident){ modifier->ApplyToUnitClasses.erase(UnitClass::Get(ident)); });
	ClassDB::bind_method(D_METHOD("get_apply_to_unit_classes"), +[](const CUpgradeModifier *modifier){ return ContainerToGodotArray(modifier->ApplyToUnitClasses); });
}
