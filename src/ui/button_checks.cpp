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
/**@name button_checks.cpp - The button checks. */
//
//      (c) Copyright 1999-2015 by Lutz Sammer, Vladi Belperchinov-Shabanski
//      and Andrettin
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

#include "action/actions.h"
//Wyrmgus start
#include "action/action_research.h"
#include "action/action_train.h"
#include "action/action_upgradeto.h"
//Wyrmgus end
#include "network/network.h"
#include "player.h"
#include "script.h"
#include "ui/button_action.h"
#include "ui/interface.h"
#include "unit/unit.h"
#include "unit/unit_type.h"
#include "upgrade/dependency.h"
#include "upgrade/upgrade.h"

#include <stdio.h>
/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**  ButtonCheck for button enabled, always true.
**  This needed to overwrite the internal tests.
**
**  @param unit    Pointer to unit for button.
**  @param button  Pointer to button to check/enable.
**
**  @return        True if enabled.
*/
bool ButtonCheckTrue(const CUnit &, const ButtonAction &)
{
	return true;
}

/**
**  Check for button enabled, always false.
**  This needed to overwrite the internal tests.
**
**  @param unit    Pointer to unit for button.
**  @param button  Pointer to button to check/enable.
**
**  @return        True if enabled.
*/
bool ButtonCheckFalse(const CUnit &, const ButtonAction &)
{
	return false;
}

/**
**  Check for button enabled, if upgrade is ready.
**
**  @param unit    Pointer to unit for button.
**  @param button  Pointer to button to check/enable.
**
**  @return        True if enabled.
*/
bool ButtonCheckUpgrade(const CUnit &unit, const ButtonAction &button)
{
	CPlayer *player = unit.Player;
	char *buf = new_strdup(button.AllowStr.c_str());

	for (const char *s = strtok(buf, ","); s; s = strtok(nullptr, ",")) {
		if (UpgradeIdentAllowed(*unit.Player, s) != 'R') {
			delete[] buf;
			return false;
		}
	}
	delete[] buf;
	return true;
}

/**
**  Check for the button being enabled, if the upgrade has not been acquired.
**
**  @param unit    Pointer to unit for button.
**  @param button  Pointer to button to check/enable.
**
**  @return        True if enabled.
*/
bool ButtonCheckUpgradeNot(const CUnit &unit, const ButtonAction &button)
{
	return !ButtonCheckUpgrade(unit, button);
}

/**
**  Check for button enabled, if upgrade is ready.
**
**  @param unit    Pointer to unit for button.
**  @param button  Pointer to button to check/enable.
**
**  @return        True if enabled.
*/
bool ButtonCheckUpgradeOr(const CUnit &unit, const ButtonAction &button)
{
	CPlayer *player = unit.Player;
	char *buf = new_strdup(button.AllowStr.c_str());

	for (const char *s = strtok(buf, ","); s; s = strtok(nullptr, ",")) {
		if (UpgradeIdentAllowed(*unit.Player, s) == 'R') {
			delete[] buf;
			return true;
		}
	}
	delete[] buf;
	return false;
}

/**
**  Check for button enabled, if unit has an individual upgrade.
**
**  @param unit    Pointer to unit for button.
**  @param button  Pointer to button to check/enable.
**
**  @return        True if enabled.
*/
bool ButtonCheckIndividualUpgrade(const CUnit &unit, const ButtonAction &button)
{
	char *buf = new_strdup(button.AllowStr.c_str());

	for (const char *s = strtok(buf, ","); s; s = strtok(nullptr, ",")) {
		if (unit.GetIndividualUpgrade(CUpgrade::Get(s)) == 0) {
			delete[] buf;
			return false;
		}
	}
	delete[] buf;
	return true;
}

