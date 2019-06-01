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
/**@name animation_variable.cpp - The animation variable source file. */
//
//      (c) Copyright 1998-2019 by Lutz Sammer, Russell Smith, Jimmy Salmon
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

#include "action/action_spellcast.h"
#include "animation/animation_variable.h"
#include "script.h"
#include "spell/spells.h"
#include "unit/unit.h"
#include "unit/unit_type.h"

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**	@brief	Parse the animation variable
**
**  @param	var_str	The animation variable string
*/
void AnimationVariable::Parse(const std::string &var_str)
{
	if (var_str.empty()) {
		return;
	}
	
	char s[100];

	strcpy(s, var_str.c_str());
	char *cur = &s[2];
	if (s[0] == 'v' || s[0] == 't') { //unit variable detected
		if (s[0] == 't') {
			this->Target = true;
		}
		char *next = strchr(cur, '.');
		if (next == nullptr) {
			fprintf(stderr, "Need also specify the variable '%s' tag \n", cur);
			ExitFatal(1);
		} else {
			*next = '\0';
		}
		this->VariableIndex = UnitTypeVar.VariableNameLookup[cur];// User variables
		if (this->VariableIndex == -1) {
			if (!strcmp(cur, "InsideCount")) {
				this->InsideCount = true;
			} else {
				fprintf(stderr, "Bad variable name '%s'\n", cur);
				ExitFatal(1);
			}
		}
		if (!strcmp(next + 1, "Value")) {
		} else if (!strcmp(next + 1, "Max")) {
			this->Max = true;
		} else if (!strcmp(next + 1, "Percent")) {
			this->Percent = true;
		}
	} else if (s[0] == 's') { //spell type detected
		this->Spell = CSpell::GetSpell(cur);
	} else {
		this->Value = std::stoi(var_str);
	}
}


/**
**	@brief	Get the value for the animation variable for a given unit
**
**  @return The animation variable value for the unit
*/
int AnimationVariable::GetValue(const CUnit *unit) const
{
	if (this->Target) {
		if (unit->CurrentOrder()->HasGoal()) {
			unit = unit->CurrentOrder()->GetGoal();
		} else {
			return 0;
		}
	}
	
	if (this->VariableIndex != -1) { //unit variable
		if (this->Max) {
			return unit->GetModifiedVariable(this->VariableIndex, VariableMax);
		} else if (this->Percent) {
			return unit->GetModifiedVariable(this->VariableIndex, VariableValue) * 100 / unit->GetModifiedVariable(this->VariableIndex, VariableMax);
		} else {
			return unit->GetModifiedVariable(this->VariableIndex, VariableValue);
		}
	} else if (this->InsideCount) {
		return unit->InsideCount;
	} else if (this->Spell != nullptr) { //spell type
		const COrder_SpellCast &order = *static_cast<COrder_SpellCast *>(unit->CurrentOrder());
		const CSpell *spell = &order.GetSpell();
		
		if (this->Spell == spell) {
			return 1;
		} else {
			return 0;
		}
	}

	return this->Value;
}
