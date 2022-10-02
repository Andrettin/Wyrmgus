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
/**@name script_spell.cpp - The spell script functions.. */
//
//      (c) Copyright 1998-2022 by Joris Dauphin, Crestez Leonard and Andrettin
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

#include "spell/spell.h"

#include "luacallback.h"
#include "player/civilization.h"
#include "player/faction.h"
#include "script.h"
#include "sound/script_sound.h"
#include "sound/sound.h"
#include "spell/spell_action_adjust_variable.h"
#include "spell/spell_action_adjust_vitals.h"
#include "spell/spell_action_spawn_missile.h"
#include "spell/spell_action_summon.h"
#include "spell/spell_areaadjustvital.h"
#include "spell/spell_areabombardment.h"
#include "spell/spell_capture.h"
#include "spell/spell_demolish.h"
#include "spell/spell_polymorph.h"
//Wyrmgus start
#include "spell/spell_retrain.h"
//Wyrmgus end
#include "spell/spell_spawnportal.h"
#include "spell/spell_target_type.h"
#include "spell/spell_teleport.h"
#include "unit/unit_type.h"
#include "upgrade/upgrade.h"
#include "upgrade/upgrade_structs.h"

// **************************************************************************
// Action parsers for spellAction
// **************************************************************************

/**
**  Parse the action for spell.
**
**  @param l  Lua state.
*/
static std::unique_ptr<wyrmgus::spell_action> CclSpellAction(lua_State *l)
{
	if (!lua_istable(l, -1)) {
		LuaError(l, "incorrect argument");
	}
	const int args = lua_rawlen(l, -1);

	const char *value = LuaToString(l, -1, 1);

	std::unique_ptr<wyrmgus::spell_action> spellaction;
	if (!strcmp(value, "adjust-variable")) {
		spellaction = std::make_unique<wyrmgus::spell_action_adjust_variable>();
	} else if (!strcmp(value, "adjust-vitals")) {
		spellaction = std::make_unique<wyrmgus::spell_action_adjust_vitals>();
	} else if (!strcmp(value, "area-adjust-vitals")) {
		spellaction = std::make_unique<Spell_AreaAdjustVital>();
	} else if (!strcmp(value, "area-bombardment")) {
		spellaction = std::make_unique<Spell_AreaBombardment>();
	} else if (!strcmp(value, "capture")) {
		spellaction = std::make_unique<Spell_Capture>();
	} else if (!strcmp(value, "demolish")) {
		spellaction = std::make_unique<Spell_Demolish>();
	} else if (!strcmp(value, "polymorph")) {
		spellaction = std::make_unique<Spell_Polymorph>();
	//Wyrmgus start
	} else if (!strcmp(value, "retrain")) {
		spellaction = std::make_unique<Spell_Retrain>();
	//Wyrmgus end
	} else if (!strcmp(value, "spawn-missile")) {
		spellaction = std::make_unique<wyrmgus::spell_action_spawn_missile>();
	} else if (!strcmp(value, "spawn-portal")) {
		spellaction = std::make_unique<Spell_SpawnPortal>();
	} else if (!strcmp(value, "summon")) {
		spellaction = std::make_unique<wyrmgus::spell_action_summon>();
	} else if (!strcmp(value, "teleport")) {
		spellaction = std::make_unique<Spell_Teleport>();
	} else {
		LuaError(l, "Unsupported action type: %s" _C_ value);
	}
	spellaction->Parse(l, 1, args);
	return spellaction;
}

/**
**  Get a condition value from a scm object.
**
**  @param l      Lua state.
**  @param value  scm value to convert.
**
**  @return CONDITION_TRUE, CONDITION_FALSE, CONDITION_ONLY or -1 on error.
**  @note This is a helper function to make CclSpellCondition shorter
**        and easier to understand.
*/
char Ccl2Condition(lua_State *l, const char *value)
{
	if (!strcmp(value, "true")) {
		return CONDITION_TRUE;
	} else if (!strcmp(value, "false")) {
		return CONDITION_FALSE;
	} else if (!strcmp(value, "only")) {
		return CONDITION_ONLY;
	} else {
		LuaError(l, "Bad condition result: %s" _C_ value);
		return -1;
	}
}

