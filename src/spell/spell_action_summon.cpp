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

#include "spell/spell_action_summon.h"

#include "action/action_defend.h"
#include "actions.h"
#include "ai/ai_local.h"
#include "commands.h"
#include "map/map_layer.h"
#include "player/player.h"
#include "script.h"
#include "spell/spell.h"
#include "unit/unit.h"
#include "unit/unit_find.h"
#include "util/string_conversion_util.h"

namespace wyrmgus {

void spell_action_summon::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "unit_type") {
		this->UnitType = unit_type::get(value);
	} else if (key == "time_to_live") {
		this->TTL = std::stoi(value);
	} else if (key == "require_corpse") {
		this->RequireCorpse = string::to_bool(value);
	} else if (key == "join_to_ai_force") {
		this->JoinToAiForce = string::to_bool(value);
	} else {
		throw std::runtime_error("Invalid adjust vitals spell action property: \"" + key + "\".");
	}
}

void spell_action_summon::check() const
{
	if (this->UnitType == nullptr) {
		throw std::runtime_error("Summon spell action has no unit type.");
	}
}

void spell_action_summon::Parse(lua_State *l, int startIndex, int endIndex)
{
	for (int j = startIndex; j < endIndex; ++j) {
		const char *value = LuaToString(l, -1, j + 1);
		++j;
		if (!strcmp(value, "unit-type")) {
			value = LuaToString(l, -1, j + 1);
			this->UnitType = unit_type::get(value);
		} else if (!strcmp(value, "time-to-live")) {
			this->TTL = LuaToNumber(l, -1, j + 1);
		} else if (!strcmp(value, "require-corpse")) {
			this->RequireCorpse = true;
			--j;
		} else if (!strcmp(value, "join-to-ai-force")) {
			this->JoinToAiForce = true;
			--j;
		} else {
			LuaError(l, "Unsupported summon tag: %s" _C_ value);
		}
	}
	// Now, checking value.
	if (this->UnitType == nullptr) {
		LuaError(l, "Use a unittype for summon (with unit-type)");
	}
}

class IsDyingAndNotABuilding final
{
public:
	bool operator()(const CUnit *unit) const
	{
		return unit->CurrentAction() == UnitAction::Die && !unit->Type->BoolFlag[BUILDING_INDEX].value;
	}
};

/**
**  Cast summon spell.
**
**  @param caster       Unit that casts the spell
**  @param spell        Spell-type pointer
**  @param target       Target unit that spell is addressed to
**  @param goalPos      coord of target spot when/if target does not exist
**
**  @return             =!0 if spell should be repeated, 0 if not
*/
int spell_action_summon::Cast(CUnit &caster, const spell &spell, CUnit *target, const Vec2i &goalPos, const int z, const int modifier)
{
	Vec2i pos = goalPos;
	bool cansummon;
	unit_type &unittype = *this->UnitType;
	const int ttl = this->TTL * modifier / 100;

	if (this->RequireCorpse) {
		const Vec2i offset(1, 1);
		const Vec2i minPos = pos - offset;
		const Vec2i maxPos = pos + offset;

		CUnit *unit = FindUnit_If(minPos, maxPos, z, IsDyingAndNotABuilding());
		cansummon = false;

		if (unit != nullptr) { //  Found a corpse. eliminate it and proceed to summoning.
			pos = unit->tilePos;
			unit->Remove(nullptr);
			unit->Release();
			cansummon = true;
		}
	} else {
		cansummon = true;
	}

	if (cansummon) {
		//Wyrmgus start
//		DebugPrint("Summoning a %s\n" _C_ unittype.Name.c_str());
		DebugPrint("Summoning a %s\n" _C_ unittype.GetDefaultName(caster.Player).c_str());
		//Wyrmgus end

		//
		// Create units.
		// FIXME: do summoned units count on food?
		//
		target = MakeUnit(unittype, caster.Player);
		if (target != nullptr) {
			target->tilePos = pos;
			target->MapLayer = CMap::get()->MapLayers[z].get();
			target->drop_out_on_side(LookingW, nullptr);
			// To avoid defending summoned unit for AI
			target->Summoned = 1;
			//
			//  set life span. ttl=0 results in a permanent unit.
			//
			if (ttl) {
				target->TTL = GameCycle + ttl;
			}

			// Insert summoned unit to AI force so it will help them in battle
			if (this->JoinToAiForce && caster.Player->AiEnabled) {
				int force = caster.Player->Ai->Force.GetForce(caster);
				if (force != -1) {
					caster.Player->Ai->Force[force].Insert(target);
					target->GroupId = caster.GroupId;
					CommandDefend(*target, caster, FlushCommands);
				}
			}

			caster.Variable[MANA_INDEX].Value -= spell.get_mana_cost();
		} else {
			DebugPrint("Unable to allocate Unit");
		}
		return 1;
	}
	return 0;
}

}
