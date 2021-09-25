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

#include "stratagus.h"

#include "animation/animation_setvar.h"

#include "actions.h"
//Wyrmgus start
#include "commands.h"
//Wyrmgus end
#include "script.h"
#include "unit/unit.h"
#include "unit/unit_manager.h"
#include "util/assert_util.h"
#include "util/string_util.h"

void CAnimation_SetVar::Action(CUnit &unit, int &/*move*/, int /*scale*/) const
{
	assert_throw(unit.Anim.Anim == this);

	CUnit *goal = &unit;

	if (!goal) {
		return;
	}

	const std::vector<std::string> str_list = wyrmgus::string::split(this->var_str, '.');

	const int index = UnitTypeVar.VariableNameLookup[str_list[0]]; //user variables
	if (index == -1) {
		throw std::runtime_error("Bad variable name \"" + str_list[0] + "\".");
	}

	const int rop = this->value;
	int value = 0;
	if (str_list[1] == "Value") {
		value = goal->Variable[index].Value;
	}

	switch (this->mod) {
		case modAdd:
			value += rop;
			break;
		default:
			value = rop;
	}

	if (str_list[1] == "Value") {
		goal->Variable[index].Value = value;
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
void CAnimation_SetVar::Init(const char *s, lua_State *)
{
	const std::string str(s);
	const size_t len = str.size();

	size_t begin = 0;
	size_t end = str.find(' ', begin);
	this->var_str.assign(str, begin, end - begin);

	begin = std::min(len, str.find_first_not_of(' ', end));
	end = std::min(len, str.find(' ', begin));
	const std::string modStr(str, begin, end - begin);

	if (modStr == "=") {
		this->mod = modSet;
	} else if (modStr == "+=") {
		this->mod = modAdd;
	} else {
		this->mod = static_cast<SetVar_ModifyTypes>(std::stoi(modStr));
	}

	std::string value_str;
	begin = std::min(len, str.find_first_not_of(' ', end));
	end = std::min(len, str.find(' ', begin));
	value_str.assign(str, begin, end - begin);
	this->value = std::stoi(value_str);
}
