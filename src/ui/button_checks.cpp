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

#include "stratagus.h"

#include "actions.h"
//Wyrmgus start
#include "action/action_research.h"
#include "action/action_train.h"
#include "action/action_upgradeto.h"
//Wyrmgus end
#include "network.h"
#include "player.h"
#include "script.h"
#include "script/condition/condition.h"
#include "ui/button.h"
#include "ui/button_cmd.h"
#include "ui/interface.h"
#include "unit/unit.h"
#include "unit/unit_type.h"
#include "upgrade/upgrade.h"
#include "util/vector_util.h"

/**
**  ButtonCheck for button enabled, always true.
**  This needed to overwrite the internal tests.
**
**  @param unit    Pointer to unit for button.
**  @param button  Pointer to button to check/enable.
**
**  @return        True if enabled.
*/
bool ButtonCheckTrue(const CUnit &, const wyrmgus::button &)
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
bool ButtonCheckFalse(const CUnit &, const wyrmgus::button &)
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
bool ButtonCheckUpgrade(const CUnit &unit, const wyrmgus::button &button)
{
	const CPlayer *player = unit.Player;

	for (const std::string &str : button.allow_strings) {
		if (UpgradeIdentAllowed(*player, str) != 'R') {
			return false;
		}
	}

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
bool ButtonCheckUpgradeNot(const CUnit &unit, const wyrmgus::button &button)
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
bool ButtonCheckUpgradeOr(const CUnit &unit, const wyrmgus::button &button)
{
	const CPlayer *player = unit.Player;

	for (const std::string &str : button.allow_strings) {
		if (UpgradeIdentAllowed(*player, str) == 'R') {
			return true;
		}
	}

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
bool ButtonCheckIndividualUpgrade(const CUnit &unit, const wyrmgus::button &button)
{
	for (const std::string &str : button.allow_strings) {
		if (unit.GetIndividualUpgrade(CUpgrade::get(str)) == 0) {
			return false;
		}
	}
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
bool ButtonCheckIndividualUpgradeOr(const CUnit &unit, const wyrmgus::button &button)
{
	for (const std::string &str : button.allow_strings) {
		if (unit.GetIndividualUpgrade(CUpgrade::get(str)) > 0) {
			return true;
		}
	}
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
bool ButtonCheckUnitVariable(const CUnit &unit, const wyrmgus::button &button)
{
	for (size_t i = 0; i < button.allow_strings.size(); ++i) {
		const std::string &var = button.allow_strings[i];
		++i;
		const std::string &type = button.allow_strings[i];
		++i;
		const std::string &binop = button.allow_strings[i];
		++i;
		const std::string &value = button.allow_strings[i];
		const int index = UnitTypeVar.VariableNameLookup[var.c_str()];// User variables
		if (index == -1) {
			throw std::runtime_error("Bad variable name \"" + var + "\".");
		}
		int varValue = 0;
		if (type == "Value") {
			//Wyrmgus start
//			varValue = unit.Variable[index].Value;
			varValue = unit.GetModifiedVariable(index, VariableAttribute::Value);
			//Wyrmgus end
		} else if (type == "Max") {
			//Wyrmgus start
//			varValue = unit.Variable[index].Max;
			varValue = unit.GetModifiedVariable(index, VariableAttribute::Max);
			//Wyrmgus end
		} else if (type == "Increase") {
			//Wyrmgus start
//			varValue = unit.Variable[index].Increase;
			varValue = unit.GetModifiedVariable(index, VariableAttribute::Increase);
			//Wyrmgus end
		} else if (type == "Enable") {
			varValue = unit.Variable[index].Enable;
		} else if (type == "Percent") {
			//Wyrmgus start
//			varValue = unit.Variable[index].Value * 100 / unit.Variable[index].Max;
			varValue = unit.GetModifiedVariable(index, VariableAttribute::Value) * 100 / unit.GetModifiedVariable(index, VariableAttribute::Max);
			//Wyrmgus end
		} else {
			throw std::runtime_error("Bad variable type \"" + type + "\".");
		}
		const int cmpValue = std::stoi(value);
		bool cmpResult = false;
		if (binop == ">") {
			cmpResult = varValue > cmpValue;
		} else if (binop == ">=") {
			cmpResult = varValue >= cmpValue;
		} else if (binop == "<") {
			cmpResult = varValue < cmpValue;
		} else if (binop == "<=") {
			cmpResult = varValue <= cmpValue;
		} else if (binop == "==") {
			cmpResult = varValue == cmpValue;
		} else if (binop == "!=") {
			cmpResult = varValue != cmpValue;
		} else {
			throw std::runtime_error("Bad compare type \"" + binop + "\".");
		}
		if (cmpResult == false) {
			return false;
		}
	}
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
bool ButtonCheckUnitsOr(const CUnit &unit, const wyrmgus::button &button)
{
	const CPlayer *player = unit.Player;

	for (const std::string &str : button.allow_strings) {
		const wyrmgus::unit_type *unit_type = wyrmgus::unit_type::try_get(str);
		if (unit_type != nullptr && player->has_unit_type(unit_type)) {
			return true;
		}
	}
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
bool ButtonCheckUnitsAnd(const CUnit &unit, const wyrmgus::button &button)
{
	const CPlayer *player = unit.Player;

	for (const std::string &str : button.allow_strings) {
		const wyrmgus::unit_type *unit_type = wyrmgus::unit_type::try_get(str);
		if (unit_type != nullptr && player->has_unit_type(unit_type)) {
			return false;
		}
	}
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
bool ButtonCheckUnitsNot(const CUnit &unit, const wyrmgus::button &button)
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
bool ButtonCheckNetwork(const CUnit &, const wyrmgus::button &)
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
bool ButtonCheckNoNetwork(const CUnit &, const wyrmgus::button &)
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
bool ButtonCheckNoWork(const CUnit &unit, const wyrmgus::button &)
{
	const UnitAction action = unit.CurrentAction();
	//Wyrmgus start
//	return action != UnitAction::Train
//		   && action != UnitAction::UpgradeTo
//		   && action != UnitAction::Research;
	//don't stop showing the button for a quick moment if the time cost is 0
	return (action != UnitAction::Train || static_cast<COrder_Train *>(unit.CurrentOrder())->GetUnitType().Stats[unit.Player->get_index()].get_time_cost() == 0)
		   && (action != UnitAction::UpgradeTo || static_cast<COrder_UpgradeTo *>(unit.CurrentOrder())->GetUnitType().Stats[unit.Player->get_index()].get_time_cost() == 0)
		   && (action != UnitAction::Research || static_cast<COrder_Research *>(unit.CurrentOrder())->GetUpgrade().get_time_cost() == 0);
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
bool ButtonCheckNoResearch(const CUnit &unit, const wyrmgus::button &)
{
	const UnitAction action = unit.CurrentAction();
	return action != UnitAction::UpgradeTo && action != UnitAction::Research;
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
bool ButtonCheckUpgradeTo(const CUnit &unit, const wyrmgus::button &button)
{
	if (unit.CurrentAction() != UnitAction::Still) {
		return false;
	}

	const wyrmgus::unit_type *unit_type = button.get_value_unit_type(&unit);

	if (unit_type == nullptr) {
		return false;
	}

	return check_conditions<true>(unit_type, unit.Player, false);
}

/**
**  Check if all requirements for an attack are met.
**
**  @param unit    Pointer to unit for button.
**  @param button  Pointer to button to check/enable.
**
**  @return        True if enabled.
*/
bool ButtonCheckAttack(const CUnit &unit, const wyrmgus::button &)
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
bool ButtonCheckResearch(const CUnit &unit, const wyrmgus::button &button)
{
	// don't show any if working
	if (!ButtonCheckNoWork(unit, button)) {
		return false;
	}

	const CUpgrade *upgrade = button.get_value_upgrade(&unit);

	// check if allowed
	if (!check_conditions<true>(upgrade, unit.Player, false)) {
		return false;
	}

	if ((button.Action == ButtonCmd::Research || button.Action == ButtonCmd::ResearchClass)
		&& UpgradeIdAllowed(*unit.Player, upgrade->ID) != 'A' && UpgradeIdAllowed(*unit.Player, upgrade->ID) != 'R') {
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
bool ButtonCheckSingleResearch(const CUnit &unit, const wyrmgus::button &button)
{
	if (ButtonCheckResearch(unit, button)
		//Wyrmgus start
//		&& !unit.Player->UpgradeTimers.Upgrades[UpgradeIdByIdent(button.ValueStr)]) {
		&& (!unit.Player->UpgradeTimers.Upgrades[UpgradeIdByIdent(button.ValueStr)] || unit.Player->UpgradeTimers.Upgrades[UpgradeIdByIdent(button.ValueStr)] == CUpgrade::get(button.ValueStr)->get_time_cost())
	) {
		//Wyrmgus end
		return true;
	}
	return false;
}

/**
**  Check for button enabled, if the unit has an inventory.
**
**  @param unit    Pointer to unit for button.
**  @param button  Pointer to button to check/enable.
**
**  @return        True if enabled.
*/
bool ButtonCheckHasInventory(const CUnit &unit, const wyrmgus::button &button)
{
	Q_UNUSED(button)

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
bool ButtonCheckHasSubButtons(const CUnit &unit, const wyrmgus::button &button)
{
	for (const wyrmgus::button *other_button : wyrmgus::button::get_all()) {
		if (other_button->GetLevelID() != button.Value) {
			continue;
		}
		
		if (other_button->Action == ButtonCmd::Button && (other_button->Value == button.GetLevelID() || other_button->Value == 0)) { //don't count buttons to return to the level where this button is, or buttons to return to the default level
			continue;
		}

		std::array<char, 128> unit_ident{};
		sprintf(unit_ident.data(), ",%s,", unit.Type->Ident.c_str());
		if (other_button->UnitMask[0] != '*' && !strstr(other_button->UnitMask.c_str(), unit_ident.data()) && !wyrmgus::vector::contains(other_button->get_unit_classes(), unit.Type->get_unit_class())) {
			continue;
		}
		
		if (!other_button->is_always_shown() && !IsButtonAllowed(unit, *other_button)) {
			continue;
		}
		
		return true;
	}
	
	return false;
}
