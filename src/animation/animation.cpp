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
//      (c) Copyright 1998-2022 by Lutz Sammer, Russell Smith, Jimmy Salmon
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

#include "stratagus.h"

#include "action/action_spellcast.h"

#include "animation/animation.h"

#include "animation/animation_attack.h"
#include "animation/animation_die.h"
#include "animation/animation_exactframe.h"
#include "animation/animation_frame.h"
#include "animation/animation_goto.h"
#include "animation/animation_ifvar.h"
#include "animation/animation_label.h"
#include "animation/animation_move.h"
#include "animation/animation_randomgoto.h"
#include "animation/animation_randomrotate.h"
#include "animation/animation_randomsound.h"
#include "animation/animation_randomwait.h"
#include "animation/animation_rotate.h"
#include "animation/animation_setvar.h"
#include "animation/animation_sound.h"
#include "animation/animation_spawnmissile.h"
#include "animation/animation_unbreakable.h"
#include "animation/animation_wait.h"

#include "actions.h"
#include "animation/animation_sequence.h"
#include "animation/animation_set.h"
#include "config.h"
#include "player/player.h"
#include "script.h"
#include "spell/spell.h"
#include "unit/unit.h"
#include "unit/unit_type.h"
#include "util/assert_util.h"
#include "util/string_util.h"
#include "util/util.h"

std::vector<LabelsStruct> Labels;
std::vector<LabelsLaterStruct> LabelsLater;

/*----------------------------------------------------------------------------
--  Animation
----------------------------------------------------------------------------*/

/**
**  Show unit animation.
**
**  @param unit  Unit of the animation.
**  @param anim  Animation script to handle.
**
**  @return      The flags of the current script step.
*/
int UnitShowAnimation(CUnit &unit, const CAnimation *anim)
{
	return UnitShowAnimationScaled(unit, anim, 8);
}

/**
**  Parse player number in animation frame
**
**  @param unit      Unit of the animation.
**  @param parseint  Integer to parse.
**
**  @return  The parsed value.
*/
static int ParseAnimPlayer(const CUnit &unit, const std::string &parseint)
{
	if (parseint == "this") {
		return unit.Player->get_index();
	}
	return ParseAnimInt(unit, parseint);
}

/**
**  Parse integer in animation frame.
**
**  @param unit      Unit of the animation.
**  @param parseint  Integer to parse.
**
**  @return  The parsed value.
*/