/**
**  Parse the Condition for spell.
**
**  @param l          Lua state.
**  @param condition  pointer to condition to fill with data.
**
**  @note Conditions must be allocated. All data already in is LOST.
*/
static void CclSpellCondition(lua_State *l, ConditionInfo *condition)
{
	//  Now parse the list and set values.
	if (!lua_istable(l, -1)) {
		LuaError(l, "incorrect argument");
	}
	const int args = lua_rawlen(l, -1);
	for (int j = 0; j < args; ++j) {
		const char *value = LuaToString(l, -1, j + 1);
		++j;
		if (!strcmp(value, "alliance")) {
			condition->Alliance = Ccl2Condition(l, LuaToString(l, -1, j + 1));
		} else if (!strcmp(value, "opponent")) {
			condition->Opponent = Ccl2Condition(l, LuaToString(l, -1, j + 1));
		} else if (!strcmp(value, "self")) {
			condition->TargetSelf = Ccl2Condition(l, LuaToString(l, -1, j + 1));
		//Wyrmgus start
		} else if (!strcmp(value, "thrusting-weapon")) {
			condition->ThrustingWeapon = Ccl2Condition(l, LuaToString(l, -1, j + 1));
		} else if (!strcmp(value, "faction-unit")) {
			condition->FactionUnit = Ccl2Condition(l, LuaToString(l, -1, j + 1));
		} else if (!strcmp(value, "civilization-equivalent")) {
			value = LuaToString(l, -1, j + 1);
			const wyrmgus::civilization *civilization = wyrmgus::civilization::get(value);
			condition->civilization_equivalent = civilization;
		} else if (!strcmp(value, "faction-equivalent")) {
			value = LuaToString(l, -1, j + 1);
			wyrmgus::faction *faction = wyrmgus::faction::get(value);
			condition->FactionEquivalent = faction;
		//Wyrmgus end
		} else {
			int index = UnitTypeVar.BoolFlagNameLookup[value];
			if (index != -1) {
				condition->BoolFlag[index] = Ccl2Condition(l, LuaToString(l, -1, j + 1));
				continue;
			}
			index = UnitTypeVar.VariableNameLookup[value];
			if (index != -1) { // Valid index.
				lua_rawgeti(l, -1, j + 1);
				if (!lua_istable(l, -1)) {
					LuaError(l, "Table expected in variable in condition");
				}
				for (lua_pushnil(l); lua_next(l, -2); lua_pop(l, 1)) {
					const char *const key = LuaToString(l, -2);
					condition->Variable[index].Check = true;
					if (!strcmp(key, "Enable")) {
						condition->Variable[index].Enable = Ccl2Condition(l, LuaToString(l, -1));
					} else if (!strcmp(key, "ExactValue")) {
						condition->Variable[index].ExactValue = LuaToNumber(l, -1);
					} else if (!strcmp(key, "ExceptValue")) {
						condition->Variable[index].ExceptValue = LuaToNumber(l, -1);
					} else if (!strcmp(key, "MinValue")) {
						condition->Variable[index].MinValue = LuaToNumber(l, -1);
					} else if (!strcmp(key, "MaxValue")) {
						condition->Variable[index].MaxValue = LuaToNumber(l, -1);
					} else if (!strcmp(key, "MinMax")) {
						condition->Variable[index].MinMax = LuaToNumber(l, -1);
					} else if (!strcmp(key, "MinValuePercent")) {
						condition->Variable[index].MinValuePercent = LuaToNumber(l, -1);
					} else if (!strcmp(key, "MaxValuePercent")) {
						condition->Variable[index].MaxValuePercent = LuaToNumber(l, -1);
					} else if (!strcmp(key, "ConditionApplyOnCaster")) {
						condition->Variable[index].ConditionApplyOnCaster = LuaToBoolean(l, -1);
					} else { // Error
						LuaError(l, "%s invalid for Variable in condition" _C_ key);
					}
				}
				lua_pop(l, 1); // lua_rawgeti()
				continue;
			}
			LuaError(l, "Unsuported condition tag: %s" _C_ value);
		}
	}
}