/**
**  Check for button enabled, if unit has any of the individual upgrades.
**
**  @param unit    Pointer to unit for button.
**  @param button  Pointer to button to check/enable.
**
**  @return        True if enabled.
*/
bool ButtonCheckIndividualUpgradeOr(const CUnit &unit, const ButtonAction &button)
{
	char *buf = new_strdup(button.AllowStr.c_str());

	for (const char *s = strtok(buf, ","); s; s = strtok(nullptr, ",")) {
		if (unit.GetIndividualUpgrade(CUpgrade::Get(s)) > 0) {
			delete[] buf;
			return true;
		}
	}
	delete[] buf;
	return false;
}

/**
**  Check for button enabled, if unit's variables pass the condition check.
**
**  @param unit    Pointer to unit for button.
**  @param button  Pointer to button to check/enable.
**
**  @return        True if enabled.
*/
bool ButtonCheckUnitVariable(const CUnit &unit, const ButtonAction &button)
{
	char *buf = new_strdup(button.AllowStr.c_str());

	for (const char *var = strtok(buf, ","); var; var = strtok(nullptr, ",")) {
		const char *type = strtok(nullptr, ",");
		const char *binop = strtok(nullptr, ",");
		const char *value = strtok(nullptr, ",");
		const int index = UnitTypeVar.VariableNameLookup[var];// User variables
		if (index == -1) {
			fprintf(stderr, "Bad variable name '%s'\n", var);
			Exit(1);
			return false;
		}
		int varValue;
		if (!strcmp(type, "Value")) {
			//Wyrmgus start
//			varValue = unit.Variable[index].Value;
			varValue = unit.GetModifiedVariable(index, VariableValue);
			//Wyrmgus end
		} else if (!strcmp(type, "Max")) {
			//Wyrmgus start
//			varValue = unit.Variable[index].Max;
			varValue = unit.GetModifiedVariable(index, VariableMax);
			//Wyrmgus end
		} else if (!strcmp(type, "Increase")) {
			//Wyrmgus start
//			varValue = unit.Variable[index].Increase;
			varValue = unit.GetModifiedVariable(index, VariableIncrease);
			//Wyrmgus end
		} else if (!strcmp(type, "Enable")) {
			varValue = unit.Variable[index].Enable;
		} else if (!strcmp(type, "Percent")) {
			//Wyrmgus start
//			varValue = unit.Variable[index].Value * 100 / unit.Variable[index].Max;
			varValue = unit.GetModifiedVariable(index, VariableValue) * 100 / unit.GetModifiedVariable(index, VariableMax);
			//Wyrmgus end
		} else {
			fprintf(stderr, "Bad variable type '%s'\n", type);
			Exit(1);
			return false;
		}
		const int cmpValue = atoi(value);
		bool cmpResult = false;
		if (!strcmp(binop, ">")) {
			cmpResult = varValue > cmpValue;
		} else if (!strcmp(binop, ">=")) {
			cmpResult = varValue >= cmpValue;
		} else if (!strcmp(binop, "<")) {
			cmpResult = varValue < cmpValue;
		} else if (!strcmp(binop, "<=")) {
			cmpResult = varValue <= cmpValue;
		} else if (!strcmp(binop, "==")) {
			cmpResult = varValue == cmpValue;
		} else if (!strcmp(binop, "!=")) {
			cmpResult = varValue != cmpValue;
		} else {
			fprintf(stderr, "Bad compare type '%s'\n", binop);
			Exit(1);
			return false;
		}
		if (cmpResult == false) {
			delete[] buf;
			return false;
		}
	}
	delete[] buf;
	return true;
}

/**
**  Check for button enabled, if any unit is available.
**
**  @param unit    Pointer to unit for button.
**  @param button  Pointer to button to check/enable.
**
**  @return        True if enabled.
*/
bool ButtonCheckUnitsOr(const CUnit &unit, const ButtonAction &button)
{
	CPlayer *player = unit.Player;
	char *buf = new_strdup(button.AllowStr.c_str());

	for (const char *s = strtok(buf, ","); s; s = strtok(nullptr, ",")) {
		if (player->HaveUnitTypeByIdent(s)) {
			delete[] buf;
			return true;
		}
	}
	delete[] buf;
	return false;
}

