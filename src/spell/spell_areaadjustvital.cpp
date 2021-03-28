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
/**@name spell_areaadjustvital.cpp - The spell AreaAdjustVital. */
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

#include "spell/spell_areaadjustvital.h"

#include "script.h"
#include "spell/spell.h"
#include "unit/unit.h"
#include "unit/unit_find.h"

void Spell_AreaAdjustVital::Parse(lua_State *l, int startIndex, int endIndex)
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
		} else if (!strcmp(value, "range")) {
			this->Range = LuaToNumber(l, -1, j + 1);
		} else if (!strcmp(value, "use-mana")) {
			this->UseMana = LuaToBoolean(l, -1, j + 1);
		} else {
			LuaError(l, "Unsupported area-adjust-vitals tag: %s" _C_ value);
		}
	}
}

/**
** Cast Area Adjust Vital on all valid units in range.
**
**  @param caster       Unit that casts the spell
**  @param spell        Spell-type pointer
**  @param target       Target unit that spell is addressed to
**  @param goalPos      TilePos of target spot when/if target does not exist
**
**  @return             =!0 if spell should be repeated, 0 if not
*/
int Spell_AreaAdjustVital::Cast(CUnit &caster, const wyrmgus::spell &spell, CUnit *target, const Vec2i &goalPos, int z, int modifier)
{
	const Vec2i range(this->Range, this->Range);
	const Vec2i typeSize(caster.Type->get_tile_size());
	std::vector<CUnit *> units;

	// Get all the units around the unit
	//Wyrmgus start
//	Select(goalPos - range, goalPos + typeSize + range, units);
	Select(goalPos - range, goalPos + typeSize + range, units, z);
	//Wyrmgus end
	int hp = this->HP * modifier / 100;
	int mana = this->Mana * modifier / 100;
	int shield = this->Shield * modifier / 100;
	for (size_t j = 0; j != units.size(); ++j) {
		target = units[j];
		if (!CanCastSpell(caster, spell, target)) {
			continue;
		}
		if (hp < 0) {
			HitUnit(&caster, *target, -hp);
		} else {
			target->Variable[HP_INDEX].Value += hp;
			//Wyrmgus start
//			target->Variable[HP_INDEX].Value = std::min(target->Variable[HP_INDEX].Max, target->Variable[HP_INDEX].Value);
			target->Variable[HP_INDEX].Value = std::min(target->GetModifiedVariable(HP_INDEX, VariableAttribute::Max), target->Variable[HP_INDEX].Value);
			//Wyrmgus end
		}
		target->Variable[MANA_INDEX].Value += mana;
		//Wyrmgus start
//		target->Variable[MANA_INDEX].Value = std::clamp(target->Variable[MANA_INDEX].Value, 0, target->Variable[MANA_INDEX].Max);
		target->Variable[MANA_INDEX].Value = std::clamp(target->Variable[MANA_INDEX].Value, 0, target->GetModifiedVariable(MANA_INDEX, VariableAttribute::Max));
		//Wyrmgus end
		target->Variable[SHIELD_INDEX].Value += shield;
		target->Variable[SHIELD_INDEX].Value = std::clamp(target->Variable[SHIELD_INDEX].Value, 0, target->Variable[SHIELD_INDEX].Max);
	}
	if (UseMana) {
		caster.Variable[MANA_INDEX].Value -= spell.get_mana_cost();
	}
	return 0;
}
