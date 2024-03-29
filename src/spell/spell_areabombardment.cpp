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
/**@name spell_areabombardment.cpp - The spell AreaBombardment. */
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

#include "spell/spell_areabombardment.h"

#include "database/defines.h"
#include "map/map.h"
#include "map/map_info.h"
#include "missile.h"
#include "script.h"
#include "unit/unit.h"
#include "util/util.h"
#include "video/video.h"

void Spell_AreaBombardment::Parse(lua_State *l, int startIndex, int endIndex)
{
	for (int j = startIndex; j < endIndex; ++j) {
		const char *value = LuaToString(l, -1, j + 1);
		++j;
		if (!strcmp(value, "fields")) {
			this->Fields = LuaToNumber(l, -1, j + 1);
		} else if (!strcmp(value, "shards")) {
			this->Shards = LuaToNumber(l, -1, j + 1);
		} else if (!strcmp(value, "damage")) {
			this->Damage = LuaToNumber(l, -1, j + 1);
		} else if (!strcmp(value, "start-offset-x")) {
			this->StartOffsetX = LuaToNumber(l, -1, j + 1);
		} else if (!strcmp(value, "start-offset-y")) {
			this->StartOffsetY = LuaToNumber(l, -1, j + 1);
		} else if (!strcmp(value, "missile")) {
			value = LuaToString(l, -1, j + 1);
			this->Missile = wyrmgus::missile_type::get(value);
		} else {
			LuaError(l, "Unsupported area-bombardment tag: %s" _C_ value);
		}
	}
	// Now, checking value.
	if (this->Missile == nullptr) {
		LuaError(l, "Use a missile for area-bombardment (with missile)");
	}
}


/**
** Cast area bombardment.
**
**  @param caster       Unit that casts the spell
**  @param spell        Spell-type pointer
**  @param target       Target unit that spell is addressed to
**  @param goalPos      TilePos of target spot when/if target does not exist
**
**  @return             =!0 if spell should be repeated, 0 if not
**  @internal: vladi: blizzard differs than original in this way:
**   original: launches 50 shards at 5 random spots x 10 for 25 mana.
*/
int Spell_AreaBombardment::Cast(CUnit &caster, const wyrmgus::spell &, CUnit *, const Vec2i &goalPos, int z, int modifier)
{
	int fields = this->Fields;
	const int shards = this->Shards;
	const int damage = this->Damage * modifier / 100;
	const PixelDiff offset(this->StartOffsetX, this->StartOffsetY);
	const wyrmgus::missile_type *missile = this->Missile;

	while (fields--) {
		Vec2i dpos;

		// FIXME: radius configurable...
		do {
			// find new destination in the map
			dpos.x = goalPos.x + SyncRand(5) - 2;
			dpos.y = goalPos.y + SyncRand(5) - 2;
		} while (!CMap::get()->Info->IsPointOnMap(dpos, z));

		const PixelPos dest = CMap::get()->tile_pos_to_map_pixel_pos_center(dpos);
		const PixelPos start = dest + offset;
		for (int i = 0; i < shards; ++i) {
			::Missile *mis = MakeMissile(*missile, start, dest, z);
			if (mis->Type->BlizzardSpeed) {
				mis->Delay = i * mis->Type->get_sleep() * 2 * wyrmgus::defines::get()->get_tile_width() / mis->Type->BlizzardSpeed;
			} else if (mis->Type->get_speed()) {
				mis->Delay = i * mis->Type->get_sleep() * 2 * wyrmgus::defines::get()->get_tile_width() / mis->Type->get_speed();
			} else {
				mis->Delay = i * mis->Type->get_sleep() * mis->Type->G->NumFrames;
			}
			mis->Damage = damage;
			// FIXME: not correct -- blizzard should continue even if mage is
			//    destroyed (though it will be quite short time...)
			mis->SourceUnit = caster.acquire_ref();
		}
	}
	return 1;
}
