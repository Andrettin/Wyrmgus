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
/**@name spell_demolish.cpp - The spell demolish. */
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

#include "spell/spell_demolish.h"

#include "map/map.h"
#include "map/tile.h"
#include "script.h"
#include "unit/unit.h"
#include "unit/unit_find.h"
#include "unit/unit_type_type.h"
#include "util/util.h"

void Spell_Demolish::Parse(lua_State *l, int startIndex, int endIndex)
{
	for (int j = startIndex; j < endIndex; ++j) {
		const char *value = LuaToString(l, -1, j + 1);
		++j;
		if (!strcmp(value, "range")) {
			this->Range = LuaToNumber(l, -1, j + 1);
		} else if (!strcmp(value, "damage")) {
			this->Damage = LuaToNumber(l, -1, j + 1);
		//Wyrmgus start
		} else if (!strcmp(value, "basic-damage")) {
			this->BasicDamage = LuaToNumber(l, -1, j + 1);
		} else if (!strcmp(value, "piercing-damage")) {
			this->PiercingDamage = LuaToNumber(l, -1, j + 1);
		} else if (!strcmp(value, "fire-damage")) {
			this->FireDamage = LuaToNumber(l, -1, j + 1);
		} else if (!strcmp(value, "cold-damage")) {
			this->ColdDamage = LuaToNumber(l, -1, j + 1);
		} else if (!strcmp(value, "arcane-damage")) {
			this->ArcaneDamage = LuaToNumber(l, -1, j + 1);
		} else if (!strcmp(value, "lightning-damage")) {
			this->LightningDamage = LuaToNumber(l, -1, j + 1);
		} else if (!strcmp(value, "air-damage")) {
			this->AirDamage = LuaToNumber(l, -1, j + 1);
		} else if (!strcmp(value, "earth-damage")) {
			this->EarthDamage = LuaToNumber(l, -1, j + 1);
		} else if (!strcmp(value, "water-damage")) {
			this->WaterDamage = LuaToNumber(l, -1, j + 1);
		} else if (!strcmp(value, "acid-damage")) {
			this->AcidDamage = LuaToNumber(l, -1, j + 1);
		} else if (!strcmp(value, "shadow-damage")) {
			this->ShadowDamage = LuaToNumber(l, -1, j + 1);
		} else if (!strcmp(value, "hack-damage")) {
			this->HackDamage = LuaToBoolean(l, -1, j + 1);
		} else if (!strcmp(value, "pierce-damage")) {
			this->PierceDamage = LuaToBoolean(l, -1, j + 1);
		} else if (!strcmp(value, "blunt-damage")) {
			this->BluntDamage = LuaToBoolean(l, -1, j + 1);
		} else if (!strcmp(value, "damage-self")) {
			this->DamageSelf = LuaToBoolean(l, -1, j + 1);
		} else if (!strcmp(value, "damage-friendly")) {
			this->DamageFriendly = LuaToBoolean(l, -1, j + 1);
		} else if (!strcmp(value, "damage-terrain")) {
			this->DamageTerrain = LuaToBoolean(l, -1, j + 1);
		//Wyrmgus end
		} else {
			LuaError(l, "Unsupported demolish tag: %s" _C_ value);
		}
	}
}

