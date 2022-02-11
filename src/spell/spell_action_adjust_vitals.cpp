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
//      (c) Copyright 1998-2022 by Vladi Belperchinov-Shabanski, Lutz Sammer,
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

#include "spell/spell_action_adjust_vitals.h"

#include "script.h"
#include "spell/spell.h"
#include "unit/unit.h"

namespace wyrmgus {

void spell_action_adjust_vitals::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "hit_points") {
		this->HP = std::stoi(value);
	} else if (key == "mana") {
		this->Mana = std::stoi(value);
	} else if (key == "shield") {
		this->Shield = std::stoi(value);
	} else if (key == "max_multi_cast") {
		this->MaxMultiCast = std::stoi(value);
	} else {
		throw std::runtime_error("Invalid adjust vitals spell action property: \"" + key + "\".");
	}
}

void spell_action_adjust_vitals::Parse(lua_State *l, int startIndex, int endIndex)
{
	for (int j = startIndex; j < endIndex; ++j) {
		const char *value = LuaToString(l, -1, j + 1);
		++j;
		if (!strcmp(value, "hit-points")) {
			this->HP = LuaToNumber(l, -1, j + 1);
		} else if (!strcmp(value, "mana-points")) {
			this->Mana = LuaToNumber(l, -1, j + 1);
		} else if (!strcmp(value, "shield-points")) {
			this->Shield = LuaToNumber(l, -1, j + 1);
		} else if (!strcmp(value, "max-multi-cast")) {
			this->MaxMultiCast = LuaToNumber(l, -1, j + 1);
		} else {
			LuaError(l, "Unsupported adjust-vitals tag: %s" _C_ value);
		}
	}
}

/**
** Cast healing. (or exorcism)
**
**  @param caster       Unit that casts the spell
**  @param spell        Spell-type pointer
**  @param target       Target unit that spell is addressed to
**  @param goalPos      coord of target spot when/if target does not exist
**
**  @return             =!0 if spell should be repeated, 0 if not
*/
int spell_action_adjust_vitals::Cast(CUnit &caster, const spell &spell, CUnit *target, const Vec2i &/*goalPos*/, int /*z*/, int modifier)
{
	if (target == nullptr) {
		return 0;
	}

	const int hp = this->HP * modifier / 100;
	const int mana = this->Mana * modifier / 100;
	const int shield = this->Shield * modifier / 100;
	const int manacost = spell.get_mana_cost();
	int diffHP;
	int diffMana;
	int diffShield;

	//  Healing and harming
	if (hp > 0) {
		//Wyrmgus start
//		diffHP = target->Variable[HP_INDEX].Max - target->Variable[HP_INDEX].Value;
		diffHP = target->GetModifiedVariable(HP_INDEX, VariableAttribute::Max) - target->Variable[HP_INDEX].Value;
		//Wyrmgus end
	} else {
		diffHP = target->Variable[HP_INDEX].Value;
	}
	if (mana > 0) {
		//Wyrmgus start

//		diffMana = target->Stats->Variables[MANA_INDEX].Max - target->Variable[MANA_INDEX].Value;
		diffMana = target->GetModifiedVariable(MANA_INDEX, VariableAttribute::Max) - target->GetModifiedVariable(MANA_INDEX, VariableAttribute::Value);
		//Wyrmgus end
	} else {
		diffMana = target->Variable[MANA_INDEX].Value;
	}

	if (shield > 0) {
		diffShield = target->Stats->Variables[SHIELD_INDEX].Max - target->Variable[SHIELD_INDEX].Value;
	} else {
		diffShield = target->Variable[SHIELD_INDEX].Value;
	}

	//  When harming cast again to send the hp to negative values.
	//  Careful, a perfect 0 target hp kills too.
	//  Avoid div by 0 errors too!
	int castcount = 1;
	if (hp) {
		castcount = std::max<int>(castcount,
			diffHP / abs(hp) + (((hp < 0) && (diffHP % (-hp) > 0)) ? 1 : 0));
	}
	if (mana) {
		castcount = std::max<int>(castcount,
			diffMana / abs(mana) + (((mana < 0) && (diffMana % (-mana) > 0)) ? 1 : 0));
	}
	if (shield) {
		castcount = std::max<int>(castcount,
			diffShield / abs(shield) + (((shield < 0) && (diffShield % (-shield) > 0)) ? 1 : 0));
	}
	if (manacost) {
		castcount = std::min<int>(castcount, caster.Variable[MANA_INDEX].Value / manacost);
	}
	if (this->MaxMultiCast) {
		castcount = std::min<int>(castcount, this->MaxMultiCast);
	}

	caster.Variable[MANA_INDEX].Value -= castcount * manacost;
	if (hp < 0) {
		if (&caster != target) {
			HitUnit(&caster, *target, -(castcount * hp));
		} else {
			target->Variable[HP_INDEX].Value += castcount * hp;
			target->Variable[HP_INDEX].Value = std::max(target->Variable[HP_INDEX].Value, 0);
		}
	} else {
		target->Variable[HP_INDEX].Value += castcount * hp;
		//Wyrmgus start
//		target->Variable[HP_INDEX].Value = std::min(target->Variable[HP_INDEX].Max, target->Variable[HP_INDEX].Value);
		target->Variable[HP_INDEX].Value = std::min(target->GetModifiedVariable(HP_INDEX, VariableAttribute::Max), target->Variable[HP_INDEX].Value);
		//Wyrmgus end
	}
	target->Variable[MANA_INDEX].Value += castcount * mana;
	//Wyrmgus start
//	target->Variable[MANA_INDEX].Value = std::clamp(target->Variable[MANA_INDEX].Value, 0, target->Variable[MANA_INDEX].Max);
	target->Variable[MANA_INDEX].Value = std::clamp(target->Variable[MANA_INDEX].Value, 0, target->GetModifiedVariable(MANA_INDEX, VariableAttribute::Max));
	//Wyrmgus end
	target->Variable[SHIELD_INDEX].Value += castcount * shield;
	target->Variable[SHIELD_INDEX].Value = std::clamp(target->Variable[SHIELD_INDEX].Value, 0, target->Variable[SHIELD_INDEX].Max);

	if (spell.repeats_cast()) {
		return 1;
	}
	return 0;
}

}