/**
**  Check for button enabled, if all units are available.
**
**  @param unit    Pointer to unit for button.
**  @param button  Pointer to button to check/enable.
**
**  @return        True if enabled.
*/
bool ButtonCheckUnitsAnd(const CUnit &unit, const ButtonAction &button)
{
	CPlayer *player = unit.Player;
	char *buf = new_strdup(button.AllowStr.c_str());

	for (const char *s = strtok(buf, ","); s; s = strtok(nullptr, ",")) {
		if (!player->HaveUnitTypeByIdent(s)) {
			delete[] buf;
			return false;
		}
	}
	delete[] buf;
	return true;
}

/**
**  Check for button enabled, if no unit is available.
**
**  @param unit    Pointer to unit for button.
**  @param button  Pointer to button to check/enable.
**
**  @return        True if enabled.
*/
bool ButtonCheckUnitsNot(const CUnit &unit, const ButtonAction &button)
{
	return !ButtonCheckUnitsAnd(unit, button);
}

/**
**  Check if network play is enabled.
**
**  @param unit    Pointer to unit for button.
**  @param button  Pointer to button to check/enable.
**
**  @return        True if enabled.
**
**  @note: this check could also be moved into intialisation.
*/
bool ButtonCheckNetwork(const CUnit &, const ButtonAction &)
{
	return IsNetworkGame();
}

/**
**  Check if network play is disabled.
**
**  @param unit    Pointer to unit for button.
**  @param button  Pointer to button to check/enable.
**
**  @return        True if disabled.
**
**  @note: this check could also be moved into intialisation.
*/
bool ButtonCheckNoNetwork(const CUnit &, const ButtonAction &)
{
	return !IsNetworkGame();
}

/**
**  Check for button enabled, if the unit isn't working.
**  Working is training, upgrading, researching.
**
**  @param unit    Pointer to unit for button.
**  @param button  Pointer to button to check/enable.
**
**  @return        True if enabled.
*/
bool ButtonCheckNoWork(const CUnit &unit, const ButtonAction &)
{
	int action = unit.CurrentAction();
	//Wyrmgus start
//	return action != UnitActionTrain
//		   && action != UnitActionUpgradeTo
//		   && action != UnitActionResearch;
	//don't stop showing the button for a quick moment if the time cost is 0
	return (action != UnitActionTrain || static_cast<COrder_Train *>(unit.CurrentOrder())->GetUnitType().Stats[unit.Player->Index].Costs[TimeCost] == 0)
		   && (action != UnitActionUpgradeTo || static_cast<COrder_UpgradeTo *>(unit.CurrentOrder())->GetUnitType().Stats[unit.Player->Index].Costs[TimeCost] == 0)
		   && (action != UnitActionResearch || static_cast<COrder_Research *>(unit.CurrentOrder())->GetUpgrade().Costs[TimeCost] == 0);
	//Wyrmgus end
}

/**
**  Check for button enabled, if the unit isn't researching.
**
**  @param unit    Pointer to unit for button.
**  @param button  Pointer to button to check/enable.
**
**  @return        True if enabled.
*/
bool ButtonCheckNoResearch(const CUnit &unit, const ButtonAction &)
{
	int action = unit.CurrentAction();
	return action != UnitActionUpgradeTo && action != UnitActionResearch;
}

/**
**  Check for button enabled, if all requirements for an upgrade to unit
**  are met.
**
**  @param unit    Pointer to unit for button.
**  @param button  Pointer to button to check/enable.
**
**  @return        True if enabled.
*/
bool ButtonCheckUpgradeTo(const CUnit &unit, const ButtonAction &button)
{
	if (unit.CurrentAction() != UnitActionStill) {
		return false;
	}
	return CheckDependencies(CUnitType::UnitTypes[button.Value], unit.Player, false, true);
}

