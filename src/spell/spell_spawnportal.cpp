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
/**@name spell_spawnportal.cpp - The spell SpawnPortal. */
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

#include "stratagus.h"

#include "spell/spell_spawnportal.h"

#include "player/player.h"
#include "script.h"
#include "unit/unit.h"

void Spell_SpawnPortal::Parse(lua_State *l, int startIndex, int endIndex)
{
	for (int j = startIndex; j < endIndex; ++j) {
		const char *value = LuaToString(l, -1, j + 1);
		++j;
		if (!strcmp(value, "portal-type")) {
			value = LuaToString(l, -1, j + 1);
			this->PortalType = wyrmgus::unit_type::get(value);
		} else if (!strcmp(value, "time-to-live")) {
			this->TTL = LuaToNumber(l, -1, j + 1);
		} else if (!strcmp(value, "current-player")) {
			this->CurrentPlayer = true;
			--j;
		} else {
			LuaError(l, "Unsupported spawn-portal tag: %s" _C_ value);
		}
	}
	// Now, checking value.
	if (this->PortalType == nullptr) {
		LuaError(l, "Use a unittype for spawn-portal (with portal-type)");
	}
}

/**
** Cast circle of power.
**
**  @param caster       Unit that casts the spell
**  @param spell        Spell-type pointer
**  @param target       Target unit that spell is addressed to
**  @param goalPos      tilePos of target spot when/if target does not exist
**
**  @return             =!0 if spell should be repeated, 0 if not
*/
int Spell_SpawnPortal::Cast(CUnit &caster, const wyrmgus::spell &, CUnit *, const Vec2i &goalPos, int z, int modifier)
{
	// FIXME: vladi: cop should be placed only on explored land
	CUnit *portal = caster.Goal;

	const int ttl = this->TTL * modifier / 100;

	DebugPrint("Spawning a portal exit.\n");
	if (portal && portal->IsAlive()) {
		//Wyrmgus start
//		portal->MoveToXY(goalPos);
		portal->MoveToXY(goalPos, z);
		//Wyrmgus end
	} else {
		portal = MakeUnitAndPlace(goalPos, *this->PortalType,
								  //Wyrmgus start
//								  CurrentPlayer ? caster.Player : CPlayer::get_neutral_player());
								  CurrentPlayer ? caster.Player : CPlayer::get_neutral_player(), z);
								  //Wyrmgus end
		portal->Summoned = 1;
	}
	portal->TTL = GameCycle + ttl;
	//  Goal is used to link to destination circle of power
	caster.Goal = portal;
	//FIXME: setting destination circle of power should use mana
	return 0;
}
