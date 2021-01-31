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
/**@name script_missile.cpp - The missile-type ccl functions. */
//
//      (c) Copyright 2002-2021 by Lutz Sammer, Jimmy Salmon and Andrettin
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

#include "missile.h"

#include "luacallback.h"
#include "missile/missile_class.h"
#include "script.h"
#include "sound/sound.h"
#include "unit/unit.h"
#include "unit/unit_manager.h"
#include "unit/unit_type.h"
#include "util/exception_util.h"
#include "video/video.h"

namespace wyrmgus {

void missile_type::Load(lua_State *l)
{
	// Parse the arguments
	for (lua_pushnil(l); lua_next(l, 2); lua_pop(l, 1)) {
		const char *value = LuaToString(l, -2);

		if (!strcmp(value, "File")) {
			this->image_file = LuaToString(l, -1);
		} else if (!strcmp(value, "Size")) {
			Vec2i size;
			CclGetPos(l, &size.x, &size.y);
			this->frame_size = size;
		} else if (!strcmp(value, "Frames")) {
			this->frames = LuaToNumber(l, -1);
		} else if (!strcmp(value, "Flip")) {
			this->Flip = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "NumDirections")) {
			this->num_directions = LuaToNumber(l, -1);
		} else if (!strcmp(value, "Transparency")) {
			this->Transparency = LuaToNumber(l, -1);
		} else if (!strcmp(value, "FiredSound")) {
			this->fired_sound = sound::get(LuaToString(l, -1));
		} else if (!strcmp(value, "ImpactSound")) {
			this->impact_sound = sound::get(LuaToString(l, -1));
		} else if (!strcmp(value, "ChangeVariable")) {
			const int index = UnitTypeVar.VariableNameLookup[LuaToString(l, -1)];// User variables
			if (index == -1) {
				exception::throw_with_trace(std::runtime_error("Bad variable name \"" + std::string(LuaToString(l, -1))  + "\"."));
			}
			this->ChangeVariable = index;
		} else if (!strcmp(value, "ChangeAmount")) {
			this->ChangeAmount = LuaToNumber(l, -1);
		} else if (!strcmp(value, "ChangeMax")) {
			this->ChangeMax = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Class")) {
			const char *class_name = LuaToString(l, -1);
			this->missile_class = string_to_missile_class(class_name);
		} else if (!strcmp(value, "NumBounces")) {
			this->NumBounces = LuaToNumber(l, -1);
		} else if (!strcmp(value, "MaxBounceSize")) {
			this->MaxBounceSize = LuaToNumber(l, -1);
		} else if (!strcmp(value, "ParabolCoefficient")) {
			this->ParabolCoefficient = LuaToNumber(l, -1);
		} else if (!strcmp(value, "Delay")) {
			this->StartDelay = LuaToNumber(l, -1);
		} else if (!strcmp(value, "Sleep")) {
			this->sleep = LuaToNumber(l, -1);
		} else if (!strcmp(value, "Speed")) {
			this->speed = LuaToNumber(l, -1);
		} else if (!strcmp(value, "BlizzardSpeed")) {
			this->BlizzardSpeed = LuaToNumber(l, -1);
		//Wyrmgus start
		} else if (!strcmp(value, "AttackSpeed")) {
			this->AttackSpeed = LuaToNumber(l, -1);
		//Wyrmgus end
		} else if (!strcmp(value, "TTL")) {
			this->TTL = LuaToNumber(l, -1);
		} else if (!strcmp(value, "Damage")) {
			this->Damage = CclParseNumberDesc(l);
			lua_pushnil(l);
		} else if (!strcmp(value, "ReduceFactor")) {
			this->ReduceFactor = LuaToNumber(l, -1);
		} else if (!strcmp(value, "SmokePrecision")) {
			this->SmokePrecision = LuaToNumber(l, -1);
		} else if (!strcmp(value, "MissileStopFlags")) {
			this->MissileStopFlags = LuaToNumber(l, -1);
		} else if (!strcmp(value, "DrawLevel")) {
			this->draw_level = LuaToNumber(l, -1);
		} else if (!strcmp(value, "Range")) {
			this->range = LuaToNumber(l, -1);
		} else if (!strcmp(value, "ImpactMissile")) {
			if (!lua_istable(l, -1)) {
				MissileConfig mc;
				mc.Name = LuaToString(l, -1);
				this->Impact.push_back(std::move(mc));
			} else {
				const int impacts = lua_rawlen(l, -1);
				for (int i = 0; i < impacts; ++i) {
					MissileConfig mc;
					mc.Name = LuaToString(l, -1, i + 1);
					this->Impact.push_back(std::move(mc));
				}
			}
		} else if (!strcmp(value, "SmokeMissile")) {
			this->Smoke.Name = LuaToString(l, -1);
		} else if (!strcmp(value, "ImpactParticle")) {
			this->ImpactParticle = std::make_unique<LuaCallback>(l, -1);
		} else if (!strcmp(value, "SmokeParticle")) {
			this->SmokeParticle = std::make_unique<LuaCallback>(l, -1);
		} else if (!strcmp(value, "OnImpact")) {
			this->OnImpact = std::make_unique<LuaCallback>(l, -1);
		} else if (!strcmp(value, "CanHitOwner")) {
			this->CanHitOwner = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "AlwaysFire")) {
			this->AlwaysFire = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "Pierce")) {
			this->Pierce = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "PierceOnce")) {
			this->PierceOnce = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "PierceIgnoreBeforeGoal")) {
			this->PierceIgnoreBeforeGoal = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "IgnoreWalls")) {
			this->IgnoreWalls = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "KillFirstUnit")) {
			this->KillFirstUnit = LuaToBoolean(l, -1);
		} else if (!strcmp(value, "FriendlyFire")) {
			this->FriendlyFire = LuaToBoolean(l, -1);
		//Wyrmgus start
		} else if (!strcmp(value, "AlwaysHits")) {
			this->AlwaysHits = LuaToBoolean(l, -1);
		//Wyrmgus end
		} else if (!strcmp(value, "SplashFactor")) {
			this->SplashFactor = LuaToNumber(l, -1);
		} else if (!strcmp(value, "CorrectSphashDamage")) {
			this->CorrectSphashDamage = LuaToBoolean(l, -1);
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}
}

}