/**
**  Check if all requirements for an attack are met.
**
**  @param unit    Pointer to unit for button.
**  @param button  Pointer to button to check/enable.
**
**  @return        True if enabled.
*/
bool ButtonCheckAttack(const CUnit &unit, const ButtonAction &)
{
	//Wyrmgus start
//	return unit.Type->CanAttack;
	return unit.CanAttack(true);
	//Wyrmgus end
}

/**
**  Check if all requirements for upgrade research are met.
**
**  @param unit    Pointer to unit for button.
**  @param button  Pointer to button to check/enable.
**
**  @return        True if enabled.
*/
bool ButtonCheckResearch(const CUnit &unit, const ButtonAction &button)
{
	// don't show any if working
	if (!ButtonCheckNoWork(unit, button)) {
		return false;
	}

	// check if allowed
	if (!CheckDependencies(AllUpgrades[button.Value], unit.Player, false, true)) {
		return false;
	}
	if (!strncmp(button.ValueStr.c_str(), "upgrade-", 8)
		&& UpgradeIdentAllowed(*unit.Player, button.ValueStr) != 'A' && UpgradeIdentAllowed(*unit.Player, button.ValueStr) != 'R') {
		return false;
	}
	return true;
}

/**
**  Check if all requirements for upgrade research are met only one
**  running research allowed.
**
**  @param unit    Pointer to unit for button.
**  @param button  Pointer to button to check/enable.
**
**  @return        True if enabled.
*/
bool ButtonCheckSingleResearch(const CUnit &unit, const ButtonAction &button)
{
	if (ButtonCheckResearch(unit, button)
		//Wyrmgus start
//		&& !unit.Player->UpgradeTimers.Upgrades[UpgradeIdByIdent(button.ValueStr)]) {
		&& (!unit.Player->UpgradeTimers.Upgrades[UpgradeIdByIdent(button.ValueStr)] || unit.Player->UpgradeTimers.Upgrades[UpgradeIdByIdent(button.ValueStr)] == AllUpgrades[UpgradeIdByIdent(button.ValueStr)]->Costs[TimeCost])
	) {
		//Wyrmgus end
		return true;
	}
	return false;
}

//Wyrmgus start
/**
**  Check for button enabled, if the unit has an inventory.
**
**  @param unit    Pointer to unit for button.
**  @param button  Pointer to button to check/enable.
**
**  @return        True if enabled.
*/
bool ButtonCheckHasInventory(const CUnit &unit, const ButtonAction &button)
{
	return Selected.size() == 1 && unit.HasInventory();
}

/**
**  Check for button enabled, if the button level it leads to has any sub buttons.
**
**  @param unit    Pointer to unit for button.
**  @param button  Pointer to button to check/enable.
**
**  @return        True if enabled.
*/
bool ButtonCheckHasSubButtons(const CUnit &unit, const ButtonAction &button)
{
	for (size_t i = 0; i < UnitButtonTable.size(); ++i) {
		if (UnitButtonTable[i]->GetLevelIndex() != button.Value) {
			continue;
		}
		
		if (UnitButtonTable[i]->Action == ButtonButton && (UnitButtonTable[i]->Value == button.GetLevelIndex() || UnitButtonTable[i]->Value == 0)) { //don't count buttons to return to the level where this button is, or buttons to return to the default level
			continue;
		}

		char unit_ident[128];
		sprintf(unit_ident, ",%s,", unit.Type->Ident.c_str());
		if (UnitButtonTable[i]->UnitMask[0] != '*' && !strstr(UnitButtonTable[i]->UnitMask.c_str(), unit_ident)) {
			continue;
		}
		
		if (!UnitButtonTable[i]->AlwaysShow && !IsButtonAllowed(unit, *UnitButtonTable[i])) {
			continue;
		}
		
		return true;
	}
	
	return false;
}
//Wyrmgus end