int ParseAnimInt(const CUnit &unit, const std::string &parseint)
{
	const CUnit *goal = &unit;

	if (parseint.empty()) {
		return 0;
	}

	const std::vector<std::string> str_list = wyrmgus::string::split(parseint, '.');

	if (str_list.size() > 1) {
		const std::string &cur = str_list[1];

		if (parseint[0] == 'v' || parseint[0] == 't') { //unit variable detected
			if (parseint[0] == 't') {
				if (unit.CurrentOrder()->has_goal()) {
					goal = unit.CurrentOrder()->get_goal();
				} else {
					return 0;
				}
			}

			if (str_list.size() < 3) {
				throw std::runtime_error("Need also specify the variable for the \"" + cur + "\" tag.");
			}

			const std::string &next = str_list[2];

			const int index = UnitTypeVar.VariableNameLookup[cur]; //user variables
			if (index == -1) {
				if (cur == "ResourcesHeld") {
					return goal->ResourcesHeld;
				} else if (cur == "ResourceActive") {
					return goal->Resource.Active;
				} else if (cur == "InsideCount") {
					return static_cast<int>(goal->get_units_inside().size());
				} else if (cur == "_Distance") {
					return unit.MapDistanceTo(*goal);
				}
				throw std::runtime_error("Bad variable name \"" + cur + "\".");
			}
			if (next == "Value") {
				//Wyrmgus start
	//			return goal->Variable[index].Value;
				return goal->GetModifiedVariable(index, VariableAttribute::Value);
				//Wyrmgus end
			} else if (next == "Max") {
				//Wyrmgus start
	//			return goal->Variable[index].Max;
				return goal->GetModifiedVariable(index, VariableAttribute::Max);
				//Wyrmgus end
			} else if (next == "Increase") {
				//Wyrmgus start
	//			return goal->Variable[index].Increase;
				return goal->GetModifiedVariable(index, VariableAttribute::Increase);
				//Wyrmgus end
			} else if (next == "Enable") {
				return goal->Variable[index].Enable;
			} else if (next == "Percent") {
				//Wyrmgus start
	//			return goal->Variable[index].Value * 100 / goal->Variable[index].Max;
				return goal->GetModifiedVariable(index, VariableAttribute::Value) * 100 / goal->GetModifiedVariable(index, VariableAttribute::Max);
				//Wyrmgus end
			}
			return 0;
		} else if (parseint[0] == 'b' || parseint[0] == 'g') { //unit bool flag detected
			if (parseint[0] == 'g') {
				if (unit.CurrentOrder()->has_goal()) {
					goal = unit.CurrentOrder()->get_goal();
				} else {
					return 0;
				}
			}
			const int index = UnitTypeVar.BoolFlagNameLookup[cur];// User bool flags
			if (index == -1) {
				throw std::runtime_error("Bad bool-flag name \"" + cur + "\".");
			}
			return goal->Type->BoolFlag[index].value;
		} else if (parseint[0] == 's') { //spell type detected
			assert_throw(goal->CurrentAction() == UnitAction::SpellCast);
			const COrder_SpellCast &order = *static_cast<COrder_SpellCast *>(goal->CurrentOrder());
			const wyrmgus::spell &spell = order.GetSpell();
			if (spell.get_identifier() == cur) {
				return 1;
			}
			return 0;
		} else if (parseint[0] == 'S') { // check if autocast for this spell available
			const wyrmgus::spell *spell = wyrmgus::spell::get(cur);
			if (unit.is_autocast_spell(spell)) {
				return 1;
			}
			return 0;
		} else if (parseint[0] == 'r') { //random value
			if (str_list.size() >= 3) {
				const std::string &next = str_list[2];
				const int min = std::stoi(cur);
				return min + SyncRand(std::stoi(next) - min + 1);
			} else {
				return SyncRand(std::stoi(cur) + 1);
			}
		} else if (parseint[0] == 'l') { //player number
			return ParseAnimPlayer(unit, std::string(cur));
		}
	}

	//check if we are trying to parse a number
	assert_throw(isdigit(parseint[0]) || parseint[0] == '-');
	return std::stoi(parseint);
}

/**
**  Parse flags list in animation frame.
**
**  @param unit       Unit of the animation.
**  @param parseflag  Flag list to parse.
**
**  @return The parsed value.
*/
int ParseAnimFlags(const CUnit &unit, const char *parseflag)
{
	char s[100];
	int flags = 0;

	strcpy(s, parseflag);
	char *cur = s;
	char *next = s;
	while (next && *next) {
		next = strchr(cur, '.');
		if (next) {
			*next = '\0';
			++next;
		}
		if (unit.Anim.Anim->Type == AnimationSpawnMissile) {
			if (!strcmp(cur, "none")) {
				flags = SM_None;
				return flags;
			} else if (!strcmp(cur, "damage")) {
				flags |= SM_Damage;
			} else if (!strcmp(cur, "totarget")) {
				flags |= SM_ToTarget;
			} else if (!strcmp(cur, "pixel")) {
				flags |= SM_Pixel;
			} else if (!strcmp(cur, "reltarget")) {
				flags |= SM_RelTarget;
			} else if (!strcmp(cur, "ranged")) {
				flags |= SM_Ranged;
			}  else if (!strcmp(cur, "setdirection")) {
				flags |= SM_SetDirection;
			} else {
				throw std::runtime_error("Unknown animation flag: \"" + std::string(cur) + "\".");
			}
		}
		cur = next;
	}
	return flags;
}


