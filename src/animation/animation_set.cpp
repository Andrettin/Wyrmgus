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

#include "animation/animation_set.h"

#include "animation/animation.h"
#include "animation/animation_sequence.h"
#include "economy/resource.h"
#include "iolib.h"
#include "script.h"
#include "unit/unit.h"
#include "util/assert_util.h"

static int GetAdvanceIndex(const CAnimation *base, const CAnimation *anim)
{
	if (base == anim) {
		return 0;
	}
	int i = 1;
	for (const CAnimation *it = base->get_next(); it != base; it = it->get_next()) {
		if (it == anim) {
			return i;
		}
		++i;
	}
	return -1;
}

static const CAnimation *Advance(const CAnimation *anim, int n)
{
	for (int i = 0; i < n; ++i) {
		anim = anim->get_next();
	}
	return anim;
}

namespace wyrmgus {

const std::set<std::string> animation_set::database_dependencies = { animation_sequence::class_identifier };

void animation_set::clear()
{
	CAnimation::animation_list.clear();
	data_type::clear();
}

void animation_set::SaveUnitAnim(CFile &file, const CUnit &unit)
{
	file.printf("\"anim-data\", {");
	file.printf("\"anim-wait\", %d,", unit.Anim.Wait);
	for (size_t i = 0; i < CAnimation::animation_list.size(); ++i) {
		if (CAnimation::animation_list[i] == unit.Anim.CurrAnim) {
			file.printf("\"curr-anim\", %zu,", i);
			file.printf("\"anim\", %d,", GetAdvanceIndex(unit.Anim.CurrAnim, unit.Anim.Anim));
			break;
		}
	}
	if (unit.Anim.Unbreakable) {
		file.printf(" \"unbreakable\",");
	}
	file.printf("}, ");
	// Wait backup info
	file.printf("\"wait-anim-data\", {");
	file.printf("\"anim-wait\", %d,", unit.WaitBackup.Wait);
	for (size_t i = 0; i < CAnimation::animation_list.size(); ++i) {
		if (CAnimation::animation_list[i] == unit.WaitBackup.CurrAnim) {
			file.printf("\"curr-anim\", %zu,", i);
			file.printf("\"anim\", %d,", GetAdvanceIndex(unit.WaitBackup.CurrAnim, unit.WaitBackup.Anim));
			break;
		}
	}
	if (unit.WaitBackup.Unbreakable) {
		file.printf(" \"unbreakable\",");
	}
	file.printf("}");
}

void animation_set::LoadUnitAnim(lua_State *l, CUnit &unit, int luaIndex)
{
	if (!lua_istable(l, luaIndex)) {
		LuaError(l, "incorrect argument");
	}
	const int nargs = lua_rawlen(l, luaIndex);

	for (int j = 0; j != nargs; ++j) {
		const char *value = LuaToString(l, luaIndex, j + 1);
		++j;

		if (!strcmp(value, "anim-wait")) {
			unit.Anim.Wait = LuaToNumber(l, luaIndex, j + 1);
		} else if (!strcmp(value, "curr-anim")) {
			const int animIndex = LuaToNumber(l, luaIndex, j + 1);
			unit.Anim.CurrAnim = CAnimation::animation_list.at(animIndex);
		} else if (!strcmp(value, "anim")) {
			const int animIndex = LuaToNumber(l, luaIndex, j + 1);
			unit.Anim.Anim = Advance(unit.Anim.CurrAnim, animIndex);
		} else if (!strcmp(value, "unbreakable")) {
			unit.Anim.Unbreakable = true;
			--j;
		} else {
			LuaError(l, "Unit anim-data: Unsupported tag: %s" _C_ value);
		}
	}
}

void animation_set::LoadWaitUnitAnim(lua_State *l, CUnit &unit, int luaIndex)
{
	if (!lua_istable(l, luaIndex)) {
		LuaError(l, "incorrect argument");
	}
	const int nargs = lua_rawlen(l, luaIndex);

	for (int j = 0; j != nargs; ++j) {
		const char *value = LuaToString(l, luaIndex, j + 1);
		++j;

		if (!strcmp(value, "anim-wait")) {
			unit.WaitBackup.Wait = LuaToNumber(l, luaIndex, j + 1);
		} else if (!strcmp(value, "curr-anim")) {
			const int animIndex = LuaToNumber(l, luaIndex, j + 1);
			unit.WaitBackup.CurrAnim = CAnimation::animation_list.at(animIndex);
		} else if (!strcmp(value, "anim")) {
			const int animIndex = LuaToNumber(l, luaIndex, j + 1);
			unit.WaitBackup.Anim = Advance(unit.WaitBackup.CurrAnim, animIndex);
		} else if (!strcmp(value, "unbreakable")) {
			unit.WaitBackup.Unbreakable = true;
			--j;
		} else {
			LuaError(l, "Unit anim-data: Unsupported tag: %s" _C_ value);
		}
	}
}

void animation_set::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (
		key == "start"
		|| key == "still"
		|| key.starts_with("death")
		|| key == "attack"
		|| key == "ranged_attack"
		|| key == "spell_cast"
		|| key == "move"
		|| key == "repair"
		|| key == "train"
		|| key == "research"
		|| key == "upgrade"
		|| key == "build"
		|| key.starts_with("harvest")
	) {
		const animation_sequence *animation_sequence = animation_sequence::get(value);
		this->set_animations(key, animation_sequence);
	} else {
		data_entry::process_gsml_property(property);
	}
}

