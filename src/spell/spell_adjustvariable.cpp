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
/**@name spell_adjustvariable.cpp - The spell AdjustVariable. */
//
//      (c) Copyright 1998-2020 by Vladi Belperchinov-Shabanski, Lutz Sammer,
//                                 Jimmy Salmon, Joris Dauphin and Andrettin
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

#include "spell/spell_adjustvariable.h"

//Wyrmgus start
#include "commands.h"
//Wyrmgus end
#include "config.h"
#include "script.h"
#include "unit/unit.h"
#include "util/string_util.h"

/**
**	@brief	Process data provided by a configuration file
**
**	@param	config_data	The configuration data
*/
void Spell_AdjustVariable::ProcessConfigData(const CConfigData *config_data)
{
	this->Var = new SpellActionTypeAdjustVariable[UnitTypeVar.GetNumberVariable()];
	
	for (size_t i = 0; i < config_data->Properties.size(); ++i) {
		std::string key = config_data->Properties[i].first;
		std::string value = config_data->Properties[i].second;
		
		key = string::snake_case_to_pascal_case(key);
		
		int index = UnitTypeVar.VariableNameLookup[key.c_str()];
		if (index != -1) {
			if (string::is_number(value)) {
				const int number_value = std::stoi(value);
				this->Var[index].Enable = number_value != 0;
				this->Var[index].ModifEnable = 1;
				this->Var[index].Value = number_value;
				this->Var[index].ModifValue = 1;
				this->Var[index].Max = number_value;
				this->Var[index].ModifMax = 1;
			} else {
				fprintf(stderr, "Invalid value (\"%s\") for variable \"%s\" when defining an adjust variable spell action.\n", value.c_str(), key.c_str());
			}
		} else {
			fprintf(stderr, "Invalid adjust variable spell action property: \"%s\".\n", key.c_str());
		}
	}
	
	for (const CConfigData *child_config_data : config_data->Children) {
		std::string tag = child_config_data->Tag;
		tag = string::snake_case_to_pascal_case(tag);
		
		int index = UnitTypeVar.VariableNameLookup[tag.c_str()];
		if (index != -1) {
			for (size_t j = 0; j < child_config_data->Properties.size(); ++j) {
				std::string key = child_config_data->Properties[j].first;
				std::string value = child_config_data->Properties[j].second;
				
				if (key == "enable") {
					this->Var[index].Enable = string::to_bool(value);
					this->Var[index].ModifEnable = 1;
				} else if (key == "value") {
					this->Var[index].Value = std::stoi(value);
					this->Var[index].ModifValue = 1;
				} else if (key == "max") {
					this->Var[index].Max = std::stoi(value);
					this->Var[index].ModifMax = 1;
				} else if (key == "increase") {
					this->Var[index].Increase = std::stoi(value);
					this->Var[index].ModifIncrease = 1;
				} else if (key == "invert_enable") {
					this->Var[index].InvertEnable = string::to_bool(value);
				} else if (key == "add_value") {
					this->Var[index].AddValue = std::stoi(value);
				} else if (key == "add_max") {
					this->Var[index].AddMax = std::stoi(value);
				} else if (key == "add_increase") {
					this->Var[index].AddIncrease = std::stoi(value);
				} else if (key == "increase_time") {
					this->Var[index].IncreaseTime = std::stoi(value);
				} else if (key == "target_is_caster") {
					if (value == "caster") {
						this->Var[index].TargetIsCaster = 1;
					} else if (value == "target") {
						this->Var[index].TargetIsCaster = 0;
					} else {
						fprintf(stderr, "Invalid target_is_caster value: \"%s\".\n", value.c_str());
					}
				} else {
					fprintf(stderr, "Invalid adjust variable spell action variable property: \"%s\".\n", key.c_str());
				}
			}
		} else {
			fprintf(stderr, "Invalid adjust variable spell action property: \"%s\".\n", tag.c_str());
		}
	}
}