/**
**  Parse the Condition for spell.
**
**  @param l         Lua state.
**  @param autocast  pointer to autocast to fill with data.
**
**  @note: autocast must be allocated. All data already in is LOST.
*/
void CclSpellAutocast(lua_State *l, AutoCastInfo *autocast)
{
	if (!lua_istable(l, -1)) {
		LuaError(l, "incorrect argument");
	}
	const int args = lua_rawlen(l, -1);
	for (int j = 0; j < args; ++j) {
		const char *value = LuaToString(l, -1, j + 1);
		++j;
		if (!strcmp(value, "range")) {
			autocast->Range = LuaToNumber(l, -1, j + 1);
		} else if (!strcmp(value, "min-range")) {
			autocast->MinRange = LuaToNumber(l, -1, j + 1);
		} else if (!strcmp(value, "position-autocast")) {
			lua_rawgeti(l, -1, j + 1);
			autocast->PositionAutoCast = std::make_unique<LuaCallback>(l, -1);
			lua_pop(l, 1);
		} else if (!strcmp(value, "combat")) {
			autocast->Combat = Ccl2Condition(l, LuaToString(l, -1, j + 1));
		} else if (!strcmp(value, "attacker")) {
			autocast->Attacker = Ccl2Condition(l, LuaToString(l, -1, j + 1));
		} else if (!strcmp(value, "corpse")) {
			autocast->Corpse = Ccl2Condition(l, LuaToString(l, -1, j + 1));
		} else if (!strcmp(value, "priority")) {
			lua_rawgeti(l, -1, j + 1);
			if (!lua_istable(l, -1) || lua_rawlen(l, -1) != 2) {
				LuaError(l, "incorrect argument");
			}
			lua_rawgeti(l, -1, 1);
			std::string var = LuaToString(l, -1);
			int index = UnitTypeVar.VariableNameLookup[var.c_str()];// User variables
			if (index == -1) {
				if (!strcmp(var.c_str(), "Distance")) {
					index = ACP_DISTANCE;
				} else {
					throw std::runtime_error("Bad variable name \"" + var + "\".");
				}
			}
			autocast->PriorityVar = index;
			lua_pop(l, 1);
			autocast->ReverseSort = LuaToBoolean(l, -1, 2);

			lua_pop(l, 1);
		} else if (!strcmp(value, "condition")) {
			if (!autocast->cast_conditions) {
				autocast->cast_conditions = std::make_unique<ConditionInfo>();
			}
			lua_rawgeti(l, -1, j + 1);
			CclSpellCondition(l, autocast->cast_conditions.get());
			lua_pop(l, 1);
		} else {
			LuaError(l, "Unsupported autocast tag: %s" _C_ value);
		}
	}
}

