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

#include "spell/spell_action_spawn_missile.h"

#include "include/config.h"
#include "map/map.h"
#include "missile/missile_class.h"
#include "missile.h"
#include "script.h"
#include "unit/unit.h"
#include "unit/unit_find.h"
#include "util/assert_util.h"
#include "util/string_conversion_util.h"
#include "util/util.h"

struct CompareUnitDistance {
	const CUnit *referenceunit;
	CompareUnitDistance(const CUnit &unit): referenceunit(&unit) {}
	bool operator()(const CUnit *c1, const CUnit *c2)
	{
		int d1 = c1->MapDistanceTo(*referenceunit);
		int d2 = c2->MapDistanceTo(*referenceunit);
		if (d1 == d2) {
			return UnitNumber(*c1) < UnitNumber(*c2);
		} else {
			return d1 < d2;
		}
	}
};

/**
**  Parse the missile location description for a spell action.
**
**  @param l         Lua state.
**  @param location  Pointer to missile location description.
**
**  @note This is only here to avoid code duplication. You don't have
**        any reason to USE this:)
*/
static void CclSpellMissileLocation(lua_State *l, wyrmgus::spell_action_spawn_missile::missile_location *location)
{
	assert_throw(location != nullptr);

	if (!lua_istable(l, -1)) {
		LuaError(l, "incorrect argument");
	}
	const int args = static_cast<int>(lua_rawlen(l, -1));
	for (int j = 0; j < args; ++j) {
		const char *value = LuaToString(l, -1, j + 1);
		++j;
		if (!strcmp(value, "base")) {
			value = LuaToString(l, -1, j + 1);
			location->Base = wyrmgus::spell_action_spawn_missile::string_to_location_base_type(value);
		} else if (!strcmp(value, "add-x")) {
			location->AddX = LuaToNumber(l, -1, j + 1);
		} else if (!strcmp(value, "add-y")) {
			location->AddY = LuaToNumber(l, -1, j + 1);
		} else if (!strcmp(value, "add-rand-x")) {
			location->AddRandX = LuaToNumber(l, -1, j + 1);
		} else if (!strcmp(value, "add-rand-y")) {
			location->AddRandY = LuaToNumber(l, -1, j + 1);
		} else {
			LuaError(l, "Unsupported missile location description flag: %s" _C_ value);
		}
	}
}

namespace wyrmgus {

void spell_action_spawn_missile::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "damage") {
		this->Damage = std::stoi(value);
	} else if (key == "lightning_damage") {
		this->LightningDamage = std::stoi(value);
	} else if (key == "use_unit_var") {
		this->UseUnitVar = string::to_bool(value);
	} else if (key == "always_hits") {
		this->AlwaysHits = string::to_bool(value);
	} else if (key == "always_critical") {
		this->AlwaysCritical = string::to_bool(value);
	} else if (key == "delay") {
		this->Delay = std::stoi(value);
	} else if (key == "ttl") {
		this->TTL = std::stoi(value);
	} else if (key == "missile") {
		this->Missile = missile_type::get(value);
	} else if (key == "start_point") {
		this->StartPoint = missile_location(spell_action_spawn_missile::string_to_location_base_type(value));
	} else if (key == "end_point") {
		this->EndPoint = missile_location(spell_action_spawn_missile::string_to_location_base_type(value));
	} else {
		throw std::runtime_error("Invalid spawn missile spell action property: \"" + key + "\".");
	}
}

void spell_action_spawn_missile::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "start_point") {
		scope.process(&this->StartPoint);
	} else if (tag == "end_point") {
		scope.process(&this->EndPoint);
	} else {
		throw std::runtime_error("Invalid spawn missile spell action scope: \"" + tag + "\".");
	}
}

void spell_action_spawn_missile::check() const
{
	if (this->Missile == nullptr) {
		throw std::runtime_error("Use a missile for spawn-missile (with missile).");
	}
}

void spell_action_spawn_missile::Parse(lua_State *l, int startIndex, int endIndex)
{
	for (int j = startIndex; j < endIndex; ++j) {
		const char *value = LuaToString(l, -1, j + 1);
		++j;
		if (!strcmp(value, "damage")) {
			this->Damage = LuaToNumber(l, -1, j + 1);
		} else if (!strcmp(value, "lightning-damage")) {
			this->LightningDamage = LuaToNumber(l, -1, j + 1);
		} else if (!strcmp(value, "use-unit-var")) {
			this->UseUnitVar = true;
			--j;
			//Wyrmgus start
		} else if (!strcmp(value, "always-hits")) {
			this->AlwaysHits = true;
			--j;
		} else if (!strcmp(value, "always-critical")) {
			this->AlwaysCritical = true;
			--j;
			//Wyrmgus end
		} else if (!strcmp(value, "delay")) {
			this->Delay = LuaToNumber(l, -1, j + 1);
		} else if (!strcmp(value, "ttl")) {
			this->TTL = LuaToNumber(l, -1, j + 1);
		} else if (!strcmp(value, "start-point")) {
			lua_rawgeti(l, -1, j + 1);
			CclSpellMissileLocation(l, &this->StartPoint);
			lua_pop(l, 1);
		} else if (!strcmp(value, "end-point")) {
			lua_rawgeti(l, -1, j + 1);
			CclSpellMissileLocation(l, &this->EndPoint);
			lua_pop(l, 1);
		} else if (!strcmp(value, "missile")) {
			value = LuaToString(l, -1, j + 1);
			this->Missile = missile_type::get(value);
		} else {
			LuaError(l, "Unsupported spawn-missile tag: %s" _C_ value);
		}
	}
	// Now, checking value.
	if (this->Missile == nullptr) {
		LuaError(l, "Use a missile for spawn-missile (with missile)");
	}
}

