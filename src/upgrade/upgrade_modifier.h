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
//      (c) Copyright 1999-2019 by Vladi Belperchinov-Shabanski,
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

#ifndef __UPGRADE_MODIFIER_H__
#define __UPGRADE_MODIFIER_H__

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "upgrade/upgrade_structs.h" //for CUnitStats

#include <core/object.h>
#include <core/method_bind_free_func.gen.inc>

#include <map>
#include <set>
#include <vector>

/*----------------------------------------------------------------------------
--  Declarations
----------------------------------------------------------------------------*/

class CConfigData;
class CDynasty;
class CFaction;
class CUnitType;
class CUpgrade;
class UnitClass;
struct lua_State;

/*----------------------------------------------------------------------------
--  Definition
----------------------------------------------------------------------------*/

/**
**  This is the modifier of an upgrade.
**  This does the real action of an upgrade, and an upgrade can have multiple
**  modifiers.
*/
class CUpgradeModifier : public Object
{
	GDCLASS(CUpgradeModifier, Object)
	
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
	
	bool AppliesToUnitType(const CUnitType *unit_type) const;
	
	bool AppliesToUnitClass(const UnitClass *unit_class) const
	{
		return this->ApplyToUnitClasses.find(unit_class) != this->ApplyToUnitClasses.end();
	}
	
	int GetUnitStock(const CUnitType *unit_type) const;
	void SetUnitStock(const CUnitType *unit_type, const int quantity);
	void ChangeUnitStock(const CUnitType *unit_type, const int quantity);

	int UpgradeId = 0;						/// used to filter required modifier

	CUnitStats Modifier;					/// modifier of unit stats.
	int *ModifyPercent = nullptr;			/// use for percent modifiers
	int SpeedResearch = 0;					/// speed factor for researching
	int ImproveIncomes[MaxCosts];			/// improve incomes
	std::map<const CUnitType *, int> UnitStock;	/// unit stock
	// allow/forbid bitmaps -- used as chars for example:
	// `?' -- leave as is, `F' -- forbid, `A' -- allow
	// TODO: see below allow more semantics?
	// TODO: pointers or ids would be faster and less memory use
	int  ChangeUnits[UnitTypeMax];			/// add/remove allowed units
	char ChangeUpgrades[UpgradeMax];		/// allow/forbid upgrades
private:
	std::set<const CUnitType *> ApplyToUnitTypes;	/// which unit types are affected
	std::set<const UnitClass *> ApplyToUnitClasses;	/// which unit classes are affected
public:

	CUnitType *ConvertTo = nullptr;			/// convert to this unit-type.

	//Wyrmgus start
	int ChangeCivilizationTo = -1;			/// changes the player's civilization to this one
	CFaction *ChangeFactionTo = nullptr;	/// changes the player's faction to this one
	CDynasty *ChangeDynastyTo = nullptr;	/// changes the player's dynasty to this one
	
	std::vector<CUpgrade *> RemoveUpgrades;	/// Upgrades to be removed when this upgrade modifier is implented
	//Wyrmgus end
	
	friend int CclDefineModifier(lua_State *l);
	
protected:
	static void _bind_methods();
};

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

#endif
