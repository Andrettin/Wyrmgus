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
//
//      (c) Copyright 1999-2012 by Vladi Belperchinov-Shabanski,
//                                 Joris DAUPHIN, and Jimmy Salmon
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

#ifndef SPELL_SPAWNMISSILE_H
#define SPELL_SPAWNMISSILE_H

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "spell/spells.h"

/**
**  Different targets.
*/
enum LocBaseType {
	LocBaseCaster,
	LocBaseTarget
};

/**
**  This struct is used for defining a missile start/stop location.
**
**  It's evaluated like this, and should be more or less flexible.:
**  base coordinates(caster or target) + (AddX,AddY) + (rand()%AddRandX,rand()%AddRandY)
*/
class SpellActionMissileLocation
{
public:
	SpellActionMissileLocation(LocBaseType base) : Base(base) {}

	void ProcessConfigData(const CConfigData *config_data);
	
	LocBaseType Base;	/// The base for the location (caster/target)
	int AddX = 0;		/// Add to the X coordinate
	int AddY = 0;		/// Add to the X coordinate
	int AddRandX = 0;	/// Random add to the X coordinate
	int AddRandY = 0;	/// Random add to the X coordinate
};

class Spell_SpawnMissile : public SpellActionType
{
public:
	virtual void ProcessConfigData(const CConfigData *config_data) override;
	virtual int Cast(CUnit &caster, const CSpell &spell,
					 CUnit *target, const Vec2i &goalPos, int z, int modifier);
	virtual void Parse(lua_State *lua, int startIndex, int endIndex);

private:
	int Damage = 0;							/// Missile damage.
	int LightningDamage = 0;				/// Missile lightning damage.
	int TTL = -1;							/// Missile TTL.
	int Delay = 0;							/// Missile original delay.
	bool UseUnitVar = false;				/// Use the caster's damage parameters
	//Wyrmgus start
	bool AlwaysHits = false;				/// The missile spawned from the spell always hits
	bool AlwaysCritical = false;			/// The damage from the spell is always a critical hit (double damage)
	//Wyrmgus end
	SpellActionMissileLocation StartPoint = LocBaseCaster;	/// Start point description.
	SpellActionMissileLocation EndPoint = LocBaseTarget;	/// Start point description.
	MissileType *Missile = nullptr;			/// Missile fired on cast
};

#endif
