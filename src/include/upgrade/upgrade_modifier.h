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
/**@name upgrade_modifier.h - The upgrade modifier header file. */
//
//      (c) Copyright 1999-2020 by Vladi Belperchinov-Shabanski,
//		Jimmy Salmon and Andrettin
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

#pragma once

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "upgrade/upgrade_structs.h" //for CUnitStats

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CConfigData;
class CDynasty;
class CFaction;
class CUnitType;
class CUpgrade;

namespace stratagus {
	class sml_data;
	class sml_property;
}

/**
**  This is the modifier of an upgrade.
**  This does the real action of an upgrade, and an upgrade can have multiple
**  modifiers.
*/
class CUpgradeModifier
{
public:
	CUpgradeModifier();
	~CUpgradeModifier()
	{
		if (this->ModifyPercent) {
			delete [] this->ModifyPercent;
		}
	}
	
	static std::vector<CUpgradeModifier *> UpgradeModifiers;
	
	void ProcessConfigData(const CConfigData *config_data);
	void process_sml_property(const stratagus::sml_property &property);
	void process_sml_scope(const stratagus::sml_data &scope);
	
	int GetUnitStock(CUnitType *unit_type) const;
	void SetUnitStock(CUnitType *unit_type, int quantity);
	void ChangeUnitStock(CUnitType *unit_type, int quantity);

	int UpgradeId = 0;						/// used to filter required modifier

	CUnitStats Modifier;					/// modifier of unit stats.
	int *ModifyPercent = nullptr;			/// use for percent modifiers
	int SpeedResearch = 0;					/// speed factor for researching
	int ImproveIncomes[MaxCosts];			/// improve incomes
	std::map<CUnitType *, int> UnitStock;	/// unit stock
	// allow/forbid bitmaps -- used as chars for example:
	// `?' -- leave as is, `F' -- forbid, `A' -- allow
	// TODO: see below allow more semantics?
	// TODO: pointers or ids would be faster and less memory use
	int  ChangeUnits[UnitTypeMax];			/// add/remove allowed units
	char ChangeUpgrades[UpgradeMax];		/// allow/forbid upgrades
	char ApplyTo[UnitTypeMax];				/// which unit types are affected

	CUnitType *ConvertTo = nullptr;			/// convert to this unit-type.

	//Wyrmgus start
	int change_civilization_to = -1;		/// changes the player's civilization to this one
	CFaction *ChangeFactionTo = nullptr;	/// changes the player's faction to this one
	CDynasty *ChangeDynastyTo = nullptr;	/// changes the player's dynasty to this one
	
	std::vector<CUpgrade *> RemoveUpgrades;	/// Upgrades to be removed when this upgrade modifier is implented
	//Wyrmgus end
};