/**
**  Show unit animation.
**
**  @param unit   Unit of the animation.
**  @param anim   Animation script to handle.
**  @param scale  Scaling factor of the wait times in animation (8 means no scaling).
**
**  @return       The flags of the current script step.
*/
int UnitShowAnimationScaled(CUnit &unit, const CAnimation *anim, int scale)
{
	// Changing animations
	if (anim && unit.Anim.CurrAnim != anim) {
		// Assert fails when transforming unit (upgrade-to).
		assert_log(!unit.Anim.Unbreakable || unit.Waiting);
		unit.Anim.Anim = unit.Anim.CurrAnim = anim;
		unit.Anim.Wait = 0;
	}

	// Currently waiting
	if (unit.Anim.Wait) {
		--unit.Anim.Wait;
		if (!unit.Anim.Wait) {
			// Advance to next frame
			unit.Anim.Anim = unit.Anim.Anim->get_next();
		}
		return 0;
	}
	int move = 0;
	while (!unit.Anim.Wait) {
		unit.Anim.Anim->Action(unit, move, scale);
		if (!unit.Anim.Wait) {
			// Advance to next frame
			unit.Anim.Anim = unit.Anim.Anim->get_next();
		}
	}

	--unit.Anim.Wait;
	if (!unit.Anim.Wait) {
		assert_throw(unit.Anim.Anim != nullptr);
		// Advance to next frame
		unit.Anim.Anim = unit.Anim.Anim->get_next();
		assert_throw(unit.Anim.Anim != nullptr);
	}
	return move;
}

/**
**  Add a label
*/
void AddLabel(CAnimation *anim, const std::string &name)
{
	LabelsStruct label;

	label.Anim = anim;
	label.Name = name;
	Labels.push_back(label);
}

/**
**  Find a label
*/
static CAnimation *FindLabel(const std::string &name)
{
	for (size_t i = 0; i < Labels.size(); ++i) {
		if (Labels[i].Name == name) {
			return Labels[i].Anim;
		}
	}

	throw std::runtime_error("Label not found: " + name);
}

/**
**  Find a label later
*/
void FindLabelLater(CAnimation **anim, const std::string &name)
{
	LabelsLaterStruct label;

	label.Anim = anim;
	label.Name = name;
	LabelsLater.push_back(label);
}

/**
**  Fix labels
*/
void FixLabels()
{
	for (size_t i = 0; i < LabelsLater.size(); ++i) {
		*LabelsLater[i].Anim = FindLabel(LabelsLater[i].Name);
	}
}

/**
**  Parse an animation frame
**
**  @param str  string formated as "animationType extraArgs"
*/
static std::unique_ptr<CAnimation> ParseAnimationFrame(lua_State *l, const char *str)
{
	const std::string all(str);
	const size_t len = all.size();
	size_t end = all.find(' ');
	const std::string op1(all, 0, end);
	size_t begin = std::min(len, all.find_first_not_of(' ', end));
	const std::string extraArg(all, begin);

	std::unique_ptr<CAnimation> anim;
	if (op1 == "frame") {
		anim = std::make_unique<CAnimation_Frame>();
	} else if (op1 == "exact-frame") {
		anim = std::make_unique<CAnimation_ExactFrame>();
	} else if (op1 == "wait") {
		anim = std::make_unique<CAnimation_Wait>();
	} else if (op1 == "random-wait") {
		anim = std::make_unique<CAnimation_RandomWait>();
	} else if (op1 == "sound") {
		anim = std::make_unique<CAnimation_Sound>();
	} else if (op1 == "random-sound") {
		anim = std::make_unique<CAnimation_RandomSound>();
	} else if (op1 == "attack") {
		anim = std::make_unique<CAnimation_Attack>();
	} else if (op1 == "spawn-missile") {
		anim = std::make_unique<CAnimation_SpawnMissile>();
	} else if (op1 == "if-var") {
		anim = std::make_unique<CAnimation_IfVar>();
	} else if (op1 == "set-var") {
		anim = std::make_unique<CAnimation_SetVar>();
	} else if (op1 == "die") {
		anim = std::make_unique<CAnimation_Die>();
	} else if (op1 == "rotate") {
		anim = std::make_unique<CAnimation_Rotate>();
	} else if (op1 == "random-rotate") {
		anim = std::make_unique<CAnimation_RandomRotate>();
	} else if (op1 == "move") {
		anim = std::make_unique<CAnimation_Move>();
	} else if (op1 == "unbreakable") {
		anim = std::make_unique<CAnimation_Unbreakable>();
	} else if (op1 == "label") {
		anim = std::make_unique<CAnimation_Label>();
		AddLabel(anim.get(), extraArg);
	} else if (op1 == "goto") {
		anim = std::make_unique<CAnimation_Goto>();
	} else if (op1 == "random-goto") {
		anim = std::make_unique<CAnimation_RandomGoto>();
	} else {
		LuaError(l, "Unknown animation: %s" _C_ op1.c_str());
	}
	anim->Init(extraArg.c_str(), l);
	return anim;
}

