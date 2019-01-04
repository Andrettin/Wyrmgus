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

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "stratagus.h"

#include "upgrade/upgrade_modifier.h"

#include "config.h"
#include "unit/unittype.h"
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
	memset(this->ApplyTo, '?', sizeof(this->ApplyTo));
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
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		if (key == "apply_to") {
			value = FindAndReplaceString(value, "_", "-");
			const int unit_type_id = UnitTypeIdByIdent(value.c_str());
			if (unit_type_id != -1) {
				this->ApplyTo[unit_type_id] = 'X';
			} else {
				fprintf(stderr, "Invalid unit type: \"%s\".\n", value.c_str());
			}
		} else if (key == "remove_upgrade") {
			value = FindAndReplaceString(value, "_", "-");
			CUpgrade *removed_upgrade = CUpgrade::Get(value);
			if (removed_upgrade) {
				this->RemoveUpgrades.push_back(removed_upgrade);
			} else {
				fprintf(stderr, "Invalid upgrade: \"%s\".\n", value.c_str());
			}
		} else {
			key = SnakeCaseToPascalCase(key);
			
			int index = UnitTypeVar.VariableNameLookup[key.c_str()]; // variable index
			if (index != -1) { // valid index
				if (IsStringNumber(value)) {
					this->Modifier.Variables[index].Enable = 1;
					this->Modifier.Variables[index].Value = std::stoi(value);
					this->Modifier.Variables[index].Max = std::stoi(value);
				} else { // error
					fprintf(stderr, "Invalid value (\"%s\") for variable \"%s\" when defining modifier for upgrade \"%s\".\n", value.c_str(), key.c_str(), AllUpgrades[this->UpgradeId]->Ident.c_str());
				}
			} else {
				fprintf(stderr, "Invalid upgrade modifier property: \"%s\".\n", key.c_str());
			}
		}
	}
}

int CUpgradeModifier::GetUnitStock(CUnitType *unit_type) const
{
	if (unit_type && this->UnitStock.find(unit_type) != this->UnitStock.end()) {
		return this->UnitStock.find(unit_type)->second;
	} else {
		return 0;
	}
}

void CUpgradeModifier::SetUnitStock(CUnitType *unit_type, int quantity)
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

void CUpgradeModifier::ChangeUnitStock(CUnitType *unit_type, int quantity)
{
	this->SetUnitStock(unit_type, this->GetUnitStock(unit_type) + quantity);
}
