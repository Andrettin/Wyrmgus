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
//      (c) Copyright 1998-2021 by Vladi Belperchinov-Shabanski, Lutz Sammer,
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

#include "stratagus.h"

#include "spell/spell_action_adjust_variable.h"

//Wyrmgus start
#include "commands.h"
//Wyrmgus end
#include "config.h"
#include "script.h"
#include "unit/unit.h"
#include "util/string_conversion_util.h"
#include "util/string_util.h"

namespace wyrmgus {

class spell_action_adjust_variable::variable_adjustment final
{
public:
	int Enable = 0;                 /// Value to affect to this field.
	int Value = 0;                  /// Value to affect to this field.
	int Max = 0;                    /// Value to affect to this field.
	int Increase = 0;               /// Value to affect to this field.

	bool ModifEnable = false;       /// true if we modify this field.
	bool ModifValue = false;        /// true if we modify this field.
	bool ModifMax = false;          /// true if we modify this field.
	bool ModifIncrease = false;     /// true if we modify this field.

	char InvertEnable = 0;          /// true if we invert this field.
	int AddValue = 0;               /// Add this value to this field.
	int AddMax = 0;                 /// Add this value to this field.
	int AddIncrease = 0;            /// Add this value to this field.
	int IncreaseTime = 0;           /// How many time increase the Value field.
	bool TargetIsCaster = false;    /// true if the target is the caster.
};

spell_action_adjust_variable::spell_action_adjust_variable()
{
	this->Var = std::make_unique<variable_adjustment[]>(UnitTypeVar.GetNumberVariable());
}

spell_action_adjust_variable::~spell_action_adjust_variable()
{
}

void spell_action_adjust_variable::process_sml_property(const sml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	const std::string pascal_case_key = string::snake_case_to_pascal_case(key);

	const int index = UnitTypeVar.VariableNameLookup[pascal_case_key.c_str()];
	if (index != -1) {
		if (string::is_number(value)) {
			const int number_value = std::stoi(value);
			this->Var[index].Enable = number_value != 0;
			this->Var[index].ModifEnable = true;
			this->Var[index].Value = number_value;
			this->Var[index].ModifValue = true;
			this->Var[index].Max = number_value;
			this->Var[index].ModifMax = true;
		} else {
			throw std::runtime_error("Invalid value (\"" + value + "\") for variable \"" + key + "\" when defining an adjust variable spell action.");
		}
	} else {
		throw std::runtime_error("Invalid adjust variable spell action property: \"" + key + "\".");
	}
}

void spell_action_adjust_variable::process_sml_scope(const sml_data &scope)
{
	const std::string &tag = scope.get_tag();

	const std::string pascal_case_tag = string::snake_case_to_pascal_case(tag);

	const int index = UnitTypeVar.VariableNameLookup[pascal_case_tag.c_str()];
	if (index != -1) {
		scope.for_each_property([&](const sml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			if (key == "enable") {
				this->Var[index].Enable = string::to_bool(value);
				this->Var[index].ModifEnable = true;
			} else if (key == "value") {
				this->Var[index].Value = std::stoi(value);
				this->Var[index].ModifValue = true;
			} else if (key == "max") {
				this->Var[index].Max = std::stoi(value);
				this->Var[index].ModifMax = true;
			} else if (key == "increase") {
				this->Var[index].Increase = std::stoi(value);
				this->Var[index].ModifIncrease = true;
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
					this->Var[index].TargetIsCaster = true;
				} else if (value == "target") {
					this->Var[index].TargetIsCaster = false;
				} else {
					throw std::runtime_error("Invalid target_is_caster value: \"" + value + "\".");
				}
			} else {
				throw std::runtime_error("Invalid adjust variable spell action variable property: \"" + key + "\".");
			}
		});
	} else {
		throw std::runtime_error("Invalid adjust variable spell action scope: \"" + tag + "\".");
	}
}

void spell_action_adjust_variable::Parse(lua_State *l, int startIndex, int endIndex)
{
	Q_UNUSED(endIndex)

	int j = startIndex;
	lua_rawgeti(l, -1, j + 1);
	if (!lua_istable(l, -1)) {
		LuaError(l, "Table expected for adjust-variable.");
	}
	for (lua_pushnil(l); lua_next(l, -2); lua_pop(l, 1)) {
		const char *const name = LuaToString(l, -2);
		int i = UnitTypeVar.VariableNameLookup[name];

		if (i == -1) {
			LuaError(l, "in adjust-variable : Bad variable index : '%s'" _C_ LuaToString(l, -2));
		}
		if (lua_isnumber(l, -1)) {
			this->Var[i].Enable = (LuaToNumber(l, -1) != 0);
			this->Var[i].ModifEnable = true;
			this->Var[i].Value = LuaToNumber(l, -1);
			this->Var[i].ModifValue = true;
			this->Var[i].Max = LuaToNumber(l, -1);
			this->Var[i].ModifMax = true;
		} else if (lua_istable(l, -1)) {
			for (lua_pushnil(l); lua_next(l, -2); lua_pop(l, 1)) {
				const char *const key = LuaToString(l, -2);
				if (!strcmp(key, "Enable")) {
					this->Var[i].Enable = LuaToBoolean(l, -1);
					this->Var[i].ModifEnable = true;
				} else if (!strcmp(key, "Value")) {
					this->Var[i].Value = LuaToNumber(l, -1);
					this->Var[i].ModifValue = true;
				} else if (!strcmp(key, "Max")) {
					this->Var[i].Max = LuaToNumber(l, -1);
					this->Var[i].ModifMax = true;
				} else if (!strcmp(key, "Increase")) {
					this->Var[i].Increase = LuaToNumber(l, -1);
					this->Var[i].ModifIncrease = true;
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
						this->Var[i].TargetIsCaster = true;
					} else if (!strcmp(value, "target")) {
						this->Var[i].TargetIsCaster = false;
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
int spell_action_adjust_variable::Cast(CUnit &caster, const spell &, CUnit *target, const Vec2i &/*goalPos*/, int /*z*/, int modifier)
{
	for (unsigned int i = 0; i < UnitTypeVar.GetNumberVariable(); ++i) {
		CUnit *unit = (this->Var[i].TargetIsCaster) ? &caster : target;

		if (!unit) {
			continue;
		}

		const int old_value = unit->Variable[i].Value;

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
//		unit->Variable[i].Value = std::clamp(unit->Variable[i].Value, 0, unit->Variable[i].Max);
		unit->Variable[i].Value = std::clamp(unit->Variable[i].Value, 0, unit->GetModifiedVariable(i, VariableAttribute::Max));
		//Wyrmgus end

		unit->on_variable_changed(i, unit->Variable[i].Value - old_value);
	}
	return 1;
}

}