void animation_set::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (
		tag == "start"
		|| tag == "still"
		|| tag.starts_with("death")
		|| tag == "attack"
		|| tag == "ranged_attack"
		|| tag == "spell_cast"
		|| tag == "move"
		|| tag == "repair"
		|| tag == "train"
		|| tag == "research"
		|| tag == "upgrade"
		|| tag == "build"
		|| tag.starts_with("harvest")
	) {
		animation_sequence *animation_sequence = animation_sequence::add(this->get_identifier() + "_" + tag, this->get_module());
		database::process_gsml_data(animation_sequence, scope);

		this->set_animations(tag, animation_sequence);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void animation_set::set_animations(const std::string &animation_type, const animation_sequence *animation_sequence)
{
	const CAnimation *first_anim = animation_sequence->get_first_animation();

	if (animation_type == "start") {
		this->Start = first_anim;
	} else if (animation_type == "still") {
		this->Still = first_anim;
	} else if (animation_type.starts_with("death")) {
		const size_t find_pos = animation_type.find("_");
		if (find_pos != std::string::npos) {
			const std::string death_type = animation_type.substr(find_pos + 1, animation_type.size() - find_pos - 1);

			const int death_index = ExtraDeathIndex(death_type.c_str());
			if (death_index == ANIMATIONS_DEATHTYPES) {
				this->Death[ANIMATIONS_DEATHTYPES] = first_anim;
			} else {
				this->Death[death_index] = first_anim;
			}
		} else {
			this->Death[ANIMATIONS_DEATHTYPES] = first_anim;
		}
	} else if (animation_type == "attack") {
		this->Attack = first_anim;
	} else if (animation_type == "ranged_attack") {
		this->RangedAttack = first_anim;
	} else if (animation_type == "spell_cast") {
		this->SpellCast = first_anim;
	} else if (animation_type == "move") {
		this->Move = first_anim;
	} else if (animation_type == "repair") {
		this->Repair = first_anim;
	} else if (animation_type == "train") {
		this->Train = first_anim;
	} else if (animation_type == "research") {
		this->Research = first_anim;
	} else if (animation_type == "upgrade") {
		this->Upgrade = first_anim;
	} else if (animation_type == "build") {
		this->Build = first_anim;
	} else if (animation_type.starts_with("harvest")) {
		const size_t find_pos = animation_type.find("_");
		assert_throw(find_pos != std::string::npos);

		const std::string resource_identifier = animation_type.substr(find_pos + 1, animation_type.size() - find_pos - 1);
		const resource *resource = resource::get(resource_identifier);

		this->harvest_animations[resource] = first_anim;
	} else {
		throw std::runtime_error("Invalid animation type: \"" + animation_type + "\".");
	}
}

}