/* virtual */ void Spell_AdjustVariable::Parse(lua_State *l, int startIndex, int endIndex)
{
	int j = startIndex;
	lua_rawgeti(l, -1, j + 1);
	if (!lua_istable(l, -1)) {
		LuaError(l, "Table expected for adjust-variable.");
	}
	this->Var = new SpellActionTypeAdjustVariable[UnitTypeVar.GetNumberVariable()];
	for (lua_pushnil(l); lua_next(l, -2); lua_pop(l, 1)) {
		const char *const name = LuaToString(l, -2);
		int i = UnitTypeVar.VariableNameLookup[name];

		if (i == -1) {
			LuaError(l, "in adjust-variable : Bad variable index : '%s'" _C_ LuaToString(l, -2));
		}
		if (lua_isnumber(l, -1)) {
			this->Var[i].Enable = (LuaToNumber(l, -1) != 0);
			this->Var[i].ModifEnable = 1;
			this->Var[i].Value = LuaToNumber(l, -1);
			this->Var[i].ModifValue = 1;
			this->Var[i].Max = LuaToNumber(l, -1);
			this->Var[i].ModifMax = 1;
		} else if (lua_istable(l, -1)) {
			for (lua_pushnil(l); lua_next(l, -2); lua_pop(l, 1)) {
				const char *const key = LuaToString(l, -2);
				if (!strcmp(key, "Enable")) {
					this->Var[i].Enable = LuaToBoolean(l, -1);
					this->Var[i].ModifEnable = 1;
				} else if (!strcmp(key, "Value")) {
					this->Var[i].Value = LuaToNumber(l, -1);
					this->Var[i].ModifValue = 1;
				} else if (!strcmp(key, "Max")) {
					this->Var[i].Max = LuaToNumber(l, -1);
					this->Var[i].ModifMax = 1;
				} else if (!strcmp(key, "Increase")) {
					this->Var[i].Increase = LuaToNumber(l, -1);
					this->Var[i].ModifIncrease = 1;
				} else if (!strcmp(key, "InvertEnable")) {
					this->Var[i].InvertEnable = LuaToBoolean(l, -1);
				} else if (!strcmp(key, "AddValue")) {
					this->Var[i].AddValue = LuaToNumber(l, -1);
				} else if (!strcmp(key, "AddMax")) {
					this->Var[i].AddMax = LuaToNumber(l, -1);
				} else if (!strcmp(key, "AddIncrease")) {
					this->Var[i].AddIncrease = LuaToNumber(l, -1);
				} else if (!strcmp(key, "IncreaseTime")) {
					this->Var[i].IncreaseTime = LuaToNumber(l, -1);
				} else if (!strcmp(key, "TargetIsCaster")) {
					const char *value = LuaToString(l, -1);
					if (!strcmp(value, "caster")) {
						this->Var[i].TargetIsCaster = 1;
					} else if (!strcmp(value, "target")) {
						this->Var[i].TargetIsCaster = 0;
					} else { // Error
						LuaError(l, "key '%s' not valid for TargetIsCaster in adjustvariable" _C_ value);
					}
				} else { // Error
					LuaError(l, "key '%s' not valid for adjustvariable" _C_ key);
				}
			}
		} else {
			LuaError(l, "in adjust-variable : Bad variable value");
		}
	}
	lua_pop(l, 1); // pop table

}

/**
**  Adjust User Variables.
**
**  @param caster   Unit that casts the spell
**  @param spell    Spell-type pointer
**  @param target   Target
**  @param goalPos  coord of target spot when/if target does not exist
**
**  @return        =!0 if spell should be repeated, 0 if not
*/
/* virtual */ int Spell_AdjustVariable::Cast(CUnit &caster, const CSpell &, CUnit *target, const Vec2i &/*goalPos*/, int /*z*/, int modifier)
{
	for (unsigned int i = 0; i < UnitTypeVar.GetNumberVariable(); ++i) {
		CUnit *unit = (this->Var[i].TargetIsCaster) ? &caster : target;

		if (!unit) {
			continue;
		}
		
		// Enable flag.
		if (this->Var[i].ModifEnable) {
			unit->Variable[i].Enable = this->Var[i].Enable;
		}
		unit->Variable[i].Enable ^= this->Var[i].InvertEnable;

		// Max field
		if (this->Var[i].ModifMax) {
			unit->Variable[i].Max = this->Var[i].Max;
		}
		unit->Variable[i].Max += this->Var[i].AddMax;

		// Increase field
		if (this->Var[i].ModifIncrease) {
			unit->Variable[i].Increase = this->Var[i].Increase;
		}
		unit->Variable[i].Increase += this->Var[i].AddIncrease;

		// Value field
		if (this->Var[i].ModifValue) {
			unit->Variable[i].Value = this->Var[i].Value * modifier / 100;
		}
		unit->Variable[i].Value += this->Var[i].AddValue * modifier / 100;
		unit->Variable[i].Value += this->Var[i].IncreaseTime * unit->Variable[i].Increase * modifier / 100;

		//Wyrmgus start
//		clamp(&unit->Variable[i].Value, 0, unit->Variable[i].Max);
		clamp(&unit->Variable[i].Value, 0, unit->GetModifiedVariable(i, VariableMax));
		//Wyrmgus end
		
		//Wyrmgus start
		if (i == ATTACKRANGE_INDEX && unit->Container) {
			unit->Container->UpdateContainerAttackRange();
		} else if (i == LEVEL_INDEX || i == POINTS_INDEX) {
			unit->UpdateXPRequired();
		} else if (i == XP_INDEX) {
			unit->XPChanged();
		} else if (i == STUN_INDEX && unit->Variable[i].Value > 0) { //if target has become stunned, stop it
			CommandStopUnit(*unit);
		} else if (i == KNOWLEDGEMAGIC_INDEX) {
			unit->CheckIdentification();
		}
		//Wyrmgus end
	}
	return 1;
}