/**
**  Parse missile-type.
**
**  @param l  Lua state.
*/
static int CclDefineMissileType(lua_State *l)
{
	LuaCheckArgs(l, 2);
	if (!lua_istable(l, 2)) {
		LuaError(l, "incorrect argument");
	}

	// Slot identifier
	const char *str = LuaToString(l, 1);
	wyrmgus::missile_type *mtype = wyrmgus::missile_type::get_or_add(str, nullptr);
	mtype->Load(l);
	return 0;
}

/**
**  Create a missile.
**
**  @param l  Lua state.
*/
static int CclMissile(lua_State *l)
{
	wyrmgus::missile_type *type = nullptr;
	PixelPos position(-1, -1);
	PixelPos destination(-1, -1);
	PixelPos source(-1, -1);
	int z = 0;
	Missile *missile = nullptr;

	DebugPrint("FIXME: not finished\n");

	const int args = lua_gettop(l);
	for (int j = 0; j < args; ++j) {
		const char *value = LuaToString(l, j + 1);
		++j;

		if (!strcmp(value, "type")) {
			type = wyrmgus::missile_type::try_get(LuaToString(l, j + 1));
		} else if (!strcmp(value, "pos")) {
			CclGetPos(l, &position.x, &position.y, j + 1);
		} else if (!strcmp(value, "origin-pos")) {
			CclGetPos(l, &source.x, &source.y, j + 1);
		} else if (!strcmp(value, "goal")) {
			CclGetPos(l, &destination.x, &destination.y, j + 1);
		//Wyrmgus start
		} else if (!strcmp(value, "map-layer")) {
			z = LuaToNumber(l, j + 1);
		//Wyrmgus end
		} else if (!strcmp(value, "local")) {
			Assert(type);
			//Wyrmgus start
//			missile = MakeLocalMissile(*type, position, destination);
			missile = MakeLocalMissile(*type, position, destination, z);
			//Wyrmgus end
			missile->Local = 1;
			--j;
		} else if (!strcmp(value, "global")) {
			Assert(type);
			//Wyrmgus start
//			missile = MakeMissile(*type, position, destination);
			missile = MakeMissile(*type, position, destination, z);
			//Wyrmgus end
			missile->position = position;
			missile->source = source;
			missile->destination = destination;
			//Wyrmgus start
			missile->MapLayer = z;
			//Wyrmgus end
			missile->Local = 0;
			--j;
		} else if (!strcmp(value, "frame")) {
			Assert(missile);
			missile->SpriteFrame = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "state")) {
			Assert(missile);
			missile->State = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "anim-wait")) {
			Assert(missile);
			missile->AnimWait = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "wait")) {
			Assert(missile);
			missile->Wait = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "delay")) {
			Assert(missile);
			missile->Delay = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "source")) {
			Assert(missile);
			lua_pushvalue(l, j + 1);
			missile->SourceUnit = CclGetUnitFromRef(l)->acquire_ref();
			lua_pop(l, 1);
		} else if (!strcmp(value, "target")) {
			Assert(missile);
			lua_pushvalue(l, j + 1);
			missile->TargetUnit = CclGetUnitFromRef(l)->acquire_ref();
			lua_pop(l, 1);
		} else if (!strcmp(value, "damage")) {
			Assert(missile);
			missile->Damage = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "lightning-damage")) {
			Assert(missile);
			missile->LightningDamage = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "ttl")) {
			Assert(missile);
			missile->TTL = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "hidden")) {
			Assert(missile);
			missile->Hidden = 1;
			--j;
		} else if (!strcmp(value, "step")) {
			Assert(missile);
			if (!lua_istable(l, j + 1) || lua_rawlen(l, j + 1) != 2) {
				LuaError(l, "incorrect argument");
			}
			missile->CurrentStep = LuaToNumber(l, j + 1, 1);
			missile->TotalStep = LuaToNumber(l, j + 1, 2);
		} else {
			LuaError(l, "Unsupported tag: %s" _C_ value);
		}
	}

	// we need to reinitialize position parameters - that's because of
	// the way InitMissile() (called from MakeLocalMissile()) computes
	// them - it works for creating a missile during a game but breaks
	// loading the missile from a file.
	missile->position = position;
	missile->source = source;
	missile->destination = destination;
	missile->MapLayer = z;
	return 0;
}