/**
**  Parse an animation
*/
static std::vector<std::unique_ptr<CAnimation>> ParseAnimation(lua_State *l, int idx)
{
	if (!lua_istable(l, idx)) {
		LuaError(l, "incorrect argument");
	}
	const int args = lua_rawlen(l, idx);

	if (args == 0) {
		return {};
	}
	Labels.clear();
	LabelsLater.clear();

	const char *str = LuaToString(l, idx, 1);

	std::vector<std::unique_ptr<CAnimation>> animations;

	animations.push_back(ParseAnimationFrame(l, str));
	CAnimation *first_anim = animations.back().get();

	CAnimation *prev = first_anim;
	for (int j = 1; j < args; ++j) {
		const char *secondary_str = LuaToString(l, idx, j + 1);
		std::unique_ptr<CAnimation> anim = ParseAnimationFrame(l, secondary_str);
		prev->set_next(anim.get());
		prev = anim.get();
		animations.push_back(std::move(anim));
	}
	prev->set_next(first_anim);
	FixLabels();

	return animations;
}

/**
**  Define a unit-type animation set.
**
**  @param l  Lua state.
*/
static int CclDefineAnimations(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument");
	}

	const char *name = LuaToString(l, 1);
	wyrmgus::animation_set *anims = wyrmgus::animation_set::get_or_add(name, nullptr);

	lua_pushnil(l);
	while (lua_next(l, 2)) {
		const char *value = LuaToString(l, -2);

		std::vector<std::unique_ptr<CAnimation>> animations = ParseAnimation(l, -1);
		animation_sequence *animation_sequence = animation_sequence::add(anims->get_identifier() + "_" + value, anims->get_module());
		animation_sequence->set_animations(std::move(animations));
		const CAnimation *first_anim = animation_sequence->get_first_animation();

		if (!strcmp(value, "Start")) {
			anims->Start = first_anim;
		} else if (!strncmp(value, "Still", 5)) {
			anims->Still = first_anim;
		} else if (!strncmp(value, "Death", 5)) {
			if (strlen(value) > 5) {
				const int death = ExtraDeathIndex(value + 6);
				if (death == ANIMATIONS_DEATHTYPES) {
					anims->Death[ANIMATIONS_DEATHTYPES] = first_anim;
				} else {
					anims->Death[death] = first_anim;
				}
			} else {
				anims->Death[ANIMATIONS_DEATHTYPES] = first_anim;
			}
		} else if (!strcmp(value, "Attack")) {
			anims->Attack = first_anim;
		} else if (!strcmp(value, "RangedAttack")) {
			anims->RangedAttack = first_anim;
		} else if (!strcmp(value, "SpellCast")) {
			anims->SpellCast = first_anim;
		} else if (!strcmp(value, "Move")) {
			anims->Move = first_anim;
		} else if (!strcmp(value, "Repair")) {
			anims->Repair = first_anim;
		} else if (!strcmp(value, "Train")) {
			anims->Train = first_anim;
		} else if (!strcmp(value, "Research")) {
			anims->Research = first_anim;
		} else if (!strcmp(value, "Upgrade")) {
			anims->Upgrade = first_anim;
		} else if (!strcmp(value, "Build")) {
			anims->Build = first_anim;
		} else if (!strncmp(value, "Harvest_", 8)) {
			const std::string resource_str = value + 8;
			const resource *res = resource::get(resource_str);
			anims->harvest_animations[res] = first_anim;
		} else {
			LuaError(l, "Unsupported animation: %s" _C_ value);
		}
		lua_pop(l, 1);
	}

	return 0;
}

void AnimationCclRegister()
{
	lua_register(Lua, "DefineAnimations", CclDefineAnimations);
}
