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
/**@name spell_summon.cpp - The spell Summon. */
//
//      (c) Copyright 1998-2012 by Vladi Belperchinov-Shabanski, Lutz Sammer,
//                                 Jimmy Salmon, and Joris DAUPHIN
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

#include "action/action_defend.h"

#include "spell/spell_summon.h"

#include "action/actions.h"
#include "ai/ai_local.h"
#include "commands.h"
#include "map/map_layer.h"
#include "script.h"
#include "unit/unit.h"
#include "unit/unit_find.h"

/* virtual */ void Spell_Summon::Parse(lua_State *l, int startIndex, int endIndex)
{
	for (int j = startIndex; j < endIndex; ++j) {
		const char *value = LuaToString(l, -1, j + 1);
		++j;
		if (!strcmp(value, "unit-type")) {
			value = LuaToString(l, -1, j + 1);
			this->UnitType = CUnitType::Get(value);
			if (!this->UnitType) {
				this->UnitType = 0;
				DebugPrint("unit type \"%s\" not found for summon spell.\n" _C_ value);
			}
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

class IsDyingAndNotABuilding
{
public:
	bool operator()(const CUnit *unit) const
	{
		return unit->CurrentAction() == UnitActionDie && !unit->GetType()->BoolFlag[BUILDING_INDEX].value;
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
/* virtual */ int Spell_Summon::Cast(CUnit &caster, const CSpell &spell, CUnit *target, const Vec2i &goalPos, int z, int modifier)
{
	Vec2i pos = goalPos;
	bool cansummon;
	CUnitType &unit_type = *this->UnitType;
	int ttl = this->TTL;

	if (this->RequireCorpse) {
		const Vec2i offset(1, 1);
		const Vec2i minPos = pos - offset;
		const Vec2i maxPos = pos + offset;

		CUnit *unit = FindUnit_If(minPos, maxPos, z, IsDyingAndNotABuilding());
		cansummon = false;

		if (unit != nullptr) { //  Found a corpse. eliminate it and proceed to summoning.
			pos = unit->GetTilePos();
			z = unit->GetMapLayer()->GetIndex();
			unit->Remove(nullptr);
			unit->Release();
			cansummon = true;
		}
	} else {
		cansummon = true;
	}

	if (cansummon) {
		//Wyrmgus start
//		DebugPrint("Summoning a %s\n" _C_ unit_type.Name.c_str());
		DebugPrint("Summoning a %s\n" _C_ unit_type.GetDefaultName(caster.GetPlayer()).c_str());
		//Wyrmgus end

		//
		// Create units.
		// FIXME: do summoned units count on food?
		//
		target = MakeUnit(unit_type, caster.GetPlayer());
		if (target != nullptr) {
			target->TilePos = pos;
			target->MapLayer = CMap::Map.MapLayers[z];
			DropOutOnSide(*target, LookingW, nullptr);
			// To avoid defending summoned unit for AI
			target->Summoned = 1;
			//
			//  set life span. ttl=0 results in a permanent unit.
			//
			if (ttl) {
				target->TTL = GameCycle + ttl;
			}

			// Insert summoned unit to AI force so it will help them in battle
			if (this->JoinToAiForce && caster.GetPlayer()->AiEnabled) {
				int force = caster.GetPlayer()->Ai->Force.GetForce(caster);
				if (force != -1) {
					caster.GetPlayer()->Ai->Force[force].Insert(*target);
					target->GroupId = caster.GroupId;
					CommandDefend(*target, caster, FlushCommands);
				}
			}

			caster.Variable[MANA_INDEX].Value -= spell.ManaCost;
		} else {
			DebugPrint("Unable to allocate Unit");
		}
		return 1;
	}
	return 0;
}