/**
**  Cast demolish
**  @param caster       Unit that casts the spell
**  @param spell        Spell-type pointer
**  @param target       Target unit that spell is addressed to
**  @param goalPos      tilePos of target spot when/if target does not exist
**
**  @return             =!0 if spell should be repeated, 0 if not
*/
int Spell_Demolish::Cast(CUnit &caster, const wyrmgus::spell &, CUnit *, const Vec2i &goalPos, int z, int modifier)
{
	Q_UNUSED(goalPos)
	Q_UNUSED(z)

	// Allow error margins. (Lame, I know)
	const Vec2i offset(this->Range + 2, this->Range + 2);
	//Wyrmgus start
//	Vec2i minpos = goalPos - offset;
//	Vec2i maxpos = goalPos + offset;
	Vec2i minpos = caster.tilePos - offset;
	Vec2i maxpos = caster.tilePos + Vec2i(caster.Type->get_tile_size() - QSize(1, 1)) + offset;
	//Wyrmgus end

	//Wyrmgus start
//	CMap::get()->FixSelectionArea(minpos, maxpos);
	CMap::get()->FixSelectionArea(minpos, maxpos, z);
	//Wyrmgus end

	//
	// Terrain effect of the explosion
	//
	//Wyrmgus start
	/*
	Vec2i ipos;
	for (ipos.x = minpos.x; ipos.x <= maxpos.x; ++ipos.x) {
		for (ipos.y = minpos.y; ipos.y <= maxpos.y; ++ipos.y) {
			const wyrmgus::tile &mf = *Map.Field(ipos);
			if (SquareDistance(ipos, goalPos) > square(this->Range)) {
				// Not in circle range
				continue;
			} else if (mf.isAWall()) {
				Map.RemoveWall(ipos);
			} else if (mf.RockOnMap()) {
				Map.ClearRockTile(ipos);
			} else if (mf.ForestOnMap()) {
				Map.ClearWoodTile(ipos);
			}
		}
	}
	*/

	if (this->DamageTerrain) {
		Vec2i ipos;
		for (ipos.x = minpos.x; ipos.x <= maxpos.x; ++ipos.x) {
			for (ipos.y = minpos.y; ipos.y <= maxpos.y; ++ipos.y) {
				//Wyrmgus start
//				const wyrmgus::tile &mf = *CMap::get()->Field(ipos);
				const wyrmgus::tile &mf = *CMap::get()->Field(ipos, z);
				//Wyrmgus end
				//Wyrmgus start
//				if (SquareDistance(ipos, caster.tilePos) > square(this->Range)) {
				if (caster.MapDistanceTo(ipos, z) > this->Range) {
				//Wyrmgus end
					// Not in circle range
					continue;
				} else if (mf.RockOnMap() || mf.ForestOnMap() || mf.isAWall()) {
					//Wyrmgus start
//					CMap::get()->ClearOverlayTile(ipos);
					CMap::get()->ClearOverlayTile(ipos, z);
					//Wyrmgus end
				}
			}
		}
	}
	//Wyrmgus end

	//
	//  Effect of the explosion on units. Don't bother if damage is 0
	//
	//Wyrmgus start
	//if (this->Damage) {
	if (this->Damage || this->BasicDamage || this->PiercingDamage || this->FireDamage || this->ColdDamage || this->ArcaneDamage || this->LightningDamage || this->AirDamage || this->EarthDamage || this->WaterDamage || this->AcidDamage || this->ShadowDamage) {
	//Wyrmgus end
		std::vector<CUnit *> table;
		//Wyrmgus start
//		SelectFixed(minpos, maxpos, table);
		SelectFixed(minpos, maxpos, table, z);
		//Wyrmgus end
		for (size_t i = 0; i != table.size(); ++i) {
			CUnit &unit = *table[i];
			if (unit.Type->UnitType != UnitTypeType::Fly && unit.Type->UnitType != UnitTypeType::Space && unit.IsAlive()
				//Wyrmgus start
//				&& unit.MapDistanceTo(goalPos) <= this->Range) {
				// Don't hit flying units!
//				HitUnit(&caster, unit, this->Damage);
				&& unit.MapDistanceTo(caster) <= this->Range && (UnitNumber(unit) != UnitNumber(caster) || this->DamageSelf) && (caster.is_enemy_of(unit) || this->DamageFriendly)) {

				int damage = 0;
				if (this->BasicDamage || this->PiercingDamage || this->FireDamage || this->ColdDamage || this->ArcaneDamage || this->LightningDamage || this->AirDamage || this->EarthDamage || this->WaterDamage || this->AcidDamage || this->ShadowDamage) {
					damage = std::max<int>(this->BasicDamage - unit.Variable[ARMOR_INDEX].Value, 1);
					damage += this->PiercingDamage;
					//apply resistances
					if (this->HackDamage) {
						damage *= 100 - unit.Variable[HACKRESISTANCE_INDEX].Value;
						damage /= 100;
					} else if (this->PierceDamage) {
						damage *= 100 - unit.Variable[PIERCERESISTANCE_INDEX].Value;
						damage /= 100;
					} else if (this->BluntDamage) {
						damage *= 100 - unit.Variable[BLUNTRESISTANCE_INDEX].Value;
						damage /= 100;
					}
					//apply fire and cold damage
					damage += this->FireDamage * (100 - unit.Variable[FIRERESISTANCE_INDEX].Value) / 100;
					damage += this->ColdDamage * (100 - unit.Variable[COLDRESISTANCE_INDEX].Value) / 100;
					damage += this->ArcaneDamage * (100 - unit.Variable[ARCANERESISTANCE_INDEX].Value) / 100;
					damage += this->LightningDamage * (100 - unit.Variable[LIGHTNINGRESISTANCE_INDEX].Value) / 100;
					damage += this->AirDamage * (100 - unit.Variable[AIRRESISTANCE_INDEX].Value) / 100;
					damage += this->EarthDamage * (100 - unit.Variable[EARTHRESISTANCE_INDEX].Value) / 100;
					damage += this->WaterDamage * (100 - unit.Variable[WATERRESISTANCE_INDEX].Value) / 100;
					damage += this->AcidDamage * (100 - unit.Variable[ACIDRESISTANCE_INDEX].Value) / 100;
					damage += this->ShadowDamage * (100 - unit.Variable[SHADOW_RESISTANCE_INDEX].Value) / 100;
					damage *= modifier;
					damage /= 100;
					damage -= SyncRand((damage + 2) / 2);
				}
				HitUnit(&caster, unit, this->Damage + damage);
				//Wyrmgus end
			}
		}
	}

	return 1;
}