/**
** Cast spawn missile.
**
**  @param caster       Unit that casts the spell
**  @param spell        Spell-type pointer
**  @param target       Target unit that spell is addressed to
**  @param goalPos      TilePos of target spot when/if target does not exist
**
**  @return             =!0 if spell should be repeated, 0 if not
*/
int spell_action_spawn_missile::Cast(CUnit &caster, const spell &, CUnit *target, const Vec2i &goalPos, int z, int modifier)
{
	PixelPos startPos;
	PixelPos endPos;

	/*
		hardcoded, will be done with Lua when it's possible
	*/
	if (this->Missile->get_missile_class() == missile_class::death_coil) {
		const Vec2i offset(2, 2);
		std::vector<CUnit *> table;
		//Wyrmgus start
//		Select(goalPos - offset, goalPos + offset, table);
		Select(goalPos - offset, goalPos + offset, table, z);
		//Wyrmgus end
		int count = 0;
		for (std::vector<CUnit *>::iterator it = table.begin(); it != table.end(); ++it) {
			CUnit &unit = **it;

			if (unit.Type->BoolFlag[ORGANIC_INDEX].value && unit.is_enemy_of(caster)) {
				table[count++] = &unit;
			}
		}
		if (count > 0) {
			std::sort(table.begin(), table.begin() + count, CompareUnitDistance(caster));
			int damageLeft = this->Damage * modifier / 100;
			for (std::vector<CUnit *>::iterator it = table.begin(); it != table.begin() + count && damageLeft > 0; ++it) {
				CUnit &unit = **it;
				if (unit.IsAliveOnMap()) {
					this->StartPoint.evaluate(caster, &unit, unit.tilePos, &startPos);
					this->EndPoint.evaluate(caster, &unit, unit.tilePos, &endPos);
					//Wyrmgus start
//					::Missile *missile = MakeMissile(*this->Missile, startPos, endPos);
					::Missile *missile = MakeMissile(*this->Missile, startPos, endPos, z);
					//Wyrmgus end
					missile->TTL = this->TTL;
					missile->Delay = this->Delay;
					if (it + 1 == table.begin() + count) {
						missile->Damage = damageLeft;
						damageLeft = 0;
					} else {
						missile->Damage = std::min(damageLeft, unit.Variable[HP_INDEX].Value);
						damageLeft -= unit.Variable[HP_INDEX].Value;
					}
					missile->SourceUnit = caster.acquire_ref();
					missile->TargetUnit = unit.acquire_ref();
				}
			}
			return 1;
		}
		return 0;
	} else {
		this->StartPoint.evaluate(caster, target, goalPos, &startPos);
		this->EndPoint.evaluate(caster, target, goalPos, &endPos);

		//Wyrmgus start
//		::Missile *missile = MakeMissile(*this->Missile, startPos, endPos);
		const missile_type *mtype = this->Missile;
		if (mtype->get_missile_class() == missile_class::none && this->UseUnitVar) {
			mtype = caster.GetMissile().Missile;
		}
		::Missile *missile = MakeMissile(*mtype, startPos, endPos, z);
		//Wyrmgus end
		missile->TTL = this->TTL;
		missile->Delay = this->Delay;
		if (this->UseUnitVar) {
			missile->SourceUnit = caster.acquire_ref();
		} else {
			missile->Damage = this->Damage * modifier / 100;
			missile->LightningDamage = this->LightningDamage * modifier / 100;
			if (missile->Damage != 0 || missile->LightningDamage != 0) {
				missile->SourceUnit = caster.acquire_ref();
			}
		}
		//Wyrmgus start
		if (this->AlwaysHits) {
			missile->AlwaysHits = this->AlwaysHits;
		}
		missile->AlwaysCritical = this->AlwaysCritical;
		//Wyrmgus end

		if (target != nullptr) {
			missile->TargetUnit = target->acquire_ref();
		}
	}

	return 1;
}

void spell_action_spawn_missile::missile_location::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "base") {
		this->Base = spell_action_spawn_missile::string_to_location_base_type(value);
	} else if (key == "add_x") {
		this->AddX = std::stoi(value);
	} else if (key == "add_y") {
		this->AddY = std::stoi(value);
	} else if (key == "add_rand_x") {
		this->AddRandX = std::stoi(value);
	} else if (key == "add_rand_y") {
		this->AddRandY = std::stoi(value);
	} else {
		throw std::runtime_error("Invalid spawn missile spell location property: \"" + key + "\".");
	}
}

void spell_action_spawn_missile::missile_location::process_gsml_scope(const gsml_data &scope)
{
	throw std::runtime_error("Invalid spawn missile spell location scope: \"" + scope.get_tag() + "\".");
}

/**
** Evaluate missile location description.
**
** @param location     Parameters for location.
** @param caster       Unit that casts the spell
** @param target       Target unit that spell is addressed to
** @param goalPos      TilePos of target spot when/if target does not exist
** @param res          pointer to PixelPos of the result
*/
void spell_action_spawn_missile::missile_location::evaluate(const CUnit &caster, const CUnit *target, const Vec2i &goalPos, PixelPos *res) const
{
	if (this->Base == location_base_type::caster) {
		*res = caster.get_map_pixel_pos_center();
	} else {
		if (target) {
			*res = target->get_map_pixel_pos_center();
		} else {
			*res = CMap::get()->tile_pos_to_map_pixel_pos_center(goalPos);
		}
	}
	res->x += this->AddX;
	if (this->AddRandX) {
		res->x += SyncRand(this->AddRandX);
	}
	res->y += this->AddY;
	if (this->AddRandY) {
		res->y += SyncRand(this->AddRandY);
	}
}

}