/**
**  Define burning building missiles.
**
**  @param l  Lua state.
*/
static int CclDefineBurningBuilding(lua_State *l)
{
	BurningBuildingFrames.clear();

	const int args = lua_gettop(l);
	for (int j = 0; j < args; ++j) {
		if (!lua_istable(l, j + 1)) {
			LuaError(l, "incorrect argument");
		}
		auto ptr = std::make_unique<BurningBuildingFrame>();
		const int subargs = lua_rawlen(l, j + 1);

		for (int k = 0; k < subargs; ++k) {
			const char *value = LuaToString(l, j + 1, k + 1);
			++k;

			if (!strcmp(value, "percent")) {
				ptr->Percent = LuaToNumber(l, j + 1, k + 1);
			} else if (!strcmp(value, "missile")) {
				ptr->Missile = wyrmgus::missile_type::get(LuaToString(l, j + 1, k + 1));
			}
		}
		BurningBuildingFrames.insert(BurningBuildingFrames.begin(), std::move(ptr));
	}
	return 0;
}

/**
**  Create a missile on the map
**
**  @param l  Lua state.
**
*/
static int CclCreateMissile(lua_State *l)
{
	const int arg = lua_gettop(l);
	if (arg < 6 || arg > 7) {
		LuaError(l, "incorrect argument");
	}

	const std::string name = LuaToString(l, 1);
	const wyrmgus::missile_type *mtype = wyrmgus::missile_type::get(name);

	PixelPos startpos, endpos;
	CclGetPos(l, &startpos.x, &startpos.y, 2);
	CclGetPos(l, &endpos.x, &endpos.y, 3);

	const int sourceUnitId = LuaToNumber(l, 4);
	const int destUnitId = LuaToNumber(l, 5);
	const bool dealDamage = LuaToBoolean(l, 6);
	const bool mapRelative = arg == 7 ? LuaToBoolean(l, 7) : false;
	CUnit *sourceUnit = sourceUnitId != -1 ? &wyrmgus::unit_manager::get()->GetSlotUnit(sourceUnitId) : nullptr;
	CUnit *destUnit = destUnitId != -1 ? &wyrmgus::unit_manager::get()->GetSlotUnit(destUnitId) : nullptr;

	if (mapRelative == false) {
		if (sourceUnit != nullptr) {
			startpos += sourceUnit->get_map_pixel_pos_top_left();
		}
		if (destUnit != nullptr) {
			endpos += destUnit->get_map_pixel_pos_top_left();
		}
	}

	//Wyrmgus start
//	Missile *missile = MakeMissile(*mtype, startpos, endpos);
	Missile *missile = MakeMissile(*mtype, startpos, endpos, 0);
	//Wyrmgus end
	if (!missile) {
		return 0;
	}
	if (dealDamage) {
		missile->SourceUnit = sourceUnit->acquire_ref();
	}
	missile->TargetUnit = destUnit->acquire_ref();
	return 0;
}

/**
**  Register CCL features for missile-type.
*/
void MissileCclRegister()
{
	lua_register(Lua, "DefineMissileType", CclDefineMissileType);
	lua_register(Lua, "Missile", CclMissile);
	lua_register(Lua, "DefineBurningBuilding", CclDefineBurningBuilding);
	lua_register(Lua, "CreateMissile", CclCreateMissile);
}