/**
**  Parse Spell.
**
**  @param l  Lua state.
*/
int CclDefineSpell(lua_State *l)
{
	const int args = lua_gettop(l);
	const std::string identname = LuaToString(l, 1);
	wyrmgus::spell *spell = wyrmgus::spell::get_or_add(identname, nullptr);

	for (int i = 1; i < args; ++i) {
		const char *value = LuaToString(l, i + 1);
		++i;
		if (!strcmp(value, "showname")) {
			spell->set_name(LuaToString(l, i + 1));
		} else if (!strcmp(value, "description")) {
			spell->effects_string = LuaToString(l, i + 1);
		} else if (!strcmp(value, "manacost")) {
			spell->mana_cost = LuaToNumber(l, i + 1);
		} else if (!strcmp(value, "cooldown")) {
			spell->cooldown = LuaToNumber(l, i + 1);
		} else if (!strcmp(value, "res-cost")) {
			lua_pushvalue(l, i + 1);
			if (!lua_istable(l, -1)) {
				LuaError(l, "incorrect argument");
			}
			const int len = lua_rawlen(l, -1);
			for (int j = 0; j < len; ++j) {
				const resource *resource = resource::get(LuaToString(l, -1, j + 1));
				++j;
				spell->costs[resource] = LuaToNumber(l, -1, j + 1);
			}
			lua_pop(l, 1);
		} else if (!strcmp(value, "range")) {
			if (!lua_isstring(l, i + 1) && !lua_isnumber(l, i + 1)) {
				LuaError(l, "incorrect argument");
			}
			if (lua_isstring(l, i + 1) && !strcmp(lua_tostring(l, i + 1), "infinite")) {
				spell->range = wyrmgus::spell::infinite_range;
			} else if (lua_isnumber(l, i + 1)) {
				spell->range = static_cast<int>(lua_tonumber(l, i + 1));
			} else {
				LuaError(l, "Invalid range");
			}
		} else if (!strcmp(value, "repeat-cast")) {
			spell->repeat_cast = true;
			--i;
		} else if (!strcmp(value, "stackable")) {
			spell->stackable = LuaToBoolean(l, i + 1);
		} else if (!strcmp(value, "force-use-animation")) {
			spell->force_use_animation = true;
			--i;
		} else if (!strcmp(value, "target")) {
			value = LuaToString(l, i + 1);
			spell->target = enum_converter<spell_target_type>::to_enum(value);
		} else if (!strcmp(value, "action")) {
			if (!lua_istable(l, i + 1)) {
				LuaError(l, "incorrect argument");
			}
			const int subargs = lua_rawlen(l, i + 1);
			for (int k = 0; k < subargs; ++k) {
				lua_rawgeti(l, i + 1, k + 1);
				spell->actions.push_back(CclSpellAction(l));
				lua_pop(l, 1);
			}
		} else if (!strcmp(value, "condition")) {
			if (!spell->cast_conditions) {
				spell->cast_conditions = std::make_unique<ConditionInfo>();
			}
			lua_pushvalue(l, i + 1);
			CclSpellCondition(l, spell->cast_conditions.get());
			lua_pop(l, 1);
		} else if (!strcmp(value, "autocast")) {
			if (!spell->autocast) {
				spell->autocast = std::make_unique<AutoCastInfo>();
			}
			lua_pushvalue(l, i + 1);
			CclSpellAutocast(l, spell->autocast.get());
			lua_pop(l, 1);
		} else if (!strcmp(value, "ai-cast")) {
			if (!spell->ai_cast) {
				spell->ai_cast = std::make_unique<AutoCastInfo>();
			}
			lua_pushvalue(l, i + 1);
			CclSpellAutocast(l, spell->ai_cast.get());
			lua_pop(l, 1);
		} else if (!strcmp(value, "sound-when-cast")) {
			spell->sound_when_cast = wyrmgus::sound::get(LuaToString(l, i + 1));
		} else if (!strcmp(value, "depend-upgrade")) {
			value = LuaToString(l, i + 1);
			spell->dependency_upgrade = CUpgrade::get(value);
		//Wyrmgus start
		} else if (!strcmp(value, "item-spell")) {
			const int item_class = static_cast<int>(enum_converter<wyrmgus::item_class>::to_enum(LuaToString(l, i + 1)));
			spell->ItemSpell[item_class] = true;
		//Wyrmgus end
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
	return 0;
}

static int CclGetSpellData(lua_State *l)
{
	if (lua_gettop(l) < 2) {
		LuaError(l, "incorrect argument");
	}

	const std::string identifier = LuaToString(l, 1);
	const wyrmgus::spell *spell = wyrmgus::spell::get(identifier);

	const char *data = LuaToString(l, 2);

	if (!strcmp(data, "Name")) {
		lua_pushstring(l, spell->get_name().c_str());
		return 1;
	} else {
		LuaError(l, "Invalid field: %s" _C_ data);
	}

	return 0;
}

/**
** Register CCL features for Spell.
*/
void SpellCclRegister()
{
	lua_register(Lua, "DefineSpell", CclDefineSpell);
	lua_register(Lua, "GetSpellData", CclGetSpellData);
}
