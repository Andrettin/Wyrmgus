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
/**@name animation_setvar.cpp - The animation SetVar. */
//
//      (c) Copyright 2012 by Joris Dauphin
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

#include "stratagus.h"

#include "animation/animation_setvar.h"

#include "actions.h"
//Wyrmgus start
#include "commands.h"
//Wyrmgus end
#include "script.h"
#include "unit/unit.h"
#include "unit/unit_manager.h"


/* virtual */ void CAnimation_SetVar::Action(CUnit &unit, int &/*move*/, int /*scale*/) const
{
	Assert(unit.Anim.Anim == this);

	char arg1[128];
	CUnit *goal = &unit;
	strcpy(arg1, this->varStr.c_str());

	if (this->unitSlotStr.empty() == false) {
		switch (this->unitSlotStr[0]) {
			case 'l': // last created unit
				goal = wyrmgus::unit_manager::get()->lastCreatedUnit();
				break;
			case 't': // target unit
				goal = unit.CurrentOrder()->get_goal();
				break;
			case 's': // unit self (no use)
				goal = &unit;
				break;
		}
	}
	if (!goal) {
		return;
	}

	char *next = strchr(arg1, '.');
	if (next == nullptr) {
		// Special case for non-unit_variable variables
		if (!strcmp(arg1, "DamageType")) {
			int death = ExtraDeathIndex(this->valueStr.c_str());
			if (death == ANIMATIONS_DEATHTYPES) {
				throw std::runtime_error("Incorrect death type: " + this->valueStr + ".");
			}
			goal->Type->DamageType = this->valueStr;
			return;
		}
		throw std::runtime_error("Need also specify the variable \"" + std::string(arg1) + "\" tag.");
	} else {
		*next = '\0';
	}
	const int index = UnitTypeVar.VariableNameLookup[arg1];// User variables
	if (index == -1) {
		throw std::runtime_error("Bad variable name \"" + std::string(arg1) + "\".");
	}

	const int rop = ParseAnimInt(unit, this->valueStr);
	int value = 0;
	if (!strcmp(next + 1, "Value")) {
		value = goal->Variable[index].Value;
	} else if (!strcmp(next + 1, "Max")) {
		value = goal->Variable[index].Max;
	} else if (!strcmp(next + 1, "Increase")) {
		value = goal->Variable[index].Increase;
	} else if (!strcmp(next + 1, "Enable")) {
		value = goal->Variable[index].Enable;
	} else if (!strcmp(next + 1, "Percent")) {
		value = goal->Variable[index].Value * 100 / goal->Variable[index].Max;
	}
	switch (this->mod) {
		case modAdd:
			value += rop;
			break;
		case modSub:
			value -= rop;
			break;
		case modMul:
			value *= rop;
			break;
		case modDiv:
			if (!rop) {
				throw std::runtime_error("Division by zero in AnimationSetVar.");
			}
			value /= rop;
			break;
		case modMod:
			if (!rop) {
				throw std::runtime_error("Division by zero in AnimationSetVar.");
			}
			value %= rop;
			break;
		case modAnd:
			value &= rop;
			break;
		case modOr:
			value |= rop;
			break;
		case modXor:
			value ^= rop;
			break;
		case modNot:
			value = !value;
			break;
		default:
			value = rop;
	}
	if (!strcmp(next + 1, "Value")) {
		goal->Variable[index].Value = value;
	} else if (!strcmp(next + 1, "Max")) {
		goal->Variable[index].Max = value;
	} else if (!strcmp(next + 1, "Increase")) {
		goal->Variable[index].Increase = value;
	} else if (!strcmp(next + 1, "Enable")) {
		goal->Variable[index].Enable = value;
	} else if (!strcmp(next + 1, "Percent")) {
		goal->Variable[index].Value = goal->Variable[index].Max * value / 100;
	}
	//Wyrmgus start
//	goal->Variable[index].Value = std::clamp(goal->Variable[index].Value, 0, goal->Variable[index].Max);
	goal->Variable[index].Value = std::clamp(goal->Variable[index].Value, 0, goal->GetModifiedVariable(index, VariableAttribute::Max));
	//Wyrmgus end
	//Wyrmgus start
	if (index == ATTACKRANGE_INDEX && goal->Container) {
		goal->Container->UpdateContainerAttackRange();
	} else if (index == LEVEL_INDEX || index == POINTS_INDEX) {
		goal->UpdateXPRequired();
	} else if (index == XP_INDEX) {
		goal->XPChanged();
	} else if (index == STUN_INDEX && goal->Variable[index].Value > 0) { //if unit has become stunned, stop it
		CommandStopUnit(*goal);
	} else if (index == KNOWLEDGEMAGIC_INDEX) {
		goal->CheckIdentification();
	}
	//Wyrmgus end
}

/*
**  s = "var mod value [unitSlot]"
*/
/* virtual */ void CAnimation_SetVar::Init(const char *s, lua_State *)
{
	const std::string str(s);
	const size_t len = str.size();

	size_t begin = 0;
	size_t end = str.find(' ', begin);
	this->varStr.assign(str, begin, end - begin);

	begin = std::min(len, str.find_first_not_of(' ', end));
	end = std::min(len, str.find(' ', begin));
	const std::string modStr(str, begin, end - begin);

	if (modStr == "=") {
		this->mod = modSet;
	} else if (modStr == "+=") {
		this->mod = modAdd;
	} else if (modStr == "-=") {
		this->mod = modSub;
	} else if (modStr == "*=") {
		this->mod = modMul;
	} else if (modStr == "/=") {
		this->mod = modDiv;
	} else if (modStr == "%=") {
		this->mod = modMod;
	} else if (modStr == "&=") {
		this->mod = modAnd;
	} else if (modStr == "|=") {
		this->mod = modOr;
	} else if (modStr == "^=") {
		this->mod = modXor;
	} else if (modStr == "!") {
		this->mod = modNot;
	} else {
		this->mod = (SetVar_ModifyTypes)(atoi(modStr.c_str()));
	}

	begin = std::min(len, str.find_first_not_of(' ', end));
	end = std::min(len, str.find(' ', begin));
	this->valueStr.assign(str, begin, end - begin);

	begin = std::min(len, str.find_first_not_of(' ', end));
	end = std::min(len, str.find(' ', begin));
	this->unitSlotStr.assign(str, begin, end - begin);
}
